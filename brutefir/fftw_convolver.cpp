/*
 * (c) 2011 Victor Su
 * (c) 2001-2004, 2006, 2009 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <intrin.h>
#include <malloc.h>
#include <sstream>

#include <fftw3.h>

#include "global.h"
#include "fftw_convolver.hpp"
#include "dither.hpp"
#include "raw2real.hpp"
#include "real2raw.hpp"
#include "app_path.hpp"
#include "log2.h"
#include "bit.h"
#include "timestamp.h"
#include "pinfo.h"

// This definition is necessary to workaround an issue with
// file streams when FFTW is compiled as a DLL
static void my_fftw_write_char(char c, void *f) { fputc(c, (FILE *) f); }
#define fftw_export_wisdom_to_file(f) fftw_export_wisdom(my_fftw_write_char, (void*) (f))
#define fftwf_export_wisdom_to_file(f) fftwf_export_wisdom(my_fftw_write_char, (void*) (f))
#define fftwl_export_wisdom_to_file(f) fftwl_export_wisdom(my_fftw_write_char, (void*) (f))

// This definition is necessary to workaround an issue with
// file streams when FFTW is compiled as a DLL
static int my_fftw_read_char(void *f) { return fgetc((FILE *) f); }
#define fftw_import_wisdom_from_file(f) fftw_import_wisdom(my_fftw_read_char, (void*) (f))
#define fftwf_import_wisdom_from_file(f) fftwf_import_wisdom(my_fftw_read_char, (void*) (f))
#define fftwl_import_wisdom_from_file(f) fftwl_import_wisdom(my_fftw_read_char, (void*) (f))

#define ifftplans fftplan_table[1][0]
#define ifftplans_inplace fftplan_table[1][1]
#define fftplans fftplan_table[0][0]
#define fftplans_inplace fftplan_table[0][1]

fftw_convolver::fftw_convolver(int length,
                               int _realsize,
                               dither *dither)
    : m_dither(dither)
{
    int order, ret;
    FILE *stream;
    errno_t err;
    std::wstringstream out;
    std::wstring filename;

    realsize = _realsize;

    if (realsize != 4 && realsize != 8)
    {
        pinfo("Invalid real size %d.\n", realsize);
        throw;
    }

    if ((order = log2_get(length)) == -1)
    {
        pinfo("Invalid length %d.\n", length);
        throw;
    }

    order++;
    fft_order = order;
    n_fft = 2 * length;
    n_fft2 = length;

    // create a wisdom filename from convolver parameters
    out << "wisdom-" << length << "-" << realsize;
    filename.assign(app_path::append_path(out.str()));

    if ((err = _wfopen_s(&stream, filename.c_str(), L"rt")) != 0)
    {
        if (errno != ENOENT)
        {
            pinfo("Could not open \"%s\" for reading: %s.\n",
                  filename.c_str(), strerror(errno));
            throw;
        }
    }
    else
    {
        if (realsize == 4)
        {
            ret = fftwf_import_wisdom_from_file(stream);
        }
        else
        {
            ret = fftw_import_wisdom_from_file(stream);
        }

        fclose(stream);
    }

    memset(fftplan_generated, 0, sizeof(fftplan_generated));

    pinfo("Creating 4 FFTW plans of size %d.", 1 << fft_order);

    create_fft_plan(fft_order, 0, 0);
    create_fft_plan(fft_order, 0, 1);
    create_fft_plan(fft_order, 1, 0);
    create_fft_plan(fft_order, 1, 1);

    // Wisdom is cumulative, save it each time (and get wiser)
    if ((err = _wfopen_s(&stream, filename.c_str(), L"wt+")) != 0)
    {
        pinfo("Warning: could not save wisdom:\n"
              "  could not open \"%s\" for writing: %s.\n",
              filename.c_str(), strerror(err));
    }
    else
    {
        if (realsize == 4)
        {
            fftwf_export_wisdom_to_file(stream);
        }
        else
        {
            fftw_export_wisdom_to_file(stream);
        }

        fclose(stream);
    }
}

fftw_convolver::~fftw_convolver()
{
    int fft_order;

    // Clean up plans
    for (fft_order = 0; fft_order < 32; fft_order++)
    {
        destroy_fft_plan(fft_order, 0, 0);
        destroy_fft_plan(fft_order, 0, 1);
        destroy_fft_plan(fft_order, 1, 0);
        destroy_fft_plan(fft_order, 1, 1);
    }

    m_dither = NULL;
}

void
fftw_convolver::convolver_raw2cbuf(void *rawbuf,
                                   void *cbuf,
                                   void *next_cbuf,
                                   struct buffer_format_t *bf,
                                   void (*postprocess)(void *realbuf,
                                                       int n_samples,
                                                       void *arg),
                                   void *pp_arg)
{
    if (realsize == 4)
    {
        raw2real::raw2realf(next_cbuf, (void *)&((uint8_t *)rawbuf)[bf->byte_offset],
                            bf->sf.bytes, (bf->sf.bytes - bf->sf.sbytes) << 3,
                            bf->sf.isfloat, bf->sample_spacing, bf->sf.swap, n_fft2);
    }
    else
    {
        raw2real::raw2reald(next_cbuf, (void *)&((uint8_t *)rawbuf)[bf->byte_offset],
                            bf->sf.bytes, (bf->sf.bytes - bf->sf.sbytes) << 3,
                            bf->sf.isfloat, bf->sample_spacing, bf->sf.swap, n_fft2);
    }

    if (postprocess != NULL)
    {
        postprocess(next_cbuf, n_fft2, pp_arg);
    }

    memcpy(&((uint8_t *)cbuf)[n_fft2 * realsize], next_cbuf, n_fft2 * realsize);
}

void
fftw_convolver::convolver_time2freq(void *input_cbuf,
                                    void *output_cbuf)
{
    void *fftplan;

    if (input_cbuf == output_cbuf)
    {
        fftplan = fftplans_inplace[fft_order];
    }
    else
    {
        fftplan = fftplans[fft_order];
    }

    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)fftplan,
                          (float *)input_cbuf, (float *)output_cbuf);
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)fftplan,
                         (double *)input_cbuf, (double *)output_cbuf);
    }
}

void
fftw_convolver::convolver_mixnscale(void *input_cbufs[],
                                    void *output_cbuf,
                                    double scales[],
                                    int n_bufs,
                                    int mixmode)
{
    if (realsize == 4)
    {
        mixnscalef(input_cbufs, output_cbuf, scales, n_bufs, mixmode);
    }
    else
    {
        mixnscaled(input_cbufs, output_cbuf, scales, n_bufs, mixmode);
    }
}

void
fftw_convolver::convolver_convolve_inplace(void *cbuf,
                                           void *coeffs)
{
    if (realsize == 4)
    {
        convolve_inplacef(cbuf, coeffs);
    }
    else
    {
        convolve_inplaced(cbuf, coeffs);
    }
}

void
fftw_convolver::convolver_convolve(void *input_cbuf,
                                   void *coeffs,
                                   void *output_cbuf)
{
    if (realsize == 4)
    {
        convolvef(input_cbuf, coeffs, output_cbuf);
    }
    else
    {
        convolved(input_cbuf, coeffs, output_cbuf);
    }
}

void
fftw_convolver::convolver_convolve_add(void *input_cbuf,
                                       void *coeffs,
                                       void *output_cbuf)
{
    if (realsize == 4)
    {
        convolve_addf(input_cbuf, coeffs, output_cbuf);
    }
    else
    {
        convolve_addd(input_cbuf, coeffs, output_cbuf);
    }
}

void
fftw_convolver::convolver_crossfade_inplace(void *input_cbuf,
                                            void *crossfade_cbuf,
                                            void *buffer_cbuf)
{
    void *buf1, *buf2;
    double scale, d;
    float f;
    int n;

    buf1 = buffer_cbuf;
    buf2 = &((uint8_t *)buffer_cbuf)[n_fft * realsize];
    scale = 1.0;

    convolver_mixnscale(&crossfade_cbuf, buffer_cbuf, &scale, 1,
                        CONVOLVER_MIXMODE_OUTPUT);
    convolver_freq2time(buffer_cbuf, crossfade_cbuf);
    convolver_mixnscale(&input_cbuf, buffer_cbuf, &scale, 1,
                        CONVOLVER_MIXMODE_OUTPUT);
    convolver_freq2time(buffer_cbuf, buffer_cbuf);

    if (realsize == 4)
    {
        f = 1.0 / (float)(n_fft2 - 1);
        for (n = 0; n < n_fft2; n++)
        {
            ((float *)buffer_cbuf)[n] =
                ((float *)crossfade_cbuf)[n] * (1.0 - f * (float)n) +
                ((float *)buffer_cbuf)[n] * f * (float)n;
        }
    }
    else
    {
        d = 1.0 / (double)(n_fft2 - 1);
        for (n = 0; n < n_fft2; n++)
        {
            ((double *)buf1)[n] =
                ((double *)buf1)[n] * (1.0 - d * (double)n) +
                ((double *)buf2)[n] * d * (double)n;
        }
    }

    convolver_time2freq(buffer_cbuf, buffer_cbuf);
    scale = 1.0 / (double)n_fft;
    convolver_mixnscale(&buffer_cbuf, input_cbuf, &scale, 1,
                        CONVOLVER_MIXMODE_INPUT);
}

void
fftw_convolver::convolver_dirac_convolve_inplace(void *cbuf)
{
    if (realsize == 4)
    {
        dirac_convolve_inplacef(cbuf);
    }
    else
    {
        dirac_convolve_inplaced(cbuf);
    }
}

void
fftw_convolver::convolver_dirac_convolve(void *input_cbuf,
                                         void *output_cbuf)
{
    if (realsize == 4)
    {
        dirac_convolvef(input_cbuf, output_cbuf);
    }
    else
    {
        dirac_convolved(input_cbuf, output_cbuf);
    }
}

void
fftw_convolver::convolver_freq2time(void *input_cbuf,
                                    void *output_cbuf)
{
    void *ifftplan;

    if (input_cbuf == output_cbuf)
    {
        ifftplan = ifftplans_inplace[fft_order];
    }
    else
    {
        ifftplan = ifftplans[fft_order];
    }

    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)ifftplan,
                          (float *)input_cbuf, (float *)output_cbuf);
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)ifftplan,
                         (double *)input_cbuf, (double *)output_cbuf);
    }
}

void
fftw_convolver::convolver_convolve_eval(void *input_cbuf,
                                        void *buffer_cbuf, // 1.5 x size
                                        void *output_cbuf)
{
    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)ifftplans[fft_order],
                          (float *)input_cbuf,
                          &((float *)buffer_cbuf)[n_fft2]);
        fftwf_execute_r2r((const fftwf_plan)fftplans[fft_order],
                          (float *)buffer_cbuf,
                          (float *)output_cbuf);
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)ifftplans[fft_order],
                         (double *)input_cbuf,
                         &((double *)buffer_cbuf)[n_fft2]);
        fftw_execute_r2r((const fftw_plan)fftplans[fft_order],
                         (double *)buffer_cbuf,
                         (double *)output_cbuf);
    }

    memcpy(buffer_cbuf, &((uint8_t *)buffer_cbuf)[n_fft2 * realsize],
           n_fft2 * realsize);
}

void
fftw_convolver::convolver_cbuf2raw(void *cbuf,
                                   void *outbuf,
                                   struct buffer_format_t *bf,
                                   bool apply_dither,
                                   struct dither_state_t *dither_state,
                                   struct bfoverflow_t *overflow)
{
    if (m_dither == NULL)
    {
        pinfo("Dither instance not set.");
        throw;
    }

    if (realsize == 4)
    {
        if (apply_dither && !bf->sf.isfloat)
        {
            m_dither->dither_preloop_real2int_hp_tpdf(dither_state, n_fft2);

            real2raw::real2rawf_hp_tpdf((void *)&((uint8_t *)outbuf)[bf->byte_offset],
                                        cbuf, bf->sf.sbytes << 3, bf->sf.bytes,
                                        (bf->sf.bytes - bf->sf.sbytes) << 3,
                                        bf->sf.isfloat, bf->sample_spacing,
                                        bf->sf.swap, n_fft2, overflow, dither_state, 
                                        m_dither);
        }
        else
        {
            real2raw::real2rawf_no_dither((void *)&((uint8_t *)outbuf)[bf->byte_offset],
                                          cbuf, bf->sf.sbytes << 3, bf->sf.bytes,
                                          (bf->sf.bytes - bf->sf.sbytes) << 3,
                                          bf->sf.isfloat, bf->sample_spacing,
                                          bf->sf.swap, n_fft2, overflow,
                                          m_dither);
        }
    }
    else
    {
        if (apply_dither && !bf->sf.isfloat)
        {
            m_dither->dither_preloop_real2int_hp_tpdf(dither_state, n_fft2);

            real2raw::real2rawd_hp_tpdf((void *)&((uint8_t *)outbuf)[bf->byte_offset],
                                        cbuf, bf->sf.sbytes << 3, bf->sf.bytes,
                                        (bf->sf.bytes - bf->sf.sbytes) << 3,
                                        bf->sf.isfloat, bf->sample_spacing,
                                        bf->sf.swap, n_fft2, overflow, dither_state,
                                        m_dither);
        }
        else
        {
            real2raw::real2rawd_no_dither((void *)&((uint8_t *)outbuf)[bf->byte_offset],
                                          cbuf, bf->sf.sbytes << 3,
                                          bf->sf.bytes,
                                          (bf->sf.bytes - bf->sf.sbytes) << 3,
                                          bf->sf.isfloat, bf->sample_spacing,
                                          bf->sf.swap, n_fft2, overflow,
                                          m_dither);
        }
    }
}

int
fftw_convolver::convolver_cbufsize(void)
{
    return n_fft * realsize;
}

void *
fftw_convolver::convolver_coeffs2cbuf(void *coeffs,
                                      int n_coeffs,
                                      double scale,
                                      void *optional_dest)
{
    void *rcoeffs, *coeffs_data;
    int n, len;

    len = (n_coeffs > n_fft2) ? n_fft2 : n_coeffs;
    rcoeffs = _aligned_malloc(n_fft * realsize, ALIGNMENT);
    memset(rcoeffs, 0, n_fft * realsize);

    if (realsize == 4)
    {
        for (n = 0; n < len; n++)
        {
            ((float *)rcoeffs)[n_fft2 + n] = ((float *)coeffs)[n] * (float)scale;

            if (!_finite((double)((float *)rcoeffs)[n_fft2 + n]))
            {
                pinfo("NaN or Inf value among coefficients.\n");
                return NULL;
            }
        }

        fftwf_execute_r2r((const fftwf_plan)fftplans_inplace[fft_order],
                          (float *)rcoeffs, (float *)rcoeffs);
    }
    else
    {
        for (n = 0; n < len; n++)
        {
            ((double *)rcoeffs)[n_fft2 + n] = ((double *)coeffs)[n] * scale;

            if (!_finite(((double *)rcoeffs)[n_fft2 + n]))
            {
                pinfo("NaN or Inf value among coefficients.\n");
                return NULL;
            }
        }

        fftw_execute_r2r((const fftw_plan)fftplans_inplace[fft_order],
                         (double *)rcoeffs, (double *)rcoeffs);
    }

    scale = 1.0 / (double)n_fft;

    if (optional_dest != NULL)
    {
        coeffs_data = optional_dest;
    }
    else
    {
        coeffs_data = _aligned_malloc(n_fft * realsize, ALIGNMENT);
    }

    convolver_mixnscale(&rcoeffs, coeffs_data, &scale, 1,
                        CONVOLVER_MIXMODE_INPUT);

    _aligned_free(rcoeffs);

    return coeffs_data;
}

void
fftw_convolver::convolver_runtime_coeffs2cbuf(void *src,  // nfft / 2
                                              void *dest) // nfft
{
    static void *tmp = NULL;
    double scale;

    if (tmp == NULL)
    {
        tmp = _aligned_malloc(n_fft * realsize, ALIGNMENT);
    }

    memset(dest, 0, n_fft2 * realsize);
    memcpy(&((uint8_t *)dest)[n_fft2 * realsize], src, n_fft2 * realsize);

    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)fftplans[fft_order],
                          (float *)dest, (float *)tmp);
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)fftplans[fft_order],
                         (double *)dest, (double *)tmp);
    }

    scale = 1.0 / (double)n_fft;
    convolver_mixnscale(&tmp, dest, &scale, 1, CONVOLVER_MIXMODE_INPUT);
}

bool
fftw_convolver::convolver_verify_cbuf(void *cbufs[],
                                      int n_cbufs)
{
    int n, i;

    for (n = 0; n < n_cbufs; n++)
    {
        if (realsize == 4)
        {
            for (i = 0; i < n_fft; i++)
            {
                if (!_finite((double)((float *)cbufs[n])[i]))
                {
                    pinfo("NaN or Inf value among coefficients.\n");
                    return false;
                }
            }
        }
        else
        {
            for (i = 0; i < n_fft; i++)
            {
                if (!_finite(((double *)cbufs[n])[i]))
                {
                    pinfo("NaN or Inf value among coefficients.\n");
                    return false;
                }
            }
        }
    }

    return true;
}

void
fftw_convolver::convolver_debug_dump_cbuf(const char filename[],
                                          void *cbufs[],
                                          int n_cbufs)
{
    void *coeffs;
    double scale;
    FILE *stream;
    int n, i;
    errno_t err;

    if ((err = fopen_s(&stream, filename, "wt+")) != 0)
    {
        pinfo("Could not open \"%s\" for writing: %s", filename,
              strerror(errno));
        return;
    }

    coeffs = _aligned_malloc(n_fft * realsize, ALIGNMENT);
    scale = 1.0;

    for (n = 0; n < n_cbufs; n++)
    {
        convolver_mixnscale(&cbufs[n], coeffs, &scale, 1,
                            CONVOLVER_MIXMODE_OUTPUT);
        if (realsize == 4)
        {
            fftwf_execute_r2r((const fftwf_plan)ifftplans_inplace[fft_order],
                              (float *)coeffs, (float *)coeffs);
            for (i = 0; i < n_fft2; i++)
            {
                fprintf(stream, "%.16e\n", ((float *)coeffs)[n_fft2 + i]);
            }
        }
        else
        {
            fftw_execute_r2r((const fftw_plan)ifftplans_inplace[fft_order],
                             (double *)coeffs, (double *)coeffs);
            for (i = 0; i < n_fft2; i++)
            {
                fprintf(stream, "%.16e\n", ((double *)coeffs)[n_fft2 + i]);
            }
        }
    }

    _aligned_free(coeffs);
    fclose(stream);
}

void *
fftw_convolver::create_fft_plan(int order,
                                int invert,
                                int inplace)
{
    invert = !!invert;
    inplace = !!inplace;

    if (!bit_isset(&fftplan_generated[invert][inplace], order))
    {
        pinfo("Creating %s%sFFTW plan of size %d using wisdom.",
              invert ? "inverse " : "forward ",
              inplace ? "in place " : "",
              1 << order);

        fftplan_table[invert][inplace][order] =
            get_fft_plan(1 << order, inplace, invert);

        bit_set(&fftplan_generated[invert][inplace], order);
    }

    return fftplan_table[invert][inplace][order];
}

void
fftw_convolver::destroy_fft_plan(int order,
                                 int invert,
                                 int inplace)
{
    if (bit_isset(&fftplan_generated[invert][inplace], order))
    {
        if (realsize == 4)
        {
            fftwf_destroy_plan((fftwf_plan)fftplan_table[invert][inplace][order]);
        }
        else
        {
            fftw_destroy_plan((fftw_plan)fftplan_table[invert][inplace][order]);
        }

        bit_clr(&fftplan_generated[invert][inplace], order);
    }
}

int
fftw_convolver::convolver_td_block_length(int n_coeffs)
{
    if (n_coeffs < 1)
    {
        return -1;
    }

    return 1 << log2_roof(n_coeffs);
}

td_conv_t *
fftw_convolver::convolver_td_new(void *coeffs,
                                 int n_coeffs)
{
    int blocklen, n;
    td_conv_t *tdc;
    double scaled;
    float scalef;

    if ((blocklen = convolver_td_block_length(n_coeffs)) == -1)
    {
        return NULL;
    }

    tdc = (td_conv_t *) malloc(sizeof(td_conv_t));

    memset(tdc, 0, sizeof(td_conv_t));

    tdc->fftplan = create_fft_plan(log2_get(blocklen) + 1, 0, 1);
    tdc->ifftplan = create_fft_plan(log2_get(blocklen) + 1, 1, 1);
    tdc->coeffs = _aligned_malloc(2 * blocklen * realsize, ALIGNMENT);
    tdc->blocklen = blocklen;

    memset(tdc->coeffs, 0, blocklen * realsize);
    memset(&((uint8_t *)tdc->coeffs)[(blocklen + n_coeffs) * realsize], 0,
           (blocklen - n_coeffs) * realsize);
    memcpy(&((uint8_t *)tdc->coeffs)[blocklen * realsize], coeffs,
           n_coeffs * realsize);

    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)tdc->fftplan, (float *)tdc->coeffs, (float *)tdc->coeffs);
        scalef = 1.0 / (float)(blocklen << 1);
        for (n = 0; n < blocklen << 1; n++)
        {
            ((float *)tdc->coeffs)[n] *= scalef;
        }
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)tdc->fftplan, (double *)tdc->coeffs, (double *)tdc->coeffs);
        scaled = 1.0 / (double)(blocklen << 1);
        for (n = 0; n < blocklen << 1; n++)
        {
            ((double *)tdc->coeffs)[n] *= scaled;
        }
    }

    return tdc;
}

void
fftw_convolver::convolver_td_convolve(td_conv_t *tdc,
                                      void *overlap_block)
{
    if (realsize == 4)
    {
        fftwf_execute_r2r((const fftwf_plan)tdc->fftplan, (float *)overlap_block, (float *)overlap_block);
        convolve_inplace_ordered(overlap_block, tdc->coeffs,
                                 tdc->blocklen << 1);
        fftwf_execute_r2r((const fftwf_plan)tdc->ifftplan, (float *)overlap_block, (float *)overlap_block);
    }
    else
    {
        fftw_execute_r2r((const fftw_plan)tdc->fftplan, (double *)overlap_block, (double *)overlap_block);
        convolve_inplace_ordered(overlap_block, tdc->coeffs,
                                 tdc->blocklen << 1);
        fftw_execute_r2r((const fftw_plan)tdc->ifftplan, (double *)overlap_block, (double *)overlap_block);
    }
}

void *
fftw_convolver::get_fft_plan(int length,
                             int inplace,
                             int invert)
{
    void *plan, *buf[2];

    buf[0] = _aligned_malloc(length * realsize, ALIGNMENT);
    memset(buf[0], 0, length * realsize);
    buf[1] = buf[0];

    if (inplace == 0)
    {
        buf[1] = _aligned_malloc(length * realsize, ALIGNMENT);
        memset(buf[1], 0, length * realsize);
    }

    if (realsize == 4)
    {
        plan = fftwf_plan_r2r_1d(length, (float *)buf[0], (float *)buf[1],
                                 (invert != 0) ? FFTW_HC2R : FFTW_R2HC,
                                 FFTW_MEASURE);
    }
    else
    {
        plan = fftw_plan_r2r_1d(length, (double *)buf[0], (double *)buf[1],
                                (invert != 0) ? FFTW_HC2R : FFTW_R2HC,
                                FFTW_MEASURE);
    }

    _aligned_free(buf[0]);

    if (inplace == 0)
    {
        _aligned_free(buf[1]);
    }

    return plan;
}

void
fftw_convolver::convolve_inplace_ordered(void *cbuf,
                                         void *coeffs,
                                         int size)
{
    int n, size2 = size >> 1;

    if (realsize == 4)
    {
        float a, *b = (float *)cbuf, *c = (float *)coeffs;

        b[0] *= c[0];

        for (n = 1; n < size2; n++)
        {
            a = b[n];
            b[n] = a * c[n] - b[size - n] * c[size - n];
            b[size - n] = a * c[size - n] + b[size - n] * c[n];
        }

        b[size2] *= c[size2];
    }
    else
    {
        double a, *b = (double *)cbuf, *c = (double *)coeffs;

        b[0] *= c[0];

        for (n = 1; n < size2; n++)
        {
            a = b[n];
            b[n] = a * c[n] - b[size - n] * c[size - n];
            b[size - n] = a * c[size - n] + b[size - n] * c[n];
        }

        b[size2] *= c[size2];
    }
}

void
fftw_convolver::mixnscalef(void *input_cbufs[],
                           void *output_cbuf,
                           double scales[],
                           int n_bufs,
                           int mixmode)
{
    int n, i;
    float *sscales;
    float **ibufs = (float **)input_cbufs;
    float *obuf = (float *)output_cbuf;

    // this implements float sscales[n_bufs]
    sscales = (float *) _alloca(n_bufs * sizeof(float));

    for (n = 0; n < n_bufs; n++)
    {
        sscales[n] = (float)scales[n];
    }

    switch (mixmode)
    {
    case CONVOLVER_MIXMODE_INPUT:
        switch (n_bufs)
        {
        case 1:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] = ibufs[0][n+0] * sscales[0];
                obuf[(n<<1)+1] = ibufs[0][n+1] * sscales[0];
                obuf[(n<<1)+2] = ibufs[0][n+2] * sscales[0];
                obuf[(n<<1)+3] = ibufs[0][n+3] * sscales[0];
            }

            obuf[4] = ibufs[0][n_fft >> 1] * sscales[0];
            ibufs[0] = &ibufs[0][n_fft];
            obuf[5] = ibufs[0][-1] * sscales[0];
            obuf[6] = ibufs[0][-2] * sscales[0];
            obuf[7] = ibufs[0][-3] * sscales[0];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] = ibufs[0][-n-0] * sscales[0];
                obuf[(n<<1)+5] = ibufs[0][-n-1] * sscales[0];
                obuf[(n<<1)+6] = ibufs[0][-n-2] * sscales[0];
                obuf[(n<<1)+7] = ibufs[0][-n-3] * sscales[0];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            break;
        case 2:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            break;
        case 3:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1] +
                    ibufs[2][n+0] * sscales[2];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1] +
                    ibufs[2][n+1] * sscales[2];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1] +
                    ibufs[2][n+2] * sscales[2];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1] +
                    ibufs[2][n+3] * sscales[2];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1] +
                ibufs[2][n_fft >> 1] * sscales[2];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            ibufs[2] = &ibufs[2][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1] +
                ibufs[2][-1] * sscales[2];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1] +
                ibufs[2][-2] * sscales[2];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1] +
                ibufs[2][-3] * sscales[2];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1] +
                    ibufs[2][-n-0] * sscales[2];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1] +
                    ibufs[2][-n-1] * sscales[2];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1] +
                    ibufs[2][-n-2] * sscales[2];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1] +
                    ibufs[2][-n-3] * sscales[2];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            ibufs[2] = &ibufs[2][-n_fft];
            break;
        case 4:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1] +
                    ibufs[2][n+0] * sscales[2] +
                    ibufs[3][n+0] * sscales[3];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1] +
                    ibufs[2][n+1] * sscales[2] +
                    ibufs[3][n+1] * sscales[3];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1] +
                    ibufs[2][n+2] * sscales[2] +
                    ibufs[3][n+2] * sscales[3];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1] +
                    ibufs[2][n+3] * sscales[2] +
                    ibufs[3][n+3] * sscales[3];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1] +
                ibufs[2][n_fft >> 1] * sscales[2] +
                ibufs[3][n_fft >> 1] * sscales[3];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            ibufs[2] = &ibufs[2][n_fft];
            ibufs[3] = &ibufs[3][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1] +
                ibufs[2][-1] * sscales[2] +
                ibufs[3][-1] * sscales[3];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1] +
                ibufs[2][-2] * sscales[2] +
                ibufs[3][-2] * sscales[3];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1] +
                ibufs[2][-3] * sscales[2] +
                ibufs[3][-3] * sscales[3];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1] +
                    ibufs[2][-n-0] * sscales[2] +
                    ibufs[3][-n-0] * sscales[3];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1] +
                    ibufs[2][-n-1] * sscales[2] +
                    ibufs[3][-n-1] * sscales[3];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1] +
                    ibufs[2][-n-2] * sscales[2] +
                    ibufs[3][-n-2] * sscales[3];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1] +
                    ibufs[2][-n-3] * sscales[2] +
                    ibufs[3][-n-3] * sscales[3];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            ibufs[2] = &ibufs[2][-n_fft];
            ibufs[3] = &ibufs[3][-n_fft];
            break;
        default:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] = ibufs[0][n+0] * sscales[0];
                obuf[(n<<1)+1] = ibufs[0][n+1] * sscales[0];
                obuf[(n<<1)+2] = ibufs[0][n+2] * sscales[0];
                obuf[(n<<1)+3] = ibufs[0][n+3] * sscales[0];

                for (i = 1; i < n_bufs; i++)
                {
                    obuf[(n<<1)+0] += ibufs[i][n+0] * sscales[i];
                    obuf[(n<<1)+1] += ibufs[i][n+1] * sscales[i];
                    obuf[(n<<1)+2] += ibufs[i][n+2] * sscales[i];
                    obuf[(n<<1)+3] += ibufs[i][n+3] * sscales[i];
                }
            }

            obuf[4] = ibufs[0][n_fft >> 1] * sscales[0];
            ibufs[0] = &ibufs[0][n_fft];
            obuf[5] = ibufs[0][-1] * sscales[0];
            obuf[6] = ibufs[0][-2] * sscales[0];
            obuf[7] = ibufs[0][-3] * sscales[0];

            for (i = 1; i < n_bufs; i++)
            {
                obuf[4] += ibufs[i][n_fft >> 1] * sscales[i];
                ibufs[i] = &ibufs[i][n_fft];
                obuf[5] += ibufs[i][-1] * sscales[i];
                obuf[6] += ibufs[i][-2] * sscales[i];
                obuf[7] += ibufs[i][-3] * sscales[i];
            }

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] = ibufs[0][-n-0] * sscales[0];
                obuf[(n<<1)+5] = ibufs[0][-n-1] * sscales[0];
                obuf[(n<<1)+6] = ibufs[0][-n-2] * sscales[0];
                obuf[(n<<1)+7] = ibufs[0][-n-3] * sscales[0];
                for (i = 1; i < n_bufs; i++)
                {
                    obuf[(n<<1)+4] += ibufs[i][-n-0] * sscales[i];
                    obuf[(n<<1)+5] += ibufs[i][-n-1] * sscales[i];
                    obuf[(n<<1)+6] += ibufs[i][-n-2] * sscales[i];
                    obuf[(n<<1)+7] += ibufs[i][-n-3] * sscales[i];
                }
            }

            for (i = 0; i < n_bufs; i++)
            {
                ibufs[i] = &ibufs[i][-n_fft];
            }

            break;
        }

        break;

    case CONVOLVER_MIXMODE_OUTPUT:
        switch (n_bufs)
        {
        case 1:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] = ibufs[0][(n<<1)+0] * sscales[0];
                obuf[n+1] = ibufs[0][(n<<1)+1] * sscales[0];
                obuf[n+2] = ibufs[0][(n<<1)+2] * sscales[0];
                obuf[n+3] = ibufs[0][(n<<1)+3] * sscales[0];
            }

            obuf[n_fft >> 1] = ibufs[0][4] * sscales[0];
            obuf = &obuf[n_fft];
            obuf[-1] = ibufs[0][5] * sscales[0];
            obuf[-2] = ibufs[0][6] * sscales[0];
            obuf[-3] = ibufs[0][7] * sscales[0];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] = ibufs[0][(n<<1)+4] * sscales[0];
                obuf[-n-1] = ibufs[0][(n<<1)+5] * sscales[0];
                obuf[-n-2] = ibufs[0][(n<<1)+6] * sscales[0];
                obuf[-n-3] = ibufs[0][(n<<1)+7] * sscales[0];
            }

            break;
        case 2:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1];
            }

            break;
        case 3:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1] +
                    ibufs[2][(n<<1)+0] * sscales[2];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1] +
                    ibufs[2][(n<<1)+1] * sscales[2];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1] +
                    ibufs[2][(n<<1)+2] * sscales[2];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1] +
                    ibufs[2][(n<<1)+3] * sscales[2];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1] +
                ibufs[2][4] * sscales[2];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1] +
                ibufs[2][5] * sscales[2];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1] +
                ibufs[2][6] * sscales[2];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1] +
                ibufs[2][7] * sscales[2];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1] +
                    ibufs[2][(n<<1)+4] * sscales[2];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1] +
                    ibufs[2][(n<<1)+5] * sscales[2];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1] +
                    ibufs[2][(n<<1)+6] * sscales[2];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1] +
                    ibufs[2][(n<<1)+7] * sscales[2];
            }

            break;
        case 4:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1] +
                    ibufs[2][(n<<1)+0] * sscales[2] +
                    ibufs[3][(n<<1)+0] * sscales[3];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1] +
                    ibufs[2][(n<<1)+1] * sscales[2] +
                    ibufs[3][(n<<1)+1] * sscales[3];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1] +
                    ibufs[2][(n<<1)+2] * sscales[2] +
                    ibufs[3][(n<<1)+2] * sscales[3];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1] +
                    ibufs[2][(n<<1)+3] * sscales[2] +
                    ibufs[3][(n<<1)+3] * sscales[3];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1] +
                ibufs[2][4] * sscales[2] +
                ibufs[3][4] * sscales[3];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1] +
                ibufs[2][5] * sscales[2] +
                ibufs[3][5] * sscales[3];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1] +
                ibufs[2][6] * sscales[2] +
                ibufs[3][6] * sscales[3];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1] +
                ibufs[2][7] * sscales[2] +
                ibufs[3][7] * sscales[3];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1] +
                    ibufs[2][(n<<1)+4] * sscales[2] +
                    ibufs[3][(n<<1)+4] * sscales[3];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1] +
                    ibufs[2][(n<<1)+5] * sscales[2] +
                    ibufs[3][(n<<1)+5] * sscales[3];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1] +
                    ibufs[2][(n<<1)+6] * sscales[2] +
                    ibufs[3][(n<<1)+6] * sscales[3];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1] +
                    ibufs[2][(n<<1)+7] * sscales[2] +
                    ibufs[3][(n<<1)+7] * sscales[3];
            }

            break;
        default:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] = ibufs[0][(n<<1)+0] * sscales[0];
                obuf[n+1] = ibufs[0][(n<<1)+1] * sscales[0];
                obuf[n+2] = ibufs[0][(n<<1)+2] * sscales[0];
                obuf[n+3] = ibufs[0][(n<<1)+3] * sscales[0];

                for (i = 1; i < n_bufs; i++)
                {
                    obuf[n+0] += ibufs[i][(n<<1)+0] * sscales[i];
                    obuf[n+1] += ibufs[i][(n<<1)+1] * sscales[i];
                    obuf[n+2] += ibufs[i][(n<<1)+2] * sscales[i];
                    obuf[n+3] += ibufs[i][(n<<1)+3] * sscales[i];
                }
            }

            obuf[n_fft >> 1] = ibufs[0][4] * sscales[0];
            for (i = 1; i < n_bufs; i++)
            {
                obuf[n_fft >> 1] += ibufs[i][4] * sscales[i];
            }

            obuf = &obuf[n_fft];
            obuf[-1] = ibufs[0][5] * sscales[0];
            obuf[-2] = ibufs[0][6] * sscales[0];
            obuf[-3] = ibufs[0][7] * sscales[0];

            for (i = 1; i < n_bufs; i++)
            {
                obuf[-1] += ibufs[i][5] * sscales[i];
                obuf[-2] += ibufs[i][6] * sscales[i];
                obuf[-3] += ibufs[i][7] * sscales[i];
            }

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] = ibufs[0][(n<<1)+4] * sscales[0];
                obuf[-n-1] = ibufs[0][(n<<1)+5] * sscales[0];
                obuf[-n-2] = ibufs[0][(n<<1)+6] * sscales[0];
                obuf[-n-3] = ibufs[0][(n<<1)+7] * sscales[0];

                for (i = 1; i < n_bufs; i++)
                {
                    obuf[-n-0] += ibufs[i][(n<<1)+4] * sscales[i];
                    obuf[-n-1] += ibufs[i][(n<<1)+5] * sscales[i];
                    obuf[-n-2] += ibufs[i][(n<<1)+6] * sscales[i];
                    obuf[-n-3] += ibufs[i][(n<<1)+7] * sscales[i];
                }
            }

            break;
        }

        break;

    default:
        pinfo("Invalid mixmode: %d.\n", mixmode);
        break;
    }
}

void
fftw_convolver::convolve_inplacef(void *cbuf,
                                  void *coeffs)
{
    int n;
    float a[4];
    float *b = (float *)cbuf;
    float *c = (float *)coeffs;
    float d1s, d2s;

    d1s = b[0] * c[0];
    d2s = b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        a[0] = b[n+0];
        a[1] = b[n+1];
        a[2] = b[n+2];
        a[3] = b[n+3];

        b[n+0] = a[0] * c[n+0] - b[n+4] * c[n+4];
        b[n+1] = a[1] * c[n+1] - b[n+5] * c[n+5];
        b[n+2] = a[2] * c[n+2] - b[n+6] * c[n+6];
        b[n+3] = a[3] * c[n+3] - b[n+7] * c[n+7];

        b[n+4] = a[0] * c[n+4] + b[n+4] * c[n+0];
        b[n+5] = a[1] * c[n+5] + b[n+5] * c[n+1];
        b[n+6] = a[2] * c[n+6] + b[n+6] * c[n+2];
        b[n+7] = a[3] * c[n+7] + b[n+7] * c[n+3];
    }

    b[0] = d1s;
    b[4] = d2s;
}

void
fftw_convolver::convolvef(void *input_cbuf,
                          void *coeffs,
                          void *output_cbuf)
{
    int n;
    float *b = (float *)input_cbuf;
    float *c = (float *)coeffs;
    float *d = (float *)output_cbuf;
    float d1s, d2s;

    d1s = b[0] * c[0];
    d2s = b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        d[n+0] = b[n+0] * c[n+0] - b[n+4] * c[n+4];
        d[n+1] = b[n+1] * c[n+1] - b[n+5] * c[n+5];
        d[n+2] = b[n+2] * c[n+2] - b[n+6] * c[n+6];
        d[n+3] = b[n+3] * c[n+3] - b[n+7] * c[n+7];

        d[n+4] = b[n+0] * c[n+4] + b[n+4] * c[n+0];
        d[n+5] = b[n+1] * c[n+5] + b[n+5] * c[n+1];
        d[n+6] = b[n+2] * c[n+6] + b[n+6] * c[n+2];
        d[n+7] = b[n+3] * c[n+7] + b[n+7] * c[n+3];
    }

    d[0] = d1s;
    d[4] = d2s;
}


void
fftw_convolver::convolve_addf(void *input_cbuf,
                              void *coeffs,
                              void *output_cbuf)
{
    float *b = (float *)input_cbuf;
    float *c = (float *)coeffs;
    float *d = (float *)output_cbuf;
    float d1s, d2s;
    int n;

    d1s = d[0] + b[0] * c[0];
    d2s = d[4] + b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        d[n+0] += b[n+0] * c[n+0] - b[n+4] * c[n+4];
        d[n+1] += b[n+1] * c[n+1] - b[n+5] * c[n+5];
        d[n+2] += b[n+2] * c[n+2] - b[n+6] * c[n+6];
        d[n+3] += b[n+3] * c[n+3] - b[n+7] * c[n+7];

        d[n+4] += b[n+0] * c[n+4] + b[n+4] * c[n+0];
        d[n+5] += b[n+1] * c[n+5] + b[n+5] * c[n+1];
        d[n+6] += b[n+2] * c[n+6] + b[n+6] * c[n+2];
        d[n+7] += b[n+3] * c[n+7] + b[n+7] * c[n+3];
    }

    d[0] = d1s;
    d[4] = d2s;
}

void
fftw_convolver::dirac_convolve_inplacef(void *cbuf)
{
    float fraction = 1.0 / (float)n_fft;
    int n;

    for (n = 0; n < n_fft; n += 4)
    {
        ((float *)cbuf)[n+0] *= +fraction;
        ((float *)cbuf)[n+1] *= -fraction;
        ((float *)cbuf)[n+2] *= +fraction;
        ((float *)cbuf)[n+3] *= -fraction;
    }
}

void
fftw_convolver::dirac_convolvef(void *input_cbuf,
                                void *output_cbuf)
{
    float fraction = 1.0 / (float)n_fft;
    int n;

    for (n = 0; n < n_fft; n += 4)
    {
        ((float *)output_cbuf)[n+0] = ((float *)input_cbuf)[n+0] * +fraction;
        ((float *)output_cbuf)[n+1] = ((float *)input_cbuf)[n+1] * -fraction;
        ((float *)output_cbuf)[n+2] = ((float *)input_cbuf)[n+2] * +fraction;
        ((float *)output_cbuf)[n+3] = ((float *)input_cbuf)[n+3] * -fraction;
    }
}

void
fftw_convolver::mixnscaled(void *input_cbufs[],
                           void *output_cbuf,
                           double scales[],
                           int n_bufs,
                           int mixmode)
{
    int n, i;
    double *sscales;
    double **ibufs = (double **)input_cbufs;
    double *obuf = (double *)output_cbuf;

    // this implements double sscales[n_bufs]
    sscales = (double *) _alloca(n_bufs * sizeof(double));

    for (n = 0; n < n_bufs; n++)
    {
        sscales[n] = (double)scales[n];
    }

    switch (mixmode)
    {
    case CONVOLVER_MIXMODE_INPUT:
        switch (n_bufs)
        {
        case 1:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] = ibufs[0][n+0] * sscales[0];
                obuf[(n<<1)+1] = ibufs[0][n+1] * sscales[0];
                obuf[(n<<1)+2] = ibufs[0][n+2] * sscales[0];
                obuf[(n<<1)+3] = ibufs[0][n+3] * sscales[0];
            }

            obuf[4] = ibufs[0][n_fft >> 1] * sscales[0];
            ibufs[0] = &ibufs[0][n_fft];
            obuf[5] = ibufs[0][-1] * sscales[0];
            obuf[6] = ibufs[0][-2] * sscales[0];
            obuf[7] = ibufs[0][-3] * sscales[0];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] = ibufs[0][-n-0] * sscales[0];
                obuf[(n<<1)+5] = ibufs[0][-n-1] * sscales[0];
                obuf[(n<<1)+6] = ibufs[0][-n-2] * sscales[0];
                obuf[(n<<1)+7] = ibufs[0][-n-3] * sscales[0];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            break;
        case 2:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            break;
        case 3:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1] +
                    ibufs[2][n+0] * sscales[2];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1] +
                    ibufs[2][n+1] * sscales[2];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1] +
                    ibufs[2][n+2] * sscales[2];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1] +
                    ibufs[2][n+3] * sscales[2];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1] +
                ibufs[2][n_fft >> 1] * sscales[2];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            ibufs[2] = &ibufs[2][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1] +
                ibufs[2][-1] * sscales[2];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1] +
                ibufs[2][-2] * sscales[2];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1] +
                ibufs[2][-3] * sscales[2];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1] +
                    ibufs[2][-n-0] * sscales[2];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1] +
                    ibufs[2][-n-1] * sscales[2];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1] +
                    ibufs[2][-n-2] * sscales[2];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1] +
                    ibufs[2][-n-3] * sscales[2];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            ibufs[2] = &ibufs[2][-n_fft];
            break;
        case 4:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] =
                    ibufs[0][n+0] * sscales[0] +
                    ibufs[1][n+0] * sscales[1] +
                    ibufs[2][n+0] * sscales[2] +
                    ibufs[3][n+0] * sscales[3];
                obuf[(n<<1)+1] =
                    ibufs[0][n+1] * sscales[0] +
                    ibufs[1][n+1] * sscales[1] +
                    ibufs[2][n+1] * sscales[2] +
                    ibufs[3][n+1] * sscales[3];
                obuf[(n<<1)+2] =
                    ibufs[0][n+2] * sscales[0] +
                    ibufs[1][n+2] * sscales[1] +
                    ibufs[2][n+2] * sscales[2] +
                    ibufs[3][n+2] * sscales[3];
                obuf[(n<<1)+3] =
                    ibufs[0][n+3] * sscales[0] +
                    ibufs[1][n+3] * sscales[1] +
                    ibufs[2][n+3] * sscales[2] +
                    ibufs[3][n+3] * sscales[3];
            }

            obuf[4] =
                ibufs[0][n_fft >> 1] * sscales[0] +
                ibufs[1][n_fft >> 1] * sscales[1] +
                ibufs[2][n_fft >> 1] * sscales[2] +
                ibufs[3][n_fft >> 1] * sscales[3];
            ibufs[0] = &ibufs[0][n_fft];
            ibufs[1] = &ibufs[1][n_fft];
            ibufs[2] = &ibufs[2][n_fft];
            ibufs[3] = &ibufs[3][n_fft];
            obuf[5] =
                ibufs[0][-1] * sscales[0] +
                ibufs[1][-1] * sscales[1] +
                ibufs[2][-1] * sscales[2] +
                ibufs[3][-1] * sscales[3];
            obuf[6] =
                ibufs[0][-2] * sscales[0] +
                ibufs[1][-2] * sscales[1] +
                ibufs[2][-2] * sscales[2] +
                ibufs[3][-2] * sscales[3];
            obuf[7] =
                ibufs[0][-3] * sscales[0] +
                ibufs[1][-3] * sscales[1] +
                ibufs[2][-3] * sscales[2] +
                ibufs[3][-3] * sscales[3];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] =
                    ibufs[0][-n-0] * sscales[0] +
                    ibufs[1][-n-0] * sscales[1] +
                    ibufs[2][-n-0] * sscales[2] +
                    ibufs[3][-n-0] * sscales[3];
                obuf[(n<<1)+5] =
                    ibufs[0][-n-1] * sscales[0] +
                    ibufs[1][-n-1] * sscales[1] +
                    ibufs[2][-n-1] * sscales[2] +
                    ibufs[3][-n-1] * sscales[3];
                obuf[(n<<1)+6] =
                    ibufs[0][-n-2] * sscales[0] +
                    ibufs[1][-n-2] * sscales[1] +
                    ibufs[2][-n-2] * sscales[2] +
                    ibufs[3][-n-2] * sscales[3];
                obuf[(n<<1)+7] =
                    ibufs[0][-n-3] * sscales[0] +
                    ibufs[1][-n-3] * sscales[1] +
                    ibufs[2][-n-3] * sscales[2] +
                    ibufs[3][-n-3] * sscales[3];
            }

            ibufs[0] = &ibufs[0][-n_fft];
            ibufs[1] = &ibufs[1][-n_fft];
            ibufs[2] = &ibufs[2][-n_fft];
            ibufs[3] = &ibufs[3][-n_fft];
            break;
        default:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+0] = ibufs[0][n+0] * sscales[0];
                obuf[(n<<1)+1] = ibufs[0][n+1] * sscales[0];
                obuf[(n<<1)+2] = ibufs[0][n+2] * sscales[0];
                obuf[(n<<1)+3] = ibufs[0][n+3] * sscales[0];
                for (i = 1; i < n_bufs; i++)
                {
                    obuf[(n<<1)+0] += ibufs[i][n+0] * sscales[i];
                    obuf[(n<<1)+1] += ibufs[i][n+1] * sscales[i];
                    obuf[(n<<1)+2] += ibufs[i][n+2] * sscales[i];
                    obuf[(n<<1)+3] += ibufs[i][n+3] * sscales[i];
                }
            }

            obuf[4] = ibufs[0][n_fft >> 1] * sscales[0];
            ibufs[0] = &ibufs[0][n_fft];
            obuf[5] = ibufs[0][-1] * sscales[0];
            obuf[6] = ibufs[0][-2] * sscales[0];
            obuf[7] = ibufs[0][-3] * sscales[0];

            for (i = 1; i < n_bufs; i++)
            {
                obuf[4] += ibufs[i][n_fft >> 1] * sscales[i];
                ibufs[i] = &ibufs[i][n_fft];
                obuf[5] += ibufs[i][-1] * sscales[i];
                obuf[6] += ibufs[i][-2] * sscales[i];
                obuf[7] += ibufs[i][-3] * sscales[i];
            }

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[(n<<1)+4] = ibufs[0][-n-0] * sscales[0];
                obuf[(n<<1)+5] = ibufs[0][-n-1] * sscales[0];
                obuf[(n<<1)+6] = ibufs[0][-n-2] * sscales[0];
                obuf[(n<<1)+7] = ibufs[0][-n-3] * sscales[0];
                for (i = 1; i < n_bufs; i++)
                {
                    obuf[(n<<1)+4] += ibufs[i][-n-0] * sscales[i];
                    obuf[(n<<1)+5] += ibufs[i][-n-1] * sscales[i];
                    obuf[(n<<1)+6] += ibufs[i][-n-2] * sscales[i];
                    obuf[(n<<1)+7] += ibufs[i][-n-3] * sscales[i];
                }
            }

            for (i = 0; i < n_bufs; i++)
            {
                ibufs[i] = &ibufs[i][-n_fft];
            }
            break;
        }
        break;

    case CONVOLVER_MIXMODE_OUTPUT:
        switch (n_bufs)
        {
        case 1:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] = ibufs[0][(n<<1)+0] * sscales[0];
                obuf[n+1] = ibufs[0][(n<<1)+1] * sscales[0];
                obuf[n+2] = ibufs[0][(n<<1)+2] * sscales[0];
                obuf[n+3] = ibufs[0][(n<<1)+3] * sscales[0];
            }

            obuf[n_fft >> 1] = ibufs[0][4] * sscales[0];
            obuf = &obuf[n_fft];
            obuf[-1] = ibufs[0][5] * sscales[0];
            obuf[-2] = ibufs[0][6] * sscales[0];
            obuf[-3] = ibufs[0][7] * sscales[0];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] = ibufs[0][(n<<1)+4] * sscales[0];
                obuf[-n-1] = ibufs[0][(n<<1)+5] * sscales[0];
                obuf[-n-2] = ibufs[0][(n<<1)+6] * sscales[0];
                obuf[-n-3] = ibufs[0][(n<<1)+7] * sscales[0];
            }

            break;
        case 2:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1];
            }

            break;
        case 3:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1] +
                    ibufs[2][(n<<1)+0] * sscales[2];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1] +
                    ibufs[2][(n<<1)+1] * sscales[2];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1] +
                    ibufs[2][(n<<1)+2] * sscales[2];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1] +
                    ibufs[2][(n<<1)+3] * sscales[2];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1] +
                ibufs[2][4] * sscales[2];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1] +
                ibufs[2][5] * sscales[2];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1] +
                ibufs[2][6] * sscales[2];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1] +
                ibufs[2][7] * sscales[2];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1] +
                    ibufs[2][(n<<1)+4] * sscales[2];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1] +
                    ibufs[2][(n<<1)+5] * sscales[2];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1] +
                    ibufs[2][(n<<1)+6] * sscales[2];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1] +
                    ibufs[2][(n<<1)+7] * sscales[2];
            }

            break;
        case 4:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] =
                    ibufs[0][(n<<1)+0] * sscales[0] +
                    ibufs[1][(n<<1)+0] * sscales[1] +
                    ibufs[2][(n<<1)+0] * sscales[2] +
                    ibufs[3][(n<<1)+0] * sscales[3];
                obuf[n+1] =
                    ibufs[0][(n<<1)+1] * sscales[0] +
                    ibufs[1][(n<<1)+1] * sscales[1] +
                    ibufs[2][(n<<1)+1] * sscales[2] +
                    ibufs[3][(n<<1)+1] * sscales[3];
                obuf[n+2] =
                    ibufs[0][(n<<1)+2] * sscales[0] +
                    ibufs[1][(n<<1)+2] * sscales[1] +
                    ibufs[2][(n<<1)+2] * sscales[2] +
                    ibufs[3][(n<<1)+2] * sscales[3];
                obuf[n+3] =
                    ibufs[0][(n<<1)+3] * sscales[0] +
                    ibufs[1][(n<<1)+3] * sscales[1] +
                    ibufs[2][(n<<1)+3] * sscales[2] +
                    ibufs[3][(n<<1)+3] * sscales[3];
            }

            obuf[n_fft >> 1] =
                ibufs[0][4] * sscales[0] +
                ibufs[1][4] * sscales[1] +
                ibufs[2][4] * sscales[2] +
                ibufs[3][4] * sscales[3];
            obuf = &obuf[n_fft];
            obuf[-1] =
                ibufs[0][5] * sscales[0] +
                ibufs[1][5] * sscales[1] +
                ibufs[2][5] * sscales[2] +
                ibufs[3][5] * sscales[3];
            obuf[-2] =
                ibufs[0][6] * sscales[0] +
                ibufs[1][6] * sscales[1] +
                ibufs[2][6] * sscales[2] +
                ibufs[3][6] * sscales[3];
            obuf[-3] =
                ibufs[0][7] * sscales[0] +
                ibufs[1][7] * sscales[1] +
                ibufs[2][7] * sscales[2] +
                ibufs[3][7] * sscales[3];

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] =
                    ibufs[0][(n<<1)+4] * sscales[0] +
                    ibufs[1][(n<<1)+4] * sscales[1] +
                    ibufs[2][(n<<1)+4] * sscales[2] +
                    ibufs[3][(n<<1)+4] * sscales[3];
                obuf[-n-1] =
                    ibufs[0][(n<<1)+5] * sscales[0] +
                    ibufs[1][(n<<1)+5] * sscales[1] +
                    ibufs[2][(n<<1)+5] * sscales[2] +
                    ibufs[3][(n<<1)+5] * sscales[3];
                obuf[-n-2] =
                    ibufs[0][(n<<1)+6] * sscales[0] +
                    ibufs[1][(n<<1)+6] * sscales[1] +
                    ibufs[2][(n<<1)+6] * sscales[2] +
                    ibufs[3][(n<<1)+6] * sscales[3];
                obuf[-n-3] =
                    ibufs[0][(n<<1)+7] * sscales[0] +
                    ibufs[1][(n<<1)+7] * sscales[1] +
                    ibufs[2][(n<<1)+7] * sscales[2] +
                    ibufs[3][(n<<1)+7] * sscales[3];
            }

            break;
        default:
            for (n = 0; n < n_fft >> 1; n += 4)
            {
                obuf[n+0] = ibufs[0][(n<<1)+0] * sscales[0];
                obuf[n+1] = ibufs[0][(n<<1)+1] * sscales[0];
                obuf[n+2] = ibufs[0][(n<<1)+2] * sscales[0];
                obuf[n+3] = ibufs[0][(n<<1)+3] * sscales[0];
                for (i = 1; i < n_bufs; i++)
                {
                    obuf[n+0] += ibufs[i][(n<<1)+0] * sscales[i];
                    obuf[n+1] += ibufs[i][(n<<1)+1] * sscales[i];
                    obuf[n+2] += ibufs[i][(n<<1)+2] * sscales[i];
                    obuf[n+3] += ibufs[i][(n<<1)+3] * sscales[i];
                }
            }

            obuf[n_fft >> 1] = ibufs[0][4] * sscales[0];
            for (i = 1; i < n_bufs; i++)
            {
                obuf[n_fft >> 1] += ibufs[i][4] * sscales[i];
            }

            obuf = &obuf[n_fft];
            obuf[-1] = ibufs[0][5] * sscales[0];
            obuf[-2] = ibufs[0][6] * sscales[0];
            obuf[-3] = ibufs[0][7] * sscales[0];

            for (i = 1; i < n_bufs; i++)
            {
                obuf[-1] += ibufs[i][5] * sscales[i];
                obuf[-2] += ibufs[i][6] * sscales[i];
                obuf[-3] += ibufs[i][7] * sscales[i];
            }

            for (n = 4; n < n_fft >> 1; n += 4)
            {
                obuf[-n-0] = ibufs[0][(n<<1)+4] * sscales[0];
                obuf[-n-1] = ibufs[0][(n<<1)+5] * sscales[0];
                obuf[-n-2] = ibufs[0][(n<<1)+6] * sscales[0];
                obuf[-n-3] = ibufs[0][(n<<1)+7] * sscales[0];

                for (i = 1; i < n_bufs; i++)
                {
                    obuf[-n-0] += ibufs[i][(n<<1)+4] * sscales[i];
                    obuf[-n-1] += ibufs[i][(n<<1)+5] * sscales[i];
                    obuf[-n-2] += ibufs[i][(n<<1)+6] * sscales[i];
                    obuf[-n-3] += ibufs[i][(n<<1)+7] * sscales[i];
                }
            }

            break;
        }

        break;

    default:
        pinfo("Invalid mixmode: %d.\n", mixmode);
        break;
    }
}

void
fftw_convolver::convolve_inplaced(void *cbuf,
                                  void *coeffs)
{
    int n;
    double a[4];
    double *b = (double *)cbuf;
    double *c = (double *)coeffs;
    double d1s, d2s;

    d1s = b[0] * c[0];
    d2s = b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        a[0] = b[n+0];
        a[1] = b[n+1];
        a[2] = b[n+2];
        a[3] = b[n+3];

        b[n+0] = a[0] * c[n+0] - b[n+4] * c[n+4];
        b[n+1] = a[1] * c[n+1] - b[n+5] * c[n+5];
        b[n+2] = a[2] * c[n+2] - b[n+6] * c[n+6];
        b[n+3] = a[3] * c[n+3] - b[n+7] * c[n+7];

        b[n+4] = a[0] * c[n+4] + b[n+4] * c[n+0];
        b[n+5] = a[1] * c[n+5] + b[n+5] * c[n+1];
        b[n+6] = a[2] * c[n+6] + b[n+6] * c[n+2];
        b[n+7] = a[3] * c[n+7] + b[n+7] * c[n+3];
    }

    b[0] = d1s;
    b[4] = d2s;
}

void
fftw_convolver::convolved(void *input_cbuf,
                          void *coeffs,
                          void *output_cbuf)
{
    int n;
    double *b = (double *)input_cbuf;
    double *c = (double *)coeffs;
    double *d = (double *)output_cbuf;
    double d1s, d2s;

    d1s = b[0] * c[0];
    d2s = b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        d[n+0] = b[n+0] * c[n+0] - b[n+4] * c[n+4];
        d[n+1] = b[n+1] * c[n+1] - b[n+5] * c[n+5];
        d[n+2] = b[n+2] * c[n+2] - b[n+6] * c[n+6];
        d[n+3] = b[n+3] * c[n+3] - b[n+7] * c[n+7];

        d[n+4] = b[n+0] * c[n+4] + b[n+4] * c[n+0];
        d[n+5] = b[n+1] * c[n+5] + b[n+5] * c[n+1];
        d[n+6] = b[n+2] * c[n+6] + b[n+6] * c[n+2];
        d[n+7] = b[n+3] * c[n+7] + b[n+7] * c[n+3];
    }

    d[0] = d1s;
    d[4] = d2s;
}

void
fftw_convolver::convolve_addd(void *input_cbuf,
                              void *coeffs,
                              void *output_cbuf)
{
    double *b = (double *)input_cbuf;
    double *c = (double *)coeffs;
    double *d = (double *)output_cbuf;
    double d1s, d2s;
    int n;

    d1s = d[0] + b[0] * c[0];
    d2s = d[4] + b[4] * c[4];

    for (n = 0; n < n_fft; n += 8)
    {
        d[n+0] += b[n+0] * c[n+0] - b[n+4] * c[n+4];
        d[n+1] += b[n+1] * c[n+1] - b[n+5] * c[n+5];
        d[n+2] += b[n+2] * c[n+2] - b[n+6] * c[n+6];
        d[n+3] += b[n+3] * c[n+3] - b[n+7] * c[n+7];

        d[n+4] += b[n+0] * c[n+4] + b[n+4] * c[n+0];
        d[n+5] += b[n+1] * c[n+5] + b[n+5] * c[n+1];
        d[n+6] += b[n+2] * c[n+6] + b[n+6] * c[n+2];
        d[n+7] += b[n+3] * c[n+7] + b[n+7] * c[n+3];
    }

    d[0] = d1s;
    d[4] = d2s;
}

void
fftw_convolver::dirac_convolve_inplaced(void *cbuf)
{
    double fraction = 1.0 / (double)n_fft;
    int n;

    for (n = 0; n < n_fft; n += 4)
    {
        ((double *)cbuf)[n+0] *= +fraction;
        ((double *)cbuf)[n+1] *= -fraction;
        ((double *)cbuf)[n+2] *= +fraction;
        ((double *)cbuf)[n+3] *= -fraction;
    }
}

void
fftw_convolver::dirac_convolved(void *input_cbuf,
                                void *output_cbuf)
{
    double fraction = 1.0 / (double)n_fft;
    int n;

    for (n = 0; n < n_fft; n += 4)
    {
        ((double *)output_cbuf)[n+0] = ((double *)input_cbuf)[n+0] * +fraction;
        ((double *)output_cbuf)[n+1] = ((double *)input_cbuf)[n+1] * -fraction;
        ((double *)output_cbuf)[n+2] = ((double *)input_cbuf)[n+2] * +fraction;
        ((double *)output_cbuf)[n+3] = ((double *)input_cbuf)[n+3] * -fraction;
    }
}
