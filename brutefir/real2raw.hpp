/*
 * (c) 2011 Victor Su
 * (c) 2001-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _REAL2RAW_HPP_
#define _REAL2RAW_HPP_

#include "global.h"
#include "dither.hpp"

namespace real2raw
{
    void
    real2rawf_hp_tpdf(void *_rawbuf,
                        void *_realbuf,
                        int bits,
                        int bytes,
                        int shift,
                        bool_t isfloat,
                        int spacing,
                        bool_t swap,
                        int n_samples,
                        struct bfoverflow_t *overflow,
                        struct dither_state_t *dither_state,
                        dither *dither);

    void
    real2rawf_no_dither(void *_rawbuf,
                        void *_realbuf,
                        int bits,
                        int bytes,
                        int shift,
                        bool_t isfloat,
                        int spacing,
                        bool_t swap,
                        int n_samples,
                        struct bfoverflow_t *overflow,
                        dither *dither);

    void
    real2rawd_hp_tpdf(void *_rawbuf,
                        void *_realbuf,
                        int bits,
                        int bytes,
                        int shift,
                        bool_t isfloat,
                        int spacing,
                        bool_t swap,
                        int n_samples,
                        struct bfoverflow_t *overflow,
                        struct dither_state_t *dither_state,
                        dither *dither);

    void
    real2rawd_no_dither(void *_rawbuf,
                        void *_realbuf,
                        int bits,
                        int bytes,
                        int shift,
                        bool_t isfloat,
                        int spacing,
                        bool_t swap,
                        int n_samples,
                        struct bfoverflow_t *overflow,
                        dither *dither);
};

#endif