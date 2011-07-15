/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <io.h>
#include <string>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>

#include "global.h"
#include "brutefir.hpp"
#include "preprocessor.hpp"
#include "coeff.hpp"
#include "buffer.hpp"
#include "app_path.hpp"
#include "util.hpp"
#include "hash.h"
#include "numunion.h"

namespace preprocessor
{
    // Convolves a set of impulse responses into a single one.
    //
    // Parameters:
    //   impulse_info   the set of impulse responses
    //   filter_length  the convolution filter length
    //   realsize       the "float" size
    //
    // Returns the name of the processed file or empty on error.
    std::wstring
    convolve_impulses(std::vector<struct impulse_info> impulse_info,
                      int filter_length,
                      int realsize)
    {
        int n;
        int n_channels, g_channels = 0;
        int sampling_rate, g_sampling_rate = 0;
        int n_frames, g_frames = 0;
        int filter_blocks;
        int length;
        long hash_code = 0;

        std::vector<struct impulse_info>::iterator it;
        std::wstring fn_concat;
        std::wstringstream out;
        std::wstring m_out_filename;

        brutefir *filter;
        bool_t status;

        void **coeffs;
        void *inbuf;
        void *outbuf;

        // find the largest frame size
        for (it = impulse_info.begin(); it < impulse_info.end(); it++)
        {
            fn_concat.append(it->filename);

            buffer::get_snd_file_params(it->filename.c_str(), &n_channels, &n_frames, &sampling_rate);

            if (n_frames > g_frames)
            {
                g_frames = n_frames;
            }

            if ((g_channels != 0) && (g_channels != n_channels))
            {
                throw;
            }

            if ((g_sampling_rate != 0) && (g_sampling_rate != sampling_rate))
            {
                throw;
            }

            g_channels = n_channels;
            g_sampling_rate = sampling_rate;
        }

        // calculate filter blocks
        length = util::get_next_multiple(g_frames, filter_length);
        filter_blocks = length / filter_length;

        // assemble the output filename
        hash_code = DJBHash((char *)fn_concat.c_str(), fn_concat.size());

        out << "file-" << std::hex << hash_code
            << "-" << std::dec << g_frames
            << "-" << realsize
            << "-" << g_channels
            << "-" << g_sampling_rate
            << ".wav";

        m_out_filename.assign(app_path::append_temp_path(out.str()));

        // run the impulse convolver if the output file does not already exist
        if (!boost::filesystem::exists(m_out_filename))
        {
            // instantiate filter
            filter = new brutefir(
                filter_length,
                filter_blocks,
                realsize,
                g_channels,
                (realsize == 4) ? BF_SAMPLE_FORMAT_FLOAT_LE : BF_SAMPLE_FORMAT_FLOAT64_LE,
                (realsize == 4) ? BF_SAMPLE_FORMAT_FLOAT_LE : BF_SAMPLE_FORMAT_FLOAT64_LE,
                g_sampling_rate,
                false);

            // allocate the output buffer
            outbuf = _aligned_malloc(filter_length * filter_blocks * g_channels * realsize,
                                     ALIGNMENT);

            memset(outbuf, 0, filter_length * filter_blocks * g_channels * realsize);

            // use a dirac impulse response for the initial coefficients
            coeffs = coeff::load_dirac_coeff(g_channels, filter_length, realsize);
            filter->set_coeff(coeffs, g_channels, filter_length, filter_blocks, 1.0);

            for (it = impulse_info.begin(); it < impulse_info.end(); it++)
            {
                // load the sound file into the input buffer
                inbuf = buffer::load_from_snd_file(it->filename.c_str(),
                                                   &n_channels,
                                                   &n_frames,
                                                   realsize,
                                                   filter_length * filter_blocks,
                                                   true);

                // break on error
                if ((inbuf == NULL) || (n_channels != g_channels))
                {
                    m_out_filename.clear();
                    break;
                }

                // run the filter
                status = true;
                for (n = 0; n < filter_blocks; n++)
                {
                    status &= (filter->run(&(((uint8_t *)inbuf)[n * filter_length * g_channels * realsize]),
                                           &(((uint8_t *)outbuf)[n * filter_length * g_channels * realsize]))
                                           == 0);
                }

                // free coefficients
                if (coeffs != NULL)
                {
                    for (n = 0; n < g_channels; n++)
                    {
                        if (coeffs[n] != NULL)
                        {
                            _aligned_free(coeffs[n]);
                            coeffs[n] = NULL;
                        }
                    }

                    _aligned_free(coeffs);
                    coeffs = NULL;
                }

                // the output buffer becomes the coefficients for the next iteration
                if (status)
                {
                    coeffs = buffer::deinterlace(outbuf,
                                                 g_channels,
                                                 filter_length * filter_blocks,
                                                 realsize);

                    filter->set_coeff(coeffs,
                                        g_channels,
                                        filter_length,
                                        filter_blocks,
                                        it->scale);
                }

                // free the input buffer
                if (inbuf != NULL)
                {
                    _aligned_free(inbuf);
                    inbuf = NULL;
                }

                // break on error
                if (!status)
                {
                    m_out_filename.clear();
                    break;
                }
            }

            if (!m_out_filename.empty())
            {
                // write the final output buffer to the output file
                buffer::save_to_snd_file(m_out_filename.c_str(),
                                         outbuf, g_channels,
                                         g_frames,
                                         realsize,
                                         g_sampling_rate);
            }

            // free the output buffer
            if (outbuf != NULL)
            {
                _aligned_free(outbuf);
                outbuf = NULL;
            }

            // free coefficients
            if (coeffs != NULL)
            {
                for (n = 0; n < g_channels; n++)
                {
                    if (coeffs[n] != NULL)
                    {
                        _aligned_free(coeffs[n]);
                        coeffs[n] = NULL;
                    }
                }

                _aligned_free(coeffs);
                coeffs = NULL;
            }

            delete filter;
        }

        return m_out_filename;
    }

    // Calculates the attenuation in dB for an impulse response
    // to avoid clipping.
    //
    // Parameters:
    //   filename       the name of impulse response file
    //   filter_length  the length of convolution filter
    //   realsize       the "float" size
    //   attenuation    returns the attenuation in dB
    //   n_channels     returns the number of channels in the file
    //   n_frames       returns the number of frames in the file
    //   sampling_rate  returns the sampling rate of the file
    //
    // Returns:
    //   true if successful, false otherwise.
    bool_t
    calculate_attenuation(std::wstring filename,
                          int filter_length,
                          int realsize,
                          double *attenuation,
                          int *n_channels,
                          int *n_frames,
                          int *sampling_rate)
    {
        int n, i;
        int length;
        int filter_blocks;
        int n_coeffs;

        brutefir * filter;
        bool_t status = false;

        void **coeffs;
        void *inbuf;
        void *outbuf;
        numunion_t *realbuf;

        *attenuation = 0;

        // get impulse response file parameters
        if (!buffer::get_snd_file_params(filename.c_str(),
                                        n_channels,
                                        n_frames,
                                        sampling_rate))
        {
            goto exit;
        }

        // calculate filter blocks
        length = util::get_next_multiple(*n_frames, filter_length);
        filter_blocks = length / filter_length;

        // instantiate filter
        filter = new brutefir(
            filter_length,
            filter_blocks,
            realsize,
            *n_channels,
            (realsize == 4) ? BF_SAMPLE_FORMAT_FLOAT_LE : BF_SAMPLE_FORMAT_FLOAT64_LE,
            (realsize == 4) ? BF_SAMPLE_FORMAT_FLOAT_LE : BF_SAMPLE_FORMAT_FLOAT64_LE,
            *sampling_rate,
            false);

        // allocate the output buffer
        outbuf = _aligned_malloc(filter_length * *n_channels * realsize, ALIGNMENT);
        memset(outbuf, 0, filter_length * *n_channels * realsize);

        // load the impulse response from sound file
        coeffs = coeff::load_snd_coeff(filename.c_str(),
                                       &length,
                                       realsize,
                                       filter_length * filter_blocks,
                                       &n_coeffs);

        if (coeffs == NULL)
        {
            goto exit;
        }

        filter->set_coeff(coeffs, *n_channels, filter_length, filter_blocks, 1.0);

        float max_valuef = 0;
        double max_valued = 0;

        // load full scale white noise into the input buffer
        inbuf = buffer::load_white_noise(*n_channels,
                                         filter_length * filter_blocks,
                                         realsize);

        if (inbuf == NULL)
        {
            goto exit;
        }

        // run the filter
        for (n = 0; n < filter_blocks; n++)
        {
            if (filter->run(&(((uint8_t *)inbuf)[n * filter_length * *n_channels * realsize]),
                            outbuf)
                            == 0)
            {
                // find the largest value in the output
                realbuf = (numunion_t *)outbuf;

                for (i = 0; i < filter_length * *n_channels; i++)
                {
                    if (realsize == 4)
                    {
                        if (fabs(realbuf->r32[i]) > max_valuef)
                        {
                            max_valuef = fabs(realbuf->r32[i]);
                        }
                    }
                    else
                    {
                        if (fabs(realbuf->r64[i]) > max_valued)
                        {
                            max_valued = fabs(realbuf->r64[i]);
                        }
                    }
                }
            }
        }

        if (realsize == 4)
        {
            if (max_valuef > 1)
            {
                *attenuation = -TO_DB(max_valuef);
            }
        }
        else
        {
            if (max_valued > 1)
            {
                *attenuation = -TO_DB(max_valued);
            }
        }

        status = true;

exit:
        // free the input buffer
        if (inbuf != NULL)
        {
            _aligned_free(inbuf);
            inbuf = NULL;
        }

        // free the output bilfer
        if (outbuf != NULL)
        {
            _aligned_free(outbuf);
            outbuf = NULL;
        }

        // free coefficients
        if (coeffs != NULL)
        {
            for (n = 0; n < *n_channels; n++)
            {
                if (coeffs[n] != NULL)
                {
                    _aligned_free(coeffs[n]);
                    coeffs[n] = NULL;
                }
            }

            _aligned_free(coeffs);
            coeffs = NULL;
        }

        delete filter;

        return status;
    }
}
