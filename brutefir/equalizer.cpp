/*
 * (c) 2011 Victor Su
 * (c) 2002-2005 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <malloc.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include <fftw3.h>

#include "global.h"
#include "equalizer.hpp"
#include "fftw_convolver.hpp"
#include "buffer.hpp"
#include "bfir_path.hpp"
#include "log2.h"
#include "hash.h"
#include "pinfo.h"

// Constructor for the class.
equalizer::equalizer(int block_length,
                     int n_blocks,
                     int realsize,
                     int n_channels,
                     int sampling_rate)
{
    int n;

    if ((n = log2_get(block_length * n_blocks)) == -1)
    {
        pinfo("Equalizer length (%u, %u) is not a power of two.", block_length, n_blocks);
        throw;
    }

    memset(&m_equalizer, 0, sizeof(struct equalizer_t));

    m_equalizer.block_length = block_length;
    m_equalizer.n_blocks = n_blocks;
    m_equalizer.realsize = realsize;
    m_equalizer.n_channels = n_channels;
    m_equalizer.sampling_rate = sampling_rate;

    m_equalizer.taps = 1 << n;

    m_convolver = new fftw_convolver(m_equalizer.block_length, m_equalizer.realsize, NULL);
    m_equalizer.ifftplan = m_convolver->create_fft_plan(log2_get(m_equalizer.taps), true, true);

    // Add two bands for zero and freq max
    m_equalizer.band_count = BAND_COUNT + 2;

    m_equalizer.freq[0] = 0.0;
    m_equalizer.freq[m_equalizer.band_count - 1] = (double)m_equalizer.sampling_rate / 2.0;

    for (n = 0; n < m_equalizer.band_count - 2; n++)
    {
        m_equalizer.freq[n + 1] = iso_bands[n];
    }
}

// Destructor for the class.
equalizer::~equalizer()
{
    delete m_convolver;
}

// Generates an equalizer coefficient file with the given parameters.
// Returns the name of the coefficient file.
//
// Parameters:
//   n_bands  the number of equalizer bands
//   freq     specifies the frequency of each band
//   mag      specifies the magnitude of each band
//   phase    specifies the phase of each band
//
// Returns:
//   The name of the equalizer coefficient file.
std::wstring
equalizer::generate(int n_bands,
                    double *freq,
                    double *mag,
                    double *phase)
{
    int n, i;
    std::wstring filename;

    if (n_bands > BAND_COUNT)
    {
        pinfo("Number of bands (%u) excceds limit (%u).", n_bands, BAND_COUNT);
        throw;
    }

    for (n = 0, i = 0; n < n_bands; n++)
    {
        while (freq[n] > m_equalizer.freq[i])
        {
            i++;
        }

        m_equalizer.mag[i] = mag[n];
        m_equalizer.phase[i] = phase[n];
        i++;
    }

    m_equalizer.mag[0] = m_equalizer.mag[1];
    m_equalizer.mag[m_equalizer.band_count - 1] = m_equalizer.mag[m_equalizer.band_count - 2];

    for (n = 0; n < m_equalizer.band_count; n++)
    {
        m_equalizer.freq[n] /= (double)m_equalizer.sampling_rate;
        m_equalizer.mag[n] = pow(10, m_equalizer.mag[n] / 20);
        m_equalizer.phase[n] /= (180 * M_PI);
    }

    // generate a filename representing the equalizer parameters
    filename = make_filename(n_bands, freq, mag, phase);

    // render the equalizer if the file does not already exist
    if (!boost::filesystem::exists(filename))
    {
        if (m_equalizer.realsize == 4)
        {
            render_f(&m_equalizer, filename.c_str());
        }
        else
        {
            render_d(&m_equalizer, filename.c_str());
        }
    }

    return filename;
}

// Generates a filename representing the given equalizer parameters.
//
// Parameters:
//   n_bands  the number of equalizer bands
//   freq     specifies the frequency of each band
//   mag      specifies the magnitude of each band
//   phase    specifies the phase of each band
//
// Returns:
//   A filename representing the equalizer parameters.
std::wstring
equalizer::make_filename(int n_bands,
                         double *freq,
                         double *mag,
                         double *phase)
{
    long hash_code;
    char *band_data;
    std::wstringstream out;

    // copy the band data into a single byte array
    band_data = (char *) _alloca(3 * n_bands * sizeof(double));
    memcpy(band_data, freq, n_bands * sizeof(double));
    memcpy(band_data + (n_bands * sizeof(double)), mag, n_bands * sizeof(double));
    memcpy(band_data + (2 * n_bands * sizeof(double)), phase, n_bands * sizeof(double));

    // generate a hash code of the bands array
    hash_code = DJBHash((char *)band_data, 3 * n_bands * sizeof(double));

    // assemble the filename
    out << "eq-" << std::hex << hash_code;
    out << "-" << std::dec << (m_equalizer.taps >> 1)
        << "-" << m_equalizer.realsize
        << "-" << m_equalizer.n_channels
        << "-" << m_equalizer.sampling_rate
        << ".wav";

    return bfir_path::append_temp_path(out.str());
}

float
equalizer::cosine_int_f(float mag1,
                        float mag2,
                        float freq1,
                        float freq2,
                        float curfreq)
{
    return (mag1 - mag2) * 0.5 *
        cos(M_PI * (curfreq - freq1) / (freq2 - freq1)) +
        (mag1 + mag2) * 0.5;
}

double
equalizer::cosine_int_d(double mag1,
                        double mag2,
                        double freq1,
                        double freq2,
                        double curfreq)
{
    return (mag1 - mag2) * 0.5 *
        cos(M_PI * (curfreq - freq1) / (freq2 - freq1)) +
        (mag1 + mag2) * 0.5;
}

// Renders the equalizer to the specified file (float version).
//
// Parameters:
//   eq        the equalizer parameters
//   filename  the filename to write to.
void
equalizer::render_f(struct equalizer_t *eq,
                    const wchar_t *filename)
{
    float mag, rad, curfreq, scale, divtaps, tapspi;
    float *eqmag, *eqfreq, *eqphase;
    int n, i;
    void *rbuf;
    void **bufs;

    rbuf = _aligned_malloc(eq->block_length * eq->n_blocks * eq->realsize, ALIGNMENT);

    // generate smoothed frequency domain filter
    eqmag = (float *) _alloca(eq->band_count * sizeof(float));
    eqfreq = (float *) _alloca(eq->band_count * sizeof(float));
    eqphase = (float *) _alloca(eq->band_count * sizeof(float));

    for (n = 0; n < eq->band_count; n++)
    {
        eqmag[n] = (float)eq->mag[n];
        eqfreq[n] = (float)eq->freq[n];
        eqphase[n] = (float)eq->phase[n];
    }

    scale = 1.0 / (float)eq->taps;
    divtaps = 1.0 / (float)eq->taps;
    tapspi = -(float)eq->taps * M_PI;
    ((float *)rbuf)[0] = eqmag[0] * scale;

    for (n = 1, i = 0; n < eq->taps >> 1; n++)
    {
        curfreq = (float)n * divtaps;
        while (curfreq > eqfreq[i + 1])
        {
            i++;
        }

        mag = cosine_int_f(eqmag[i], eqmag[i+1], eqfreq[i], eqfreq[i + 1],
                           curfreq) * scale;

        rad = tapspi * curfreq +
            cosine_int_f(eqphase[i], eqphase[i+1], eqfreq[i], eqfreq[i + 1],
                         curfreq);

        ((float *)rbuf)[n] = cos(rad) * mag;
        ((float *)rbuf)[eq->taps - n] = sin(rad) * mag;
    }

    ((float *)rbuf)[eq->taps >> 1] = eqmag[eq->band_count - 1] * scale;

    // convert to time-domain
    fftwf_execute_r2r((const fftwf_plan)eq->ifftplan,
                      (float *)rbuf, (float *)rbuf);

    if (filename != NULL)
    {
        bufs = (void **)_aligned_malloc(eq->n_channels * sizeof(void *), ALIGNMENT);

        for (n = 0; n < eq->n_channels; n++)
        {
            bufs[n] = _aligned_malloc((eq->taps >> 1) * eq->realsize, ALIGNMENT);

            // rbuf is in half-complex format, so only use the upper half of the buffer
            memcpy(bufs[n],
                   &(((float *)rbuf)[eq->taps >> 1]),
                   (eq->taps >> 1) * eq->realsize);
        }

        void *buffer = buffer::interlace(bufs, eq->n_channels, eq->taps >> 1, eq->realsize);

        buffer::save_to_snd_file(filename,
                                 buffer,
                                 eq->n_channels,
                                 eq->taps >> 1,
                                 eq->realsize,
                                 eq->sampling_rate);

        for (n = 0; n < eq->n_channels; n++)
        {
            _aligned_free(bufs[n]);
            bufs[n] = NULL;
        }

        _aligned_free(bufs);
        bufs = NULL;
    }

    _aligned_free(rbuf);
}

// Renders the equalizer to the specified file (double version).
//
// Parameters:
//   eq        the equalizer parameters
//   filename  the filename to write to.
void
equalizer::render_d(struct equalizer_t *eq,
                    const wchar_t *filename)
{
    double mag, rad, curfreq, scale, divtaps, tapspi;
    double *eqmag, *eqfreq, *eqphase;
    int n, i;
    void *rbuf;
    void **bufs;

    rbuf = _aligned_malloc(eq->block_length * eq->n_blocks * eq->realsize, ALIGNMENT);

    // generate smoothed frequency domain filter
    eqmag = (double *) _alloca(eq->band_count * sizeof(double));
    eqfreq = (double *) _alloca(eq->band_count * sizeof(double));
    eqphase = (double *) _alloca(eq->band_count * sizeof(double));

    for (n = 0; n < eq->band_count; n++)
    {
        eqmag[n] = (double)eq->mag[n];
        eqfreq[n] = (double)eq->freq[n];
        eqphase[n] = (double)eq->phase[n];
    }

    scale = 1.0 / (double)eq->taps;
    divtaps = 1.0 / (double)eq->taps;
    tapspi = -(double)eq->taps * M_PI;
    ((double *)rbuf)[0] = eqmag[0] * scale;

    for (n = 1, i = 0; n < eq->taps >> 1; n++)
    {
        curfreq = (double)n * divtaps;
        while (curfreq > eqfreq[i + 1])
        {
            i++;
        }

        mag = cosine_int_d(eqmag[i], eqmag[i+1], eqfreq[i], eqfreq[i + 1],
                           curfreq) * scale;

        rad = tapspi * curfreq +
            cosine_int_d(eqphase[i], eqphase[i+1], eqfreq[i], eqfreq[i + 1],
                         curfreq);

        ((double *)rbuf)[n] = cos(rad) * mag;
        ((double *)rbuf)[eq->taps - n] = sin(rad) * mag;
    }

    ((double *)rbuf)[eq->taps >> 1] = eqmag[eq->band_count - 1] * scale;

    // convert to time-domain
    fftw_execute_r2r((const fftw_plan)eq->ifftplan,
                     (double *)rbuf, (double *)rbuf);

    if (filename != NULL)
    {
        bufs = (void **)_aligned_malloc(eq->n_channels * sizeof(void *), ALIGNMENT);

        for (n = 0; n < eq->n_channels; n++)
        {
            bufs[n] = _aligned_malloc((eq->taps >> 1) * eq->realsize, ALIGNMENT);

            // rbuf is in half-complex format, so only use the upper half of the buffer
            memcpy(bufs[n],
                   &(((double *)rbuf)[eq->taps >> 1]),
                   (eq->taps >> 1) * eq->realsize);
        }

        void *buffer = buffer::interlace(bufs, eq->n_channels, eq->taps >> 1, eq->realsize);

        buffer::save_to_snd_file(filename,
                                 buffer,
                                 eq->n_channels,
                                 eq->taps >> 1,
                                 eq->realsize,
                                 eq->sampling_rate);

        for (n = 0; n < eq->n_channels; n++)
        {
            _aligned_free(bufs[n]);
            bufs[n] = NULL;
        }

        _aligned_free(bufs);
        bufs = NULL;
    }

    _aligned_free(rbuf);
}
