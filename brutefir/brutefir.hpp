/*
 * (c) 2011 Victor Su
 * (c) 2001, 2003 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _BRUTEFIR_HPP_
#define _BRUTEFIR_HPP_

#include "global.h"
#include "fftw_convolver.hpp"
#include "dither.hpp"

class brutefir
{
public:
    brutefir(int filter_length, 
             int filter_blocks, 
             int realsize,
             int channels, 
             int in_format,
             int out_format,
             int sampling_rate,
             bool_t apply_dither);
    
    ~brutefir();

    bool_t
    is_initialized();

    int
    set_coeff(const wchar_t *filename, 
              int coeff_blocks,
              double scale);

    int
    set_coeff(void **coeffs,
              int n_coeffs,
              int length,
              int coeff_blocks,
              double scale);

    int
    run(void *inbuf, 
        void *outbuf);

    void
    reset();

    void
    check_overflows();

private:
    double 
    get_full_scale(int bytes);
    
    double 
    get_normalized_scale(int bytes);
    
    double 
    get_max(int bytes);
    
    void
    setup_sample_format(int format,  
                        struct sample_format_t *sf, 
                        bool_t normalized);

    void
    setup_input(int index,
                int format);

    void
    setup_output(int index,
                 int format,
                 bool_t apply_dither);

    void
    print_overflows();

    int 
    init_convolver(int filter_length, 
                   int filter_blocks, 
                   int realsize);

    int
    init_channels(int n_channels, 
                  int in_format,
                  int out_format,
                  int sampling_rate,
                  bool_t apply_dither);

    int
    init_buffers();
    
    void
    free_buffers();

    void
    free_coeff();

    bool_t m_initialized;

    fftw_convolver *m_convolver;
    dither *m_dither;

    struct bfconf_t *bfconf;

    int convbufsize;
    int curbuf;
    int curblock;
    unsigned int blockcounter;

    uint8_t *baseptr;

    void *input_timecbuf[BF_MAXCHANNELS][2];

    void *input_freqcbuf[BF_MAXCHANNELS];
    void *output_freqcbuf[BF_MAXCHANNELS];

    void **cbuf[BF_MAXCHANNELS];
    void *ocbuf[BF_MAXCHANNELS];

    int procblocks[BF_MAXCHANNELS];

    struct bfoverflow_t overflow[BF_MAXCHANNELS];
    struct bfoverflow_t last_overflow[BF_MAXCHANNELS];
};

#endif
