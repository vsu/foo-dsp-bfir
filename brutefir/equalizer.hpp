/*
 * (c) 2011 Victor Su
 * (c) 2002-2005 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _EQUALIZER_HPP_
#define _EQUALIZER_HPP_

#include "fftw_convolver.hpp"

#define ISO_BANDS_SIZE  31
#define BAND_COUNT  ISO_BANDS_SIZE

// ISO 1/3 octave bands
static const double iso_bands[ISO_BANDS_SIZE] = 
{ 
    20, 
    25, 
    31.5, 
    40, 
    50, 
    63, 
    80, 
    100, 
    125, 
    160,
    200, 
    250, 
    315, 
    400,
    500,
    630,
    800,
    1000,
    1250,
    1600,
    2000,
    2500,
    3150,
    4000,
    5000,
    6300,
    8000,
    10000,
    12500,
    16000,
    20000
};

struct equalizer_t 
{
    int block_length;
    int n_blocks;
    int realsize;
    int n_channels;
    int sampling_rate;
    void *ifftplan;
    int band_count;
    int taps;
    double freq[BAND_COUNT + 2];  // Add two bands for zero and freq max
    double mag[BAND_COUNT + 2];
    double phase[BAND_COUNT + 2];
};

class equalizer
{
public:
    equalizer(int block_length,
              int n_blocks,
              int realsize,
              int n_channels,
              int sampling_rate);

    ~equalizer();

    std::wstring
    generate(int n_bands,
             double *freq, 
             double *mag, 
             double *phase);

private:
    std::wstring
    make_filename(int n_bands,
                  double *freq, 
                  double *mag, 
                  double *phase);

    float
    cosine_int_f(float mag1,
                 float mag2,
                 float freq1,
                 float freq2,
                 float curfreq);

    double
    cosine_int_d(double mag1,
                 double mag2,
                 double freq1,
                 double freq2,
                 double curfreq);

    void
    render_f(struct equalizer_t *eq,
             const wchar_t *filename);

    void
    render_d(struct equalizer_t *eq,
             const wchar_t *filename);

    fftw_convolver *m_convolver;
    struct equalizer_t m_equalizer;
};


#endif
