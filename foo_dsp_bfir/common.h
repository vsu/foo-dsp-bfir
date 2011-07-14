/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include "equalizer.hpp"

#define COMPONENT_NAME     "BruteFIR DSP"
#define COMPONENT_VERSION  "0.1"
#define REALSIZE           8
#define FILTER_LEN         1024
#define EQ_FILTER_BLOCKS   64
#define PATH_MAX           1024

struct file_param_t
{
    int enabled;
    int slider_level;  
    int n_channels;
    int n_frames;
    int sampling_rate;
    wchar_t filename[PATH_MAX];
};

struct dsp_bfir_param_t
{
    double mag[BAND_COUNT];
    int eq_enabled;
    int eq_slider_level;
    int overflow_warnings;
    struct file_param_t file[3];
};

#endif
