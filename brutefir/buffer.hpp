/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_ 

#include <ctime>
#include <boost/random.hpp>

namespace buffer
{
    // define the random number generator
    typedef boost::lagged_fibonacci607 base_generator_type;

    // seeded random number generator
    static base_generator_type generator(static_cast<unsigned int>(std::time(NULL)));

    void *
    load_from_snd_file(const wchar_t *filename,
                       int *n_channels,
                       int *n_frames,
                       int realsize,
                       int max_frames,
                       bool pad);

    void
    save_to_snd_file(const wchar_t *filename,
                     void *buffer,
                     int n_channels,
                     int n_frames,
                     int realsize,
                     int sampling_rate);

    bool
    get_snd_file_params(const wchar_t *filename,
                        int *n_channels,
                        int *n_frames,
                        int *sampling_rate);

    bool
    check_snd_file(const wchar_t *filename, 
                   int n_channels,
                   int sampling_rate);

    void **
    deinterlace(void *buffer,
                int n_channels,
                int n_frames,
                int realsize);

    void *
    interlace(void **buffers,
              int n_channels,
              int n_frames,
              int realsize);

    void *
    load_white_noise(int n_channels,
                     int n_frames,
                     int realsize);
}

#endif