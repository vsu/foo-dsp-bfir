/*
 * (c) 2011 Victor Su
 * (c) 2001-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "global.h"
#include "numunion.h"
#include "swap.h"
#include "pinfo.h"
#include "raw2real.hpp"

namespace raw2real
{
    void
    raw2realf(void *_realbuf,
              void *_rawbuf,
              int bytes,
              int shift,
              bool isfloat,
              int spacing,
              bool swap,
              int n_samples)
    {
        numunion_t *realbuf, *rawbuf, sample;
        int n, i;

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
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
                        realbuf->u32[n] = SWAP32(rawbuf->u32[i]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = rawbuf->r32[i];
                    }
                }
                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        sample.u64[0] = SWAP64(rawbuf->u64[i]);
                        realbuf->r32[n] = (float)sample.r64[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)rawbuf->r64[i];
                    }
                }
                break;
            default:
                goto raw2real_invalid_byte_size;
            }

            return;
        }

        sample.u64[0] = 0;
        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                realbuf->r32[n] = (float)rawbuf->i8[i];
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)((int16_t)SWAP16(rawbuf->u16[i]));
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)rawbuf->i16[i];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] =
                            (float)((int16_t)SWAP16(rawbuf->u16[i]) >> shift);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)(rawbuf->i16[i] >> shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
            shift += 8;
    #ifdef __BIG_ENDIAN__
            if (swap)
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[0] = rawbuf->u8[i++];
                    realbuf->r32[n] = (float)(sample.i32[0] >> shift);
                }
            }
            else
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[0] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    realbuf->r32[n] = (float)(sample.i32[0] >> shift);
                }
            }
    #endif
    #ifdef __LITTLE_ENDIAN__
            if (swap)
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[3] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    realbuf->r32[n] = (float)(sample.i32[0] >> shift);
                }
            }
            else
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[3] = rawbuf->u8[i++];
                    realbuf->r32[n] = (float)(sample.i32[0] >> shift);
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
                        realbuf->r32[n] = (float)((int32_t)SWAP32(rawbuf->u32[i]));
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)rawbuf->i32[i];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] =
                            (float)((int32_t)SWAP32(rawbuf->u32[i]) >> shift);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r32[n] = (float)(rawbuf->i32[i] >> shift);
                    }
                }
            }

            break;
        default:
    raw2real_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }

    void
    raw2reald(void *_realbuf,
              void *_rawbuf,
              int bytes,
              int shift,
              bool isfloat,
              int spacing,
              bool swap,
              int n_samples)
    {
        numunion_t *realbuf, *rawbuf, sample;
        int n, i;

        realbuf = (numunion_t *)_realbuf;
        rawbuf = (numunion_t *)_rawbuf;

        if (isfloat)
        {
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
                        sample.u32[0] = SWAP32(rawbuf->u32[i]);
                        realbuf->r64[n] = (double)sample.r32[0];
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)rawbuf->r32[i];
                    }
                }
                break;
            case 8:
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->u64[n] = SWAP64(rawbuf->u64[i]);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = rawbuf->r64[i];
                    }
                }
                break;
            default:
                goto raw2real_invalid_byte_size;
            }

            return;
        }

        sample.u64[0] = 0;
        switch (bytes)
        {
        case 1:
            for (n = i = 0; n < n_samples; n++, i += spacing)
            {
                realbuf->r64[n] = (double)rawbuf->i8[i];
            }
            break;
        case 2:
            if (shift == 0)
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)((int16_t)SWAP16(rawbuf->u16[i]));
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)rawbuf->i16[i];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] =
                            (double)((int16_t)SWAP16(rawbuf->u16[i]) >> shift);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)(rawbuf->i16[i] >> shift);
                    }
                }
            }
            break;
        case 3:
            spacing = spacing * 3 - 3;
            shift += 8;
    #ifdef __BIG_ENDIAN__
            if (swap)
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[0] = rawbuf->u8[i++];
                    realbuf->r64[n] = (double)(sample.i32[0] >> shift);
                }
            }
            else
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[0] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    realbuf->r64[n] = (double)(sample.i32[0] >> shift);
                }
            }
    #endif
    #ifdef __LITTLE_ENDIAN__
            if (swap)
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[3] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[1] = rawbuf->u8[i++];
                    realbuf->r64[n] = (double)(sample.i32[0] >> shift);
                }
            }
            else
            {
                for (n = i = 0; n < n_samples; n++, i += spacing)
                {
                    sample.u8[1] = rawbuf->u8[i++];
                    sample.u8[2] = rawbuf->u8[i++];
                    sample.u8[3] = rawbuf->u8[i++];
                    realbuf->r64[n] = (double)(sample.i32[0] >> shift);
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
                        realbuf->r64[n] = (double)((int32_t)SWAP32(rawbuf->u32[i]));
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)rawbuf->i32[i];
                    }
                }
            }
            else
            {
                if (swap)
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] =
                            (double)((int32_t)SWAP32(rawbuf->u32[i]) >> shift);
                    }
                }
                else
                {
                    for (n = i = 0; n < n_samples; n++, i += spacing)
                    {
                        realbuf->r64[n] = (double)(rawbuf->i32[i] >> shift);
                    }
                }
            }

            break;
        default:
    raw2real_invalid_byte_size:
            pinfo("Sample byte size %d is not suppported.\n", bytes);
            break;
        }
    }
}