/*
 * (c) Copyright 2001-2006 Anders Torger
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _COEFF_H_
#define _COEFF_H_

#include "global.h"
#include "fftw_convolver.hpp"

namespace coeff
{
    void **
    load_dirac_coeff(int n_channels,
                     int length,
                     int realsize);

    void *
    load_text_coeff(const wchar_t *filename,
                    int *length,
                    int realsize,
                    int max_length);

    void *
    load_raw_coeff(const wchar_t *filename,
                   int *length,
                   int realsize,
                   int max_length,
                   struct sample_format_t *sf);

    void **
    load_snd_coeff(const wchar_t *filename,
                   int *length,
                   int realsize,
                   int max_length,
                   int *n_coeffs);

    void **
    preprocess_coeff(fftw_convolver *convolver,
                     void *coeffs,
                     int filter_length,
                     int coeff_blocks,
                     int coeff_length,
                     int realsize,
                     double scale);
}

#endif
