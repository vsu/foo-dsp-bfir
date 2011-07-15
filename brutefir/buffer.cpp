#include <malloc.h>
#include <string.h>

#include <boost/generator_iterator.hpp>
#include <sndfile.h>

#include "global.h"
#include "raw2real.hpp"
#include "real2raw.hpp"
#include "buffer.hpp"

namespace buffer
{
    // Loads the given sound file into an interlaced buffer.
    //
    // Parameters:
    //   filename      the sound filename
    //   n_channels    returns the number of channels
    //   n_frames      returns the number of frames
    //   realsize      the "float" size
    //   max_frames    the maximum number of frames
    //   pad           true to pad data with zeros to max_frames
    //
    // Returns:
    //   buffer with sound file data.
    void *
    load_from_snd_file(const wchar_t *filename,
                       int *n_channels,
                       int *n_frames,
                       int realsize,
                       int max_frames,
                       bool_t pad)
    {
        void *buffer;
        sf_count_t frames_read;
        SNDFILE *snd_file;
        SF_INFO sf_info;

        // convert filename to char
        int size = wcstombs(NULL, filename, 0);
        char *filename_mbs = (char *) _alloca((size + 1) * sizeof(char));
        wcstombs(filename_mbs, filename, size + 1);

        // open the sound file
        sf_info.format = 0;
        snd_file = sf_open(filename_mbs, SFM_READ, &sf_info);

        if (snd_file == NULL)
        {
            return NULL;
        }

        *n_channels = sf_info.channels;
        *n_frames = (max_frames > sf_info.frames) ? (int)sf_info.frames : max_frames;

        if (pad)
        {
            buffer = _aligned_malloc(max_frames * *n_channels * realsize, ALIGNMENT);
            memset(buffer, 0, max_frames * *n_channels * realsize);
        }
        else
        {
            buffer = _aligned_malloc(*n_frames * *n_channels * realsize, ALIGNMENT);
            memset(buffer, 0, *n_frames * *n_channels * realsize);
        }

        // read frames from the sound file.
        // if the sound file data does not fill the coefficient buffer,
        // the remaining space should contain zeros.
        if (realsize == 4)
        {
            frames_read = sf_readf_float(snd_file, (float *)buffer, *n_frames);
        }
        else
        {
            frames_read = sf_readf_double(snd_file, (double *)buffer, *n_frames);
        }

        // close the sound file
        sf_close(snd_file);

        if (frames_read != *n_frames)
        {
            _aligned_free(buffer);
            buffer = NULL;
        }

        return buffer;
    }

    // Saves the given interlaced buffer to a WAV sound file.
    //
    // Parmeters:
    //   filename       specifies the sound filename
    //   buffer         data buffer
    //   n_channels     the number of channels
    //   n_frames       the number of frames
    //   realsize       the "float" size
    //   sampling_rate  the sampling rate
    void
    save_to_snd_file(const wchar_t *filename,
                     void *buffer,
                     int n_channels,
                     int n_frames,
                     int realsize,
                     int sampling_rate)
    {
        SNDFILE *snd_file;
        SF_INFO sf_info;

        // convert filename to char
        int size = wcstombs(NULL, filename, 0);
        char *filename_mbs = (char *) _alloca((size + 1) * sizeof(char));
        wcstombs(filename_mbs, filename, size + 1);

        sf_info.channels = n_channels;
        sf_info.format = (realsize == 4)
                            ? SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_LITTLE
                            : SF_FORMAT_WAV | SF_FORMAT_DOUBLE | SF_ENDIAN_LITTLE;
        sf_info.frames = n_frames;
        sf_info.samplerate = sampling_rate;

        snd_file = sf_open(filename_mbs, SFM_WRITE, &sf_info);

        if (snd_file != NULL)
        {
            if (realsize == 4)
            {
                sf_writef_float(snd_file, (float *)buffer, n_frames);
            }
            else
            {
                sf_writef_double(snd_file, (double *)buffer, n_frames);
            }

            sf_close(snd_file);
        }
    }


    // Queries parameters from the specified sound file.
    //
    // Parameters:
    //   n_channels      returns number of channels
    //   n_frames        returns number of frames
    //   sampliing_rate  returns sampling rate
    //
    // Returns:
    //   true if successful, false otherwise.
    bool_t
    get_snd_file_params(const wchar_t *filename,
                        int *n_channels,
                        int *n_frames,
                        int *sampling_rate)
    {
        bool_t result = false;
        SNDFILE *snd_file;
        SF_INFO sf_info;

         // convert filename to char
        int size = wcstombs(NULL, filename, 0);
        char *filename_mbs = (char *) _alloca((size + 1) * sizeof(char));
        wcstombs(filename_mbs, filename, size + 1);

        // open the sound file
        sf_info.format = 0;
        snd_file = sf_open(filename_mbs, SFM_READ, &sf_info);

        if (snd_file == NULL)
        {
            return false;
        }

        *n_channels = sf_info.channels;
        *sampling_rate = sf_info.samplerate;
        *n_frames = (int)sf_info.frames;

        sf_close(snd_file);
        return true;
    }

    // Checks if the specified sound file has the given number
    // of channels and sampling rate.
    bool_t
    check_snd_file(const wchar_t *filename,
                   int n_channels,
                   int sampling_rate)
    {
        bool_t result = false;
        int file_channels;
        int file_sampling_rate;
        int file_frames;

        // get the sound file parameters
        if (get_snd_file_params(filename,
                                &file_channels,
                                &file_frames,
                                &file_sampling_rate))
        {
            result = ((file_channels == n_channels) &&
                      (file_sampling_rate == sampling_rate));
        }

        return result;
    }

    // Deinterlaces the specified buffer into separate buffers
    // for each channel.
    //
    // Parameters:
    //   buffer        data buffer
    //   n_channels    number of channels
    //   n_frames      sampling rate
    //   realsize      the "float" size
    //
    // Returns:
    //   Separate buffers for each channel.
    void **
    deinterlace(void *buffer,
                int n_channels,
                int n_frames,
                int realsize)
    {
        int n;
        void **bufs;

        bufs = (void **) _aligned_malloc(n_channels * sizeof(void *), ALIGNMENT);

        for (n = 0; n < n_channels; n++)
        {
            bufs[n] = _aligned_malloc(n_frames * realsize, ALIGNMENT);
            memset(bufs[n], 0, n_frames * realsize);

            if (realsize == 4)
            {
                raw2real::raw2realf(bufs[n],
                                    &(((uint8_t *)buffer)[n * realsize]),
                                    realsize,
                                    0,
                                    true,
                                    n_channels,
                                    false,
                                    n_frames);
            }
            else
            {
                raw2real::raw2reald(bufs[n],
                                    &(((uint8_t *)buffer)[n * realsize]),
                                    realsize,
                                    0,
                                    true,
                                    n_channels,
                                    false,
                                    n_frames);
            }
        }

        return bufs;
    }

    // Interlaces the specified buffers for each channel into
    // a single buffer.
    //
    // Parameters:
    //   buffers     interlaced buffers
    //   n_channels  number of channels
    //   n_frames    number of frames
    //   realsize    the "float" size
    //
    // Returns:
    //   A single interlaced buffer.
    void *
    interlace(void **buffers,
              int n_channels,
              int n_frames,
              int realsize)
    {
        int n;
        void *buf;
        struct bfoverflow_t overflow;

        buf = _aligned_malloc(n_channels * n_frames * realsize, ALIGNMENT);
        memset(buf, 0, n_channels * n_frames * realsize);

        for (n = 0; n < n_channels; n++)
        {
            if (realsize == 4)
            {
                real2raw::real2rawf_no_dither(&(((uint8_t *)buf)[n * realsize]),
                                              buffers[n],
                                              0,
                                              realsize,
                                              0,
                                              true,
                                              n_channels,
                                              false,
                                              n_frames,
                                              &overflow,
                                              NULL);
            }
            else
            {
                real2raw::real2rawd_no_dither(&(((uint8_t *)buf)[n * realsize]),
                                              buffers[n],
                                              0,
                                              realsize,
                                              0,
                                              true,
                                              n_channels,
                                              false,
                                              n_frames,
                                              &overflow,
                                              NULL);
            }
        }

        return buf;
    }

    // Loads random full scale white noise into an interlaced buffer.
    //
    // Parameters:
    //   n_channels  number of channels
    //   n_frames    number of frames
    //   realsize    the "float" size
    //
    // Returns:
    //   A buffer with noise data.
    void *
    load_white_noise(int n_channels,
                     int n_frames,
                     int realsize)
    {
        void *buffer;

        buffer = _aligned_malloc(n_channels * n_frames * realsize, ALIGNMENT);

        // Define a uniform random number distribution which produces "float"
        // values between -1 and 1 (-1 inclusive, 1 exclusive).
        if (realsize == 4)
        {
            typedef boost::uniform_real<float> fdistribution_type;
            typedef boost::variate_generator<base_generator_type&, fdistribution_type> fgen_type;

            fdistribution_type uni_dist(-1.0f, 1.0f);
            fgen_type uni(generator, uni_dist);

            std::vector<float> samples(n_channels * n_frames);
            std::generate_n(samples.begin(), n_channels * n_frames, uni);

            memcpy(buffer, &samples[0], realsize * samples.size());
        }
        else
        {
            typedef boost::uniform_real<double> ddistribution_type;
            typedef boost::variate_generator<base_generator_type&, ddistribution_type> dgen_type;

            ddistribution_type uni_dist(-1.0f, 1.0f);
            dgen_type uni(generator, uni_dist);

            std::vector<double> samples(n_channels * n_frames);
            std::generate_n(samples.begin(), n_channels * n_frames, uni);

            memcpy(buffer, &samples[0], realsize * samples.size());
        }

        return buffer;
    }
}