/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _PREPROCESSOR_HPP_
#define _PREPROCESSOR_HPP_

#include <string>
#include <vector>

struct impulse_info
{
    std::wstring filename;
    double scale;
};

namespace preprocessor
{
    std::wstring
    convolve_impulses(std::vector<struct impulse_info> impulse_info, 
                      int filter_length,
                      int realsize);
   
    bool
    calculate_attenuation(std::wstring filename,
                          int filter_length,
                          int realsize,
                          double *attenuation,
                          int *n_channels,
                          int *n_frames,
                          int *sampling_rate);
}

#endif