/*
 * (c) 2011 Victor Su
 * (c) 2001-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "global.h"
#include "numunion.h"
#include "dither.hpp"
#include "swap.h"
#include "pinfo.h"
#include "real2raw.hpp"

namespace real2raw
{
#define REAL_OVERFLOW_UPDATE                                                   \
    if (realbuf->r32[n] < 0.0) {                                               \
        if (realbuf->r32[n] < rmin) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r32[n] < -overflow->largest) {                            \
            overflow->largest = -realbuf->r32[n];                              \
        }                                                                      \
    } else {                                                                   \
        if (realbuf->r32[n] > rmax) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r32[n] > overflow->largest) {                             \
            overflow->largest = realbuf->r32[n];                               \
        }                                                                      \
    }

#define REAL2INT_CALL dither->ditherf_real2int_hp_tpdf(((float *)realbuf)[n], rmin,    \
                                                       rmax, imin, imax, overflow,     \
                                                       dither_state, n)

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
                      dither *dither)
    {
        numunion_t *rawbuf, *realbuf, sample;
        int32_t imin, imax;
        float rmin, rmax;
        int n, i;

        // It is assumed that sbytes only can have the values possible from the
        // supported sample formats specified in global.h

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
            rmin = -overflow->max;
            rmax = overflow->max;

            if (shift != 0)
            {
                pinfo("Shift must be zero for floating point formats.\n");
                return;
            }

            switch (bytes)
            {
            case 4:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->u32[i] = SWAP32(realbuf->u32[n]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r32[i] = realbuf->r32[n];
                    }
                }

                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        sample.r64[0] = (double)realbuf->r32[n];
                        rawbuf->u64[i] = SWAP64(sample.u64[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r64[i] = (double)realbuf->r32[n];
                    }
                }
                break;
            default:
                goto real2raw_invalid_byte_size;
            }

            return;
        }

        imin = -(1 << (bits - 1));
        imax = (1 << (bits - 1)) - 1;
        rmin = (float)imin;
        rmax = (float)imax;

        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                rawbuf->i8[i] = (int8_t)REAL2INT_CALL;
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)REAL2INT_CALL;
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)(REAL2INT_CALL << shift);
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)(REAL2INT_CALL << shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
    #ifdef __BIG_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
    #endif
    #ifdef __LITTLE_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
    #endif
            break;
        case 4:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL << shift;
                    }
                }
            }

            break;
        default:
    real2raw_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }

#undef REAL_OVERFLOW_UPDATE
#undef REAL2INT_CALL


#define REAL_OVERFLOW_UPDATE                                                   \
    if (realbuf->r32[n] < 0.0) {                                               \
        if (realbuf->r32[n] < rmin) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r32[n] < -overflow->largest) {                            \
            overflow->largest = -realbuf->r32[n];                              \
        }                                                                      \
    } else {                                                                   \
        if (realbuf->r32[n] > rmax) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r32[n] > overflow->largest) {                             \
            overflow->largest = realbuf->r32[n];                               \
        }                                                                      \
    }

#define REAL2INT_CALL dither->ditherf_real2int_no_dither(((float *)realbuf)[n], rmin,  \
                                                         rmax, imin, imax, overflow)

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
                        dither *dither)
    {
        numunion_t *rawbuf, *realbuf, sample;
        int32_t imin, imax;
        float rmin, rmax;
        int n, i;

        // It is assumed that sbytes only can have the values possible from the
        // supported sample formats specified in global.h

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
            rmin = -overflow->max;
            rmax = overflow->max;

            if (shift != 0)
            {
                pinfo("Shift must be zero for floating point formats.\n");
                return;
            }

            switch (bytes)
            {
            case 4:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->u32[i] = SWAP32(realbuf->u32[n]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r32[i] = realbuf->r32[n];
                    }
                }

                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        sample.r64[0] = (double)realbuf->r32[n];
                        rawbuf->u64[i] = SWAP64(sample.u64[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r64[i] = (double)realbuf->r32[n];
                    }
                }
                break;
            default:
                goto real2raw_invalid_byte_size;
            }

            return;
        }

        imin = -(1 << (bits - 1));
        imax = (1 << (bits - 1)) - 1;
        rmin = (float)imin;
        rmax = (float)imax;

        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                rawbuf->i8[i] = (int8_t)REAL2INT_CALL;
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)REAL2INT_CALL;
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)(REAL2INT_CALL << shift);
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)(REAL2INT_CALL << shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
    #ifdef __BIG_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
    #endif
    #ifdef __LITTLE_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
    #endif
            break;
        case 4:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL << shift;
                    }
                }
            }

            break;
        default:
    real2raw_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }

#undef REAL_OVERFLOW_UPDATE
#undef REAL2INT_CALL


#define REAL_OVERFLOW_UPDATE                                                   \
    if (realbuf->r64[n] < 0.0) {                                               \
        if (realbuf->r64[n] < rmin) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r64[n] < -overflow->largest) {                            \
            overflow->largest = -realbuf->r64[n];                              \
        }                                                                      \
    } else {                                                                   \
        if (realbuf->r64[n] > rmax) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r64[n] > overflow->largest) {                             \
            overflow->largest = realbuf->r64[n];                               \
        }                                                                      \
    }

#define REAL2INT_CALL dither->ditherd_real2int_hp_tpdf(((double *)realbuf)[n], rmin,   \
                                                       rmax, imin, imax, overflow,     \
                                                       dither_state, n)

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
                        dither *dither)
    {
        numunion_t *rawbuf, *realbuf, sample;
        int32_t imin, imax;
        double rmin, rmax;
        int n, i;

        // It is assumed that sbytes only can have the values possible from the
        // supported sample formats specified in global.h

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
            rmin = -overflow->max;
            rmax = overflow->max;

            if (shift != 0)
            {
                pinfo("Shift must be zero for floating point formats.\n");
                return;
            }

            switch (bytes)
            {
            case 4:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        sample.r32[0] = (float)realbuf->r64[n];
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r32[i] = (float)realbuf->r64[n];
                    }
                }
                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->u64[i] = SWAP64(realbuf->u64[n]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r64[i] = realbuf->r64[n];
                    }
                }
                break;
            default:
                goto real2raw_invalid_byte_size;
            }

            return;
        }

        imin = -(1 << (bits - 1));
        imax = (1 << (bits - 1)) - 1;
        rmin = (double)imin;
        rmax = (double)imax;

        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                rawbuf->i8[i] = (int8_t)REAL2INT_CALL;
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)REAL2INT_CALL;
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)(REAL2INT_CALL << shift);
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)(REAL2INT_CALL << shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
        #ifdef __BIG_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
        #endif
        #ifdef __LITTLE_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
        #endif
            break;
        case 4:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL << shift;
                    }
                }
            }

            break;
        default:
        real2raw_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }

#undef REAL_OVERFLOW_UPDATE
#undef REAL2INT_CALL


#define REAL_OVERFLOW_UPDATE                                                   \
    if (realbuf->r64[n] < 0.0) {                                               \
        if (realbuf->r64[n] < rmin) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r64[n] < -overflow->largest) {                            \
            overflow->largest = -realbuf->r64[n];                              \
        }                                                                      \
    } else {                                                                   \
        if (realbuf->r64[n] > rmax) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->r64[n] > overflow->largest) {                             \
            overflow->largest = realbuf->r64[n];                               \
        }                                                                      \
    }

#define REAL2INT_CALL dither->ditherd_real2int_no_dither(((double *)realbuf)[n], rmin, \
                                                         rmax, imin, imax, overflow)

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
                        dither *dither)
    {
        numunion_t *rawbuf, *realbuf, sample;
        int32_t imin, imax;
        double rmin, rmax;
        int n, i;

        // It is assumed that sbytes only can have the values possible from the
        // supported sample formats specified in global.h

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
            rmin = -overflow->max;
            rmax = overflow->max;

            if (shift != 0)
            {
                pinfo("Shift must be zero for floating point formats.\n");
                return;
            }

            switch (bytes)
            {
            case 4:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        sample.r32[0] = (float)realbuf->r64[n];
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r32[i] = (float)realbuf->r64[n];
                    }
                }
                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->u64[i] = SWAP64(realbuf->u64[n]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        REAL_OVERFLOW_UPDATE;
                        rawbuf->r64[i] = realbuf->r64[n];
                    }
                }
                break;
            default:
                goto real2raw_invalid_byte_size;
            }

            return;
        }

        imin = -(1 << (bits - 1));
        imax = (1 << (bits - 1)) - 1;
        rmin = (double)imin;
        rmax = (double)imax;

        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                rawbuf->i8[i] = (int8_t)REAL2INT_CALL;
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)REAL2INT_CALL;
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i16[0] = (int16_t)(REAL2INT_CALL << shift);
                        rawbuf->u16[i] = SWAP16(sample.u16[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i16[i] = (int16_t)(REAL2INT_CALL << shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
    #ifdef __BIG_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[3];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[3];
                    }
                }
            }
    #endif
    #ifdef __LITTLE_ENDIAN__
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[2];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u8[i++] = sample.u8[0];
                        rawbuf->u8[i++] = sample.u8[1];
                        rawbuf->u8[i++] = sample.u8[2];
                    }
                }
            }
    #endif
            break;
        case 4:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL;
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.i32[0] = REAL2INT_CALL << shift;
                        rawbuf->u32[i] = SWAP32(sample.u32[0]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        rawbuf->i32[i] = REAL2INT_CALL << shift;
                    }
                }
            }

            break;
        default:
    real2raw_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }

#undef REAL_OVERFLOW_UPDATE
#undef REAL2INT_CALL
}