#ifdef __cplusplus
extern "C" {
#endif 

/*
 * (c) 2011 Victor Su
 * (c) 2001 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "defs.h"
#include <stdint.h>
#include <math.h>

// limits
#define BF_MAXCHANNELS 8

// sample formats
#define BF_SAMPLE_FORMAT_S8 1
#define BF_SAMPLE_FORMAT_S16_LE 2
#define BF_SAMPLE_FORMAT_S16_BE 3
#define BF_SAMPLE_FORMAT_S24_LE 4
#define BF_SAMPLE_FORMAT_S24_BE 5
#define BF_SAMPLE_FORMAT_S32_LE 6
#define BF_SAMPLE_FORMAT_S32_BE 7
#define BF_SAMPLE_FORMAT_FLOAT_LE 8
#define BF_SAMPLE_FORMAT_FLOAT_BE 9
#define BF_SAMPLE_FORMAT_FLOAT64_LE 10
#define BF_SAMPLE_FORMAT_FLOAT64_BE 11

#define BF_SAMPLE_FORMAT_MIN BF_SAMPLE_FORMAT_S8
#define BF_SAMPLE_FORMAT_MAX BF_SAMPLE_FORMAT_FLOAT64_BE

struct sample_format_t
{
    bool isfloat;
    bool swap;
    int bytes;
    int sbytes;
    double scale;
    int format;
};

struct buffer_format_t
{
    struct sample_format_t sf;
    int sample_spacing; // in samples
    int byte_offset;    // in bytes
};

struct bfchannel_t
{
    int intname;
    struct buffer_format_t bf;
    bool apply_dither;
};

struct dither_state_t
{
    int randtab_ptr;
    int8_t *randtab;
    float sf[2];
    double sd[2];
};

struct bfcoeff_t
{
    int intname;
    int n_blocks;
    int n_channels;
    int channels[BF_MAXCHANNELS];
    void **data;
};

struct bfconf_t
{
    int filter_length;
    int n_blocks;
    int realsize;
    int sampling_rate;

    struct dither_state_t dither_state[BF_MAXCHANNELS];
    int max_dither_table_size;

    int n_channels;
    struct bfchannel_t inputs[BF_MAXCHANNELS];
    struct bfchannel_t outputs[BF_MAXCHANNELS];
    struct bfcoeff_t coeffs[BF_MAXCHANNELS];
};

struct bfoverflow_t
{
    unsigned int n_overflows;
    int32_t intlargest;
    double largest;
    double max;
};

#endif

#ifdef __cplusplus
}
#endif 