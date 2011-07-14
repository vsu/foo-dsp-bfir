/*
 * (c) 2001, 2003 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _DITHER_HPP_
#define _DITHER_HPP_

#include "global.h"

class dither
{
public:
    dither(int n_channels,
           int sample_rate,
           int realsize,
           int max_size,
           int max_samples_per_loop,
           struct dither_state_t *dither_state);

    ~dither();

    void
    dither_preloop_real2int_hp_tpdf(struct dither_state_t *state,
                                    int samples_per_loop);

    int32_t
    ditherf_real2int_hp_tpdf(float real_sample,
                             float rmin,  // (float)imin
                             float rmax,  // (float)imax
                             int32_t imin,
                             int32_t imax,
                             struct bfoverflow_t *overflow,
                             struct dither_state_t *state,
                             int loop_counter);

    int32_t
    ditherf_real2int_no_dither(float real_sample,
                               float rmin,  // (float)imin
                               float rmax,  // (float)imax
                               int32_t imin,
                               int32_t imax,
                               struct bfoverflow_t *overflow);

    int32_t
    ditherd_real2int_hp_tpdf(double real_sample,
                             double rmin,  // (double)imin
                             double rmax,  // (double)imax
                             int32_t imin,
                             int32_t imax,
                             struct bfoverflow_t *overflow,
                             struct dither_state_t *state,
                             int loop_counter);

    int32_t
    ditherd_real2int_no_dither(double real_sample,
                               double rmin,  // (double)imin
                               double rmax,  // (double)imax
                               int32_t imin,
                               int32_t imax,
                               struct bfoverflow_t *overflow);

private:
    uint32_t
    tausrand(uint32_t state[3]);

    void
    tausinit(uint32_t state[3],
             uint32_t seed);

    int8_t *dither_randtab;
    int dither_randtab_size;
    void *dither_randmap;
    void *dither_randmap_ptr;
    int realsize;
};

#endif
