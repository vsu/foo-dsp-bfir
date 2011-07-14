/*
 * (c) 2001, 2003-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <malloc.h>
#include <string.h>

#include "global.h"
#include "dither.hpp"
#include "pinfo.h"

// desired spacing between channels in random number table in seconds
#define RANDTAB_SPACING 10
#define MIN_RANDTAB_SPACING 1

#define TAUSWORTHE(s,a,b,c,d) ((s & c) << d) ^ (((s <<a) ^ s) >> b)
#define LCG(n) ((69069 * n) & 0xFFFFFFFFU)

dither::dither(int n_channels,
               int sample_rate,
               int _realsize,
               int max_size,
               int max_samples_per_loop,
               struct dither_state_t *dither_state)
    : dither_randtab(NULL), dither_randmap(NULL), dither_randmap_ptr(NULL)
{
    int n, spacing = RANDTAB_SPACING * sample_rate, minspacing;
    uint32_t state[3];

    realsize = _realsize;

    minspacing = 
        (MIN_RANDTAB_SPACING * sample_rate > max_samples_per_loop) 
            ? MIN_RANDTAB_SPACING * sample_rate 
            : max_samples_per_loop;

    if (spacing < minspacing)
    {
        spacing = minspacing;
    }

    if (max_size > 0)
    {
        if (n_channels * spacing > max_size)
        {
            spacing = max_size / n_channels;
        }
    }

    if (spacing < minspacing)
    {
        pinfo("Maximum dither table size %d bytes is too small, must "
              "at least be %d bytes.\n", max_size,
              n_channels * sample_rate * minspacing);
        throw;
    }

    dither_randtab_size = n_channels * spacing + 1;

    pinfo("Dither table size is %d bytes.\n"
          "Generating random numbers.", dither_randtab_size);

    tausinit(state, 0);

    dither_randtab = (int8_t *) _aligned_malloc(dither_randtab_size, ALIGNMENT);

    for (n = 0; n < dither_randtab_size; n++)
    {
        dither_randtab[n] = (int8_t)(tausrand(state) & 0x000000FF);
    }

    // make a map for conversion of integer dither random numbers to
    // floating point ranging from -1.0 to +1.0, plus an offset of +0.5,
    // used to make the sample truncation be mid-tread requantisation
    dither_randmap_ptr = _aligned_malloc(realsize * 511, ALIGNMENT);
    dither_randmap = &((uint8_t *)dither_randmap_ptr)[256 * realsize];

    if (realsize == 4)
    {
        ((float *)dither_randmap)[-256] = -0.5;

        for (n = -255; n < 254; n++)
        {
            ((float *)dither_randmap)[n] =
                0.5 + 1.0 / 255.0 + 1.0 / 255.0 * (float)n;
        }

        ((float *)dither_randmap)[254] = 1.5;
    }
    else
    {
        ((double *)dither_randmap)[-256] = -0.5;
        
        for (n = -255; n < 254; n++)
        {
            ((double *)dither_randmap)[n] =
                0.5 + 1.0 / 255.0 + 1.0 / 255.0 * (double)n;
        }
        
        ((double *)dither_randmap)[254] = 1.5;
    }

    for (n = 0; n < n_channels; n++)
    {
        memset(&dither_state[n], 0, sizeof(struct dither_state_t));
        dither_state[n].randtab_ptr = n * spacing + 1;
    }
}

dither::~dither()
{
    if (dither_randtab != NULL)
    {
        _aligned_free(dither_randtab);
        dither_randtab = NULL;
    }

    if (dither_randmap_ptr != NULL)
    {
        _aligned_free(dither_randmap_ptr);
        dither_randmap_ptr = NULL;
    }
}

void
dither::dither_preloop_real2int_hp_tpdf(struct dither_state_t *state,
                                        int samples_per_loop)
{
    if (state->randtab_ptr + samples_per_loop >= dither_randtab_size)
    {
        dither_randtab[0] = dither_randtab[state->randtab_ptr - 1];
        state->randtab_ptr = 1;
    }

    state->randtab = &dither_randtab[state->randtab_ptr];
    state->randtab_ptr += samples_per_loop;
}

int32_t
dither::ditherf_real2int_hp_tpdf(float real_sample,
                                 float rmin, // (float)imin
                                 float rmax, // (float)imax
                                 int32_t imin,
                                 int32_t imax,
                                 struct bfoverflow_t *overflow,
                                 struct dither_state_t *state,
                                 int loop_counter)
{
    int32_t sample;
    float dithered_sample;

    // apply error feedback, with coefficients {1, -1} (high pass)
    real_sample += state->sf[0] - state->sf[1];
    state->sf[1] = state->sf[0];

    // apply dither and offset
    dithered_sample = real_sample +
                      ((float *)dither_randmap)[state->randtab[loop_counter] -
                      state->randtab[loop_counter - 1]];

    if (dithered_sample < 0)
    {
        if (dithered_sample <= rmin)
        {
            sample = imin;
            overflow->n_overflows++;

            if (real_sample < -overflow->largest)
            {
                overflow->largest = (double)-dithered_sample;
            }
        }
        else
        {
            sample = (int32_t)dithered_sample;
            sample--;

            if (sample < -overflow->intlargest)
            {
                overflow->intlargest = -sample;
            }
        }
    }
    else
    {
        if (dithered_sample > rmax)
        {
            sample = imax;
            overflow->n_overflows++;

            if (real_sample > overflow->largest)
            {
                overflow->largest = (double)dithered_sample;
            }
        }
        else
        {
            sample = (int32_t)dithered_sample;

            if (sample > overflow->intlargest)
            {
                overflow->intlargest = sample;
            }
        }
    }

    state->sf[0] = real_sample - (float)sample;

    return sample;
}

int32_t
dither::ditherf_real2int_no_dither(float real_sample,
                                   float rmin,  // (float)imin
                                   float rmax,  // (float)imax
                                   int32_t imin,
                                   int32_t imax,
                                   struct bfoverflow_t *overflow)
{
    int32_t sample;

    // Truncation is here made downwards, meaning 3.8 -> 3 and -3.2 -> -4. By
    // adding 0.5 before our truncation we get a mid-tread requantiser.
    real_sample += 0.5;
    if (real_sample < 0)
    {
        if (real_sample <= rmin)
        {
            sample = imin;
            overflow->n_overflows++;

            if (real_sample < -overflow->largest)
            {
                overflow->largest = (double)-real_sample;
            }
        }
        else
        {
            sample = (int32_t)real_sample;
            sample--;

            if (sample < -overflow->intlargest)
            {
                overflow->intlargest = -sample;
            }
        }
    }
    else
    {
        if (real_sample > rmax)
        {
            sample = imax;
            overflow->n_overflows++;

            if (real_sample > overflow->largest)
            {
                overflow->largest = (double)real_sample;
            }
        }
        else
        {
            sample = (int32_t)real_sample;

            if (sample > overflow->intlargest)
            {
                overflow->intlargest = sample;
            }
        }
    }

    return sample;
}

int32_t
dither::ditherd_real2int_hp_tpdf(double real_sample,
                                 double rmin, // (double)imin
                                 double rmax, // (double)imax
                                 int32_t imin,
                                 int32_t imax,
                                 struct bfoverflow_t *overflow,
                                 struct dither_state_t *state,
                                 int loop_counter)
{
    int32_t sample;
    double dithered_sample;

    // apply error feedback, with coefficients {1, -1} (high pass)
    real_sample += state->sd[0] - state->sd[1];
    state->sd[1] = state->sd[0];

    // apply dither and offset
    dithered_sample = real_sample +
                      ((double *)dither_randmap)[state->randtab[loop_counter] -
                              state->randtab[loop_counter - 1]];

    if (dithered_sample < 0)
    {
        if (dithered_sample <= rmin)
        {
            sample = imin;
            overflow->n_overflows++;

            if (real_sample < -overflow->largest)
            {
                overflow->largest = (double)-dithered_sample;
            }
        }
        else
        {
            sample = (int32_t)dithered_sample;
            sample--;

            if (sample < -overflow->intlargest)
            {
                overflow->intlargest = -sample;
            }
        }
    }
    else
    {
        if (dithered_sample > rmax)
        {
            sample = imax;
            overflow->n_overflows++;

            if (real_sample > overflow->largest)
            {
                overflow->largest = (double)dithered_sample;
            }
        }
        else
        {
            sample = (int32_t)dithered_sample;

            if (sample > overflow->intlargest)
            {
                overflow->intlargest = sample;
            }
        }
    }

    state->sd[0] = real_sample - (double)sample;

    return sample;
}

int32_t
dither::ditherd_real2int_no_dither(double real_sample,
                                   double rmin,  // (double)imin
                                   double rmax,  // (double)imax
                                   int32_t imin,
                                   int32_t imax,
                                   struct bfoverflow_t *overflow)
{
    int32_t sample;

    // Truncation is here made downwards, meaning 3.8 -> 3 and -3.2 -> -4. By
    // adding 0.5 before our truncation we get a mid-tread requantiser.
    real_sample += 0.5;
    if (real_sample < 0)
    {
        if (real_sample <= rmin)
        {
            sample = imin;
            overflow->n_overflows++;

            if (real_sample < -overflow->largest)
            {
                overflow->largest = (double)-real_sample;
            }
        }
        else
        {
            sample = (int32_t)real_sample;
            sample--;

            if (sample < -overflow->intlargest)
            {
                overflow->intlargest = -sample;
            }
        }
    }
    else
    {
        if (real_sample > rmax)
        {
            sample = imax;
            overflow->n_overflows++;

            if (real_sample > overflow->largest)
            {
                overflow->largest = (double)real_sample;
            }
        }
        else
        {
            sample = (int32_t)real_sample;

            if (sample > overflow->intlargest)
            {
                overflow->intlargest = sample;
            }
        }
    }

    return sample;
}

//
// This is a maximally equidistributed combined Tausworthe generator, stolen
// from GNU Scientific Library (GSL). See GSL documentation for further
// information.
//
// Generates numbers between 0x0 - 0xFFFFFFFF
//
uint32_t
dither::tausrand(uint32_t state[3])
{
    state[0] = TAUSWORTHE(state[0], 13, 19, (uint32_t)4294967294U, 12);
    state[1] = TAUSWORTHE(state[1], 2, 25, (uint32_t)4294967288U, 4);
    state[2] = TAUSWORTHE(state[2], 3, 11, (uint32_t)4294967280U, 17);

    return (state[0] ^ state[1] ^ state[2]);
}

void
dither::tausinit(uint32_t state[3],
                 uint32_t seed)
{
    // default seed is 1
    if (seed == 0)
    {
        seed = 1;
    }

    state[0] = LCG(seed);
    state[1] = LCG(state[0]);
    state[2] = LCG(state[1]);

    // "warm it up"
    tausrand(state);
    tausrand(state);
    tausrand(state);
    tausrand(state);
    tausrand(state);
    tausrand(state);
}
