/*
 * (c) Copyright 2001-2006 Anders Torger
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <malloc.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "coeff.hpp"
#include "fftw_convolver.hpp"
#include "raw2real.hpp"
#include "buffer.hpp"
#include "pinfo.h"

namespace coeff
{
    // Returns a coefficient set representing a dirac impulse.
    void **
    load_dirac_coeff(int n_channels,
                     int length,
                     int realsize)
    {
        int n;
        void **coeffs;

        coeffs = (void **)_aligned_malloc(n_channels * sizeof(void *), ALIGNMENT);
        
        for (n = 0; n < n_channels; n++)
        {
            // Create a dirac impulse
            coeffs[n] = _aligned_malloc(length * realsize, ALIGNMENT);
            memset(coeffs[n], 0, length * realsize);

            if (realsize == 4)
            {
                ((float *)coeffs[n])[0] = 1.0;
            }
            else
            {
                ((double *)coeffs[n])[0] = 1.0;
            }
        }

        return coeffs;
    }

    // Returns a coefficient set stored in the given floating point
    // text format file.
    void *
    load_text_coeff(const wchar_t *filename,
                    int *length,
                    int realsize,                                                                                                                                                                                                                           
                    int max_length)
    {
        char str[1024], *p, *s;
        void *realbuf = NULL;
        int capacity = 0;
        FILE *file;
        errno_t err;

        err = _wfopen_s(&file, filename, L"rt");
        if (err == 0)
        {
            *length = 0;
            str[1023] = '\0';
            realbuf = _aligned_malloc(1, ALIGNMENT);

            while (fgets(str, 1023, file) != NULL)
            {
                s = str;
                while (*s == ' ' || *s == '\t') s++;
                if (*s == '\n' || *s == '\0')
                {
                    continue;
                }

                if (*length == capacity)
                {
                    capacity += 1024;
                    realbuf = _aligned_realloc(realbuf, capacity * realsize, ALIGNMENT);
                }

                if (realsize == 4)
                {
                    ((float *)realbuf)[*length] = (float)strtod(s, &p);
                }
                else
                {
                    ((double *)realbuf)[*length] = strtod(s, &p);
                }

                (*length) += 1;

                if (p == s)
                {
                    pinfo("Parse error on line %d in file %s: invalid "
                          "floating point number.\n", *length, filename);

                    return NULL;
                }

                if (max_length > 0 && (*length) == max_length)
                {
                    break;
                }
            }

            fclose(file);
            realbuf = _aligned_realloc(realbuf, (*length) * realsize, ALIGNMENT);
        }
        else
        {
            pinfo("Error reading coefficients from file %s.", filename);
        }

        return realbuf;
    }

    // Returns a coefficient set stored in the given raw format binary file.
    void *
    load_raw_coeff(const wchar_t *filename,
                   int *length,
                   int realsize,
                   int max_length,                                                                                                                                                                                                                                                  
                   struct sample_format_t *sf)
    {
        int n_items;
        uint8_t *rawbuf = NULL;
        uint8_t *curp = NULL;
        void *realbuf;
        FILE *file;
        errno_t err;

        err = _wfopen_s(&file, filename, L"rb");
        if (err == 0)
        {
            *length = 0;
            rawbuf = (uint8_t *) _aligned_malloc(1024 * sf->bytes, ALIGNMENT);
            curp = rawbuf;

            while ((n_items = fread(curp, sf->bytes, 1024, file)) != 0)
            {
                *length += n_items;

                if (max_length > 0 && *length >= max_length)
                {
                    *length = max_length;
                    break;
                }

                rawbuf = (uint8_t *) _aligned_realloc(rawbuf, (*length + 1024) * sf->bytes, ALIGNMENT);
                curp = rawbuf + (*length) * sf->bytes;
            }

            fclose(file);

            rawbuf = (uint8_t *) _aligned_realloc(rawbuf, (*length) * sf->bytes, ALIGNMENT);

            if (sf->isfloat && !sf->swap && sf->bytes == realsize)
            {
                return (void *)rawbuf;
            }

            realbuf = _aligned_malloc((*length) * realsize, ALIGNMENT);

            if (realsize == 4)
            {
                raw2real::raw2realf(realbuf, rawbuf, sf->bytes, (sf->bytes - sf->sbytes) << 3,
                                    sf->isfloat, 1, sf->swap, *length);

                for (n_items = 0; n_items < *length; n_items++)
                {
                    ((float *)realbuf)[n_items] *= (float)sf->scale;
                }
            }
            else
            {
                raw2real::raw2reald(realbuf, rawbuf, sf->bytes, (sf->bytes - sf->sbytes) << 3,
                                    sf->isfloat, 1, sf->swap, *length);

                for (n_items = 0; n_items < *length; n_items++)
                {
                    ((double *)realbuf)[n_items] *= sf->scale;
                }
            }

            _aligned_free(rawbuf);
        }
        else
        {
            pinfo("Error reading coefficients from file %s.", filename);
        }

        return realbuf;
    }

    // Processes the specified coefficient sound file and
    // returns a coefficient set for each channel.
    //
    // Supported formats are any that the libsndfile library
    // can process.
    void **
    load_snd_coeff(const wchar_t *filename,
                   int *length,
                   int realsize,
                   int max_length,
                   int *n_coeffs)
    {
        int n_channels;
        int n_frames;
        void *buffer;
        void **coeffs = NULL;

        buffer = buffer::load_from_snd_file(filename, 
                                            &n_channels,
                                            &n_frames, 
                                            realsize, 
                                            max_length, 
                                            false);
    
        if (buffer != NULL)
        {
            coeffs = buffer::deinterlace(buffer, n_channels, n_frames, realsize);
            _aligned_free(buffer);

            if (coeffs != NULL)
            {
                *length = n_frames;
                *n_coeffs = n_channels;
            }
        }

        return coeffs;
    }

    // Preprocesses a coefficient set.
    void **
    preprocess_coeff(fftw_convolver *convolver,
                     void *coeffs,
                     int filter_length,
                     int coeff_blocks,
                     int coeff_length,
                     int realsize,
                     double scale)
    {
        int n;
        void *zbuf = NULL;
        void **cbuf = NULL;

        if (coeffs != NULL)
        {
            cbuf = (void **) _aligned_malloc(coeff_blocks * sizeof(void *), ALIGNMENT);

            if (coeff_length < coeff_blocks * filter_length)
            {
                zbuf = _aligned_malloc(filter_length * realsize, ALIGNMENT);
                memset(zbuf, 0, filter_length * realsize);
            }

            for (n = 0; n < coeff_blocks; n++)
            {
                if (n * filter_length > coeff_length)
                {
                    cbuf[n] = convolver->convolver_coeffs2cbuf(zbuf,
                                                               filter_length,
                                                               scale,
                                                               NULL);
                }
                else if ((n + 1) * filter_length > coeff_length)
                {
                    cbuf[n] = convolver->convolver_coeffs2cbuf(
                                  &((uint8_t *)coeffs)[n * filter_length * realsize],
                                  coeff_length - n * filter_length,
                                  scale,
                                  NULL);
                }
                else
                {
                    cbuf[n] = convolver->convolver_coeffs2cbuf(
                                  &((uint8_t *)coeffs)[n * filter_length * realsize],
                                  filter_length,
                                  scale,
                                  NULL);
                }

                if (cbuf[n] == NULL)
                {
                    pinfo("Failed to preprocess coefficient block %u.", n);
                }
            }

            if (zbuf != NULL)
            {
                _aligned_free(zbuf);
            }
        }

        return cbuf;
    }
}