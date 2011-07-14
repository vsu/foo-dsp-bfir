/*
 * (c) 2002, 2006 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _DELAY_HPP_
#define _DELAY_HPP_

#include "global.h"
#include "fftw_convolver.hpp"

struct _delaybuffer_t_
{
    int fragsize;      // fragment size
    int maxdelay;      // maximum allowable delay, or negative if delay cannot be
			           // changed in runtime
    int curdelay;      // current delay
    int curbuf;        // index of current full-sized buffer
    int n_fbufs;       // number of full-sized buffers currently used
    int n_fbufs_cap;   // number of full-sized buffers allocated
    void **fbufs;      // full-sized buffers
    int n_rest;        // samples in rest buffer
    void *rbuf;        // rest buffer
    void *shortbuf[2]; // pointers to buffers which fit the whole delay, only
                       // used when delay is <= fragment size
};

typedef struct _delaybuffer_t_ delaybuffer_t;

class delay
{
public:
    delay();

    // optional_target_buf has sample_spacing == 1
    void
    update(delaybuffer_t *db,
           void *buf,
           int sample_size,
           int sample_spacing,
           int delay,
           void *optional_target_buf);

    delaybuffer_t *
    allocate_buffer(int fragment_size,
                    int initdelay,
                    int maxdelay,
                    int sample_size);

    int
    subsample_filterblocksize(void);

    void
    subsample_update(fftw_convolver *convolver,
                     void *buf,
                     void *rest,
                     int subdelay);

    bool_t
    subsample_init(fftw_convolver *convolver,
                   int step_count,
                   int half_filter_length,
                   double kaiser_beta,
                   int fragment_size,
                   int _realsize);

private:
    double
    sinc(double x);

    void *
    sample_sinc(int half_length,
                double offset,
                double kaiser_beta);

    void
    copy_to_delaybuf(void *dbuf,
                     void *buf,
                     int sample_size,
                     int sample_spacing,
                     int n_samples);

    void
    copy_from_delaybuf(void *buf,
                       void *dbuf,
                       int sample_size,
                       int sample_spacing,
                       int n_samples);

    void
    shift_samples(void *buf,
                  int sample_size,
                  int sample_spacing,
                  int n_samples,
                  int distance);

    void
    update_delay_buffer(delaybuffer_t *db,
                        int sample_size,
                        int sample_spacing,
                        uint8_t *buf);

    void
    update_delay_short_buffer(delaybuffer_t *db,
                              int sample_size,
                              int sample_spacing,
                              uint8_t *buf);

    void
    change_delay(delaybuffer_t *db,
                 int sample_size,
                 int newdelay);

    int realsize;
    int subdelay_filter_length;
    td_conv_t **subdelay_filter;
    int subdelay_step_count;
    int subdelay_filterblock_size;
    int subdelay_fragment_size;
};

#endif
