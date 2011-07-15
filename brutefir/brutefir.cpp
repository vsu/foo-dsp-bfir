/*
 * (c) 2011 Victor Su
 * (c) 2001-2006 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <malloc.h>
#include <string.h>
#include <float.h>

#include "global.h"
#include "brutefir.hpp"
#include "fftw_convolver.hpp"
#include "dither.hpp"
#include "coeff.hpp"
#include "buffer.hpp"
#include "pinfo.h"

// Constructor for the class.
brutefir::brutefir(int filter_length,
                   int filter_blocks,
                   int realsize,
                   int channels,
                   int in_format,
                   int out_format,
                   int sampling_rate,
                   bool_t apply_dither)
    : m_initialized(false), bfconf(NULL), baseptr(NULL), m_convolver(NULL), m_dither(NULL)
{
    bfconf = (struct bfconf_t *) malloc(sizeof(struct bfconf_t));
    memset(bfconf, 0, sizeof(struct bfconf_t));

    if (init_channels(channels, in_format, out_format, sampling_rate, apply_dither) == 0)
    {
        if (init_convolver(filter_length, filter_blocks, realsize) == 0)
        {
            if (init_buffers() == 0)
            {
                reset();
            }
        }
    }
}

// Destructor for the class.
brutefir::~brutefir()
{
    free_buffers();
    free_coeff();

    // free objects
    delete m_convolver;
    delete m_dither;

    // free configuration structure
    if (bfconf != NULL)
    {
        free(bfconf);
        bfconf = NULL;
    }
}

// Returns a value indicating whether the filter is initialized.
//
// Returns:
//   True if initialized, false otherwise.
bool_t
brutefir::is_initialized()
{
    return m_initialized;
}

// Sets coefficients from the specified sound file.
//
// Supported formats are any that the libsndfile library
// can process.
//
// Parameters:
//   filename      the coefficient filename
//   coeff_blocks  the number of coefficient blocks
//   scale         the scaling factor
//
// Returns:
//   the number of channels extracted
//    -1 if incompatible file
//    -2 if coefficients could not be loaded
int
brutefir::set_coeff(const wchar_t *filename,
                    int coeff_blocks,
                    double scale)
{
    int n;
    int n_coeffs;
    int length;
    void **coeffs;

    // check compatibility of sound file
    if (!buffer::check_snd_file(filename, bfconf->n_channels, bfconf->sampling_rate))
    {
        pinfo("Incompatible file %s: format %u channels %u Hz.",
              filename,
              bfconf->n_channels,
              bfconf->sampling_rate);

        return -1;
    }

    // load the coefficients
    coeffs = coeff::load_snd_coeff(filename,
                                   &length,
                                   bfconf->realsize,
                                   coeff_blocks * bfconf->filter_length,
                                   &n_coeffs);

    if (coeffs == NULL)
    {
        pinfo("Error loading coefficients from sound file %s.", filename);
        return -2;
    }

    // free existing coefficient memory
    free_coeff();

    if (n_coeffs > bfconf->n_channels)
    {
        n_coeffs = bfconf->n_channels;
    }

    for (n = 0; n < n_coeffs; n++)
    {
        // preprocess coefficients
        bfconf->coeffs[n].data = coeff::preprocess_coeff(m_convolver,
                                                         coeffs[n],
                                                         bfconf->filter_length,
                                                         coeff_blocks,
                                                         length,
                                                         bfconf->realsize,
                                                         scale);

        _aligned_free(coeffs[n]);
        coeffs[n] = NULL;

        if (bfconf->coeffs[n].data == NULL)
        {
            pinfo("Error preprocessing coefficient %u from sound file %s.", n, filename);
            break;
        }

        bfconf->coeffs[n].n_blocks = coeff_blocks;
        bfconf->coeffs[n].intname = n;
        bfconf->coeffs[n].n_channels = 1;
        bfconf->coeffs[n].channels[0] = n;
    }

    if (n < n_coeffs)
    {
        // Free coefficient memory on error
        free_coeff();
        return -2;
    }

    m_initialized = true;
    return n_coeffs;
}

// Overload function to set coefficients from the given data array.
//
// Parameters:
//   coeffs        buffers of coefficients
//   n_coeffs      the number of coefficient buffers
//   length        the length of each buffer
//   coeff_blocks  the number of coefficient blocks
//   scale         the scaling factor
//
// Returns:
//    0 if successful
//   -1 if coefficients could not be loaded
int
brutefir::set_coeff(void **coeffs,
                    int n_coeffs,
                    int length,
                    int coeff_blocks,
                    double scale)
{
    int n;

    // free existing coefficient memory
    free_coeff();

    if (n_coeffs > bfconf->n_channels)
    {
        n_coeffs = bfconf->n_channels;
    }

    for (n = 0; n < n_coeffs; n++)
    {
        // preprocess coefficients
        bfconf->coeffs[n].data = coeff::preprocess_coeff(m_convolver,
                                                        coeffs[n],
                                                        bfconf->filter_length,
                                                        coeff_blocks,
                                                        length,
                                                        bfconf->realsize,
                                                        scale);

        if (bfconf->coeffs[n].data == NULL)
        {
            pinfo("Error preprocessing coefficient %u", n);
            break;
        }

        bfconf->coeffs[n].n_blocks = coeff_blocks;
        bfconf->coeffs[n].intname = n;
        bfconf->coeffs[n].n_channels = 1;
        bfconf->coeffs[n].channels[0] = n;
    }

    if (n < n_coeffs)
    {
        // Free coefficient memory on error
        free_coeff();
        return -2;
    }

    m_initialized = true;
    return 0;
}

// Performs filter processing on the specified input buffer.
//
// Filtered data is returned in the output buffer.
//
// Input and output buffers must be of the size specified
// in the constructor.
//
// Parameters:
//   inbuf   the input buffer
//   outbuf  the output buffer
//
// Returns:
//    0 if successful
//   -1 if invalid values are detected
int
brutefir::run(void *inbuf,
              void *outbuf)
{
    int n, i;
    int convblock;
    struct bfoverflow_t of;

    for (n = 0; n < bfconf->n_channels; n++)
    {
        // convert inputs
        m_convolver->convolver_raw2cbuf(inbuf,
                                        input_timecbuf[n][curbuf],
                                        input_timecbuf[n][!curbuf],
                                        &bfconf->inputs[n].bf,
                                        NULL,
                                        NULL);

        // transform to frequency domain
        m_convolver->convolver_time2freq(input_timecbuf[n][curbuf], input_freqcbuf[n]);

        if (procblocks[n] < bfconf->n_blocks)
        {
            procblocks[n]++;
        }

        curblock = (int)(blockcounter % (unsigned int)bfconf->n_blocks);

        // mix and scale inputs prior to convolution
        m_convolver->convolver_mixnscale(&input_freqcbuf[n],
                                         cbuf[n][curblock],
                                         &bfconf->inputs[n].bf.sf.scale,
                                         1,
                                         CONVOLVER_MIXMODE_INPUT);

        if (bfconf->n_blocks == 1)
        {
            // curblock is always zero and cbuf points at ocbuf when n_blocks == 1
            m_convolver->convolver_convolve_inplace(cbuf[n][0],
                                                    bfconf->coeffs[n].data[0]);
        }
        else
        {
            // run convolver
            m_convolver->convolver_convolve(cbuf[n][curblock],
                                            bfconf->coeffs[n].data[0],
                                            ocbuf[n]);

            for (i = 1; i < bfconf->coeffs[n].n_blocks && i < procblocks[n]; i++)
            {
                convblock = (int)((blockcounter - i) % (unsigned int)bfconf->n_blocks);

                m_convolver->convolver_convolve_add(cbuf[n][convblock],
                                                    bfconf->coeffs[n].data[i],
                                                    ocbuf[n]);
            }
        }

        // mix and scale convolve outputs prior to conversion to time domain.
        m_convolver->convolver_mixnscale(&ocbuf[n],
                                         output_freqcbuf[n],
                                         &bfconf->outputs[n].bf.sf.scale,
                                         1,
                                         CONVOLVER_MIXMODE_OUTPUT);

        // transform back to time domain
        // ocbuf[0] happens to be free, that's why we use it
        m_convolver->convolver_freq2time(output_freqcbuf[n], ocbuf[0]);

        // Check if there is NaN or Inf values, and abort if so. We cannot
        // afford to check all values, but NaN/Inf tend to spread, so
        // checking only one value usually catches the problem.
        if ((bfconf->realsize == sizeof(float) && !_finite((double)((float *) ocbuf[0])[0])) ||
            (bfconf->realsize == sizeof(double) && !_finite(((double *) ocbuf[0])[0])))
        {
            pinfo("NaN or Inf values in the system! Invalid input? Aborting.\n");
            return -1;
        }

        // write to output buffer
        of = overflow[n];

        m_convolver->convolver_cbuf2raw(ocbuf[0],
                                        outbuf,
                                        &bfconf->outputs[n].bf,
                                        bfconf->outputs[n].apply_dither,
                                        &bfconf->dither_state[n],
                                        &of);

        overflow[n] = of;
   }

    // swap convolve buffers
    curbuf = !curbuf;

    // advance input block
    blockcounter++;

    return 0;
}

// Resets the filter state.
void
brutefir::reset()
{
    int n;

    for (n = 0; n < bfconf->n_channels; n++)
    {
        overflow[n].n_overflows = 0;
        overflow[n].largest = 0;
        overflow[n].intlargest = 0;

        last_overflow[n].n_overflows = 0;
        last_overflow[n].largest = 0;
        last_overflow[n].intlargest = 0;
    }

    memset(procblocks, 0, BF_MAXCHANNELS * sizeof(int));

    curbuf = 0;
    curblock = 0;
    blockcounter = 0;
}

// Checks for overflow and print on any change.
void
brutefir::check_overflows()
{
    int n;

    for (n = 0; n < bfconf->n_channels; n++)
    {
        if (memcmp(&overflow[n], &last_overflow[n], sizeof(struct bfoverflow_t)) != 0)
        {
            for (; n < bfconf->n_channels; n++)
            {
                last_overflow[n] = overflow[n];
            }

            print_overflows();
            break;
        }
    }
}

// Gets the full scale value for specified sample size.
//
// Parameters:
//   bytes  the sample size in bytes (integer type samples)
//
// Returns:
//   The full scale value.
double
brutefir::get_full_scale(int bytes)
{
    return (double)(1 << ((bytes << 3) - 1));
}

// Gets the normalizing value for specified sample size.
//
// Parameters:
//   bytes  the sample size in bytes (integer type samples)
//
// Returns:
//   The normalizing value.
double
brutefir::get_normalized_scale(int bytes)
{
    return 1.0 / get_full_scale(bytes);
}

// Gets the maximum value for specified sample size.
//
// Parameters:
//   bytes  the sample size in bytes (integer type samples)
//
// Returns:
//   The maximum value.
double
brutefir::get_max(int bytes)
{
    return get_full_scale(bytes) - 1;
}

// Populates the sample format structure.
//
// Parameters:
//   format      the sample format code
//   sf          the sample format structure
//   normalized  true if sample scale should be normalized
void
brutefir::setup_sample_format(int format,
                              struct sample_format_t *sf,
                              bool_t normalized)
{
    sf->format = format;

    switch(format)
    {
    case BF_SAMPLE_FORMAT_S8:
        sf->bytes = 1;
        sf->sbytes = 1;
        sf->isfloat = false;
        sf->swap = false;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S16_LE:
        sf->bytes = 2;
        sf->sbytes = 2;
        sf->isfloat = false;
        sf->swap = false;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S16_BE:
        sf->bytes = 2;
        sf->sbytes = 2;
        sf->isfloat = false;
        sf->swap = true;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S24_LE:
        sf->bytes = 3;
        sf->sbytes = 3;
        sf->isfloat = false;
        sf->swap = false;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S24_BE:
        sf->bytes = 3;
        sf->sbytes = 3;
        sf->isfloat = false;
        sf->swap = true;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S32_LE:
        sf->bytes = 4;
        sf->sbytes = 4;
        sf->isfloat = false;
        sf->swap = false;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_S32_BE:
        sf->bytes = 4;
        sf->sbytes = 4;
        sf->isfloat = false;
        sf->swap = true;
        sf->scale = normalized
            ? get_normalized_scale(sf->bytes) : get_full_scale(sf->bytes);
        break;

    case BF_SAMPLE_FORMAT_FLOAT_LE:
        sf->bytes = 4;
        sf->sbytes = 4;
        sf->isfloat = true;
        sf->swap = false;
        sf->scale = 1.0;
        break;

    case BF_SAMPLE_FORMAT_FLOAT_BE:
        sf->bytes = 4;
        sf->sbytes = 4;
        sf->isfloat = true;
        sf->swap = true;
        sf->scale = 1.0;
        break;

    case BF_SAMPLE_FORMAT_FLOAT64_LE:
        sf->bytes = 8;
        sf->sbytes = 8;
        sf->isfloat = true;
        sf->swap = false;
        sf->scale = 1.0;
        break;

    case BF_SAMPLE_FORMAT_FLOAT64_BE:
        sf->bytes = 8;
        sf->sbytes = 8;
        sf->isfloat = true;
        sf->swap = true;
        sf->scale = 1.0;
        break;
    }
}

// Sets up channel input parameters.
//
// Parameters:
//   index   the input channel index
//   format  the sample format code
void
brutefir::setup_input(int index,
                      int format)
{
    struct buffer_format_t *bf;

    bfconf->inputs[index].intname = index;

    bf = &bfconf->inputs[index].bf;

    setup_sample_format(format, &bf->sf, true);
    bf->byte_offset = index * bf->sf.bytes;
    bf->sample_spacing = bfconf->n_channels;
}

// Sets up channel output parameters.
//
// Parameters:
//   index         the output channel index
//   format        the sample format code
//   apply_dither  true to apply dither (integer type samples only)
void
brutefir::setup_output(int index,
                       int format,
                       bool_t apply_dither)
{
    struct buffer_format_t *bf;

    bfconf->outputs[index].intname = index;
    bfconf->outputs[index].apply_dither = apply_dither;

    bf = &bfconf->outputs[index].bf;

    setup_sample_format(format, &bf->sf, false);
    bf->byte_offset = index * bf->sf.bytes;
    bf->sample_spacing = bfconf->n_channels;
}

// Prints overflow warnings.
void
brutefir::print_overflows(void)
{
    bool_t is_overflow = false;
    double peak;
    int n;

    for (n = 0; n < bfconf->n_channels; n++)
    {
        if (overflow[n].n_overflows > 0)
        {
            is_overflow = true;
            break;
        }
    }

    if (!is_overflow)
    {
        return;
    }

    for (n = 0; n < bfconf->n_channels; n++)
    {
        peak = overflow[n].largest;

        if (peak < (double)overflow[n].intlargest)
        {
            peak = (double)overflow[n].intlargest;
        }

        if (peak != 0.0)
        {
            if ((peak = 20.0 * log10(peak / overflow[n].max)) == 0.0)
            {
                peak = -0.0; // we want to display -0.0 rather than +0.0
            }

            pinfo("peak: %d/%u/%+.2f ", n, overflow[n].n_overflows, peak);
        }
        else
        {
            pinfo("peak: %d/%u/-Inf ", n, overflow[n].n_overflows);
        }
    }
}

// Initializes channels.
//
// Parameters:
//   n_channels     the number of channels
//   in_format      the input format code
//   out_format     the output format code
//   sampling_rate  the sampling rate
//   apply_dither   true to apply dither to output
//
// Returns:
//    0 if successful
//   -1 if channel limit is exceeded
int
brutefir::init_channels(int n_channels,
                        int in_format,
                        int out_format,
                        int sampling_rate,
                        bool_t apply_dither)
{
    int n;

    if (n_channels > BF_MAXCHANNELS)
    {
        pinfo("Number of channels (%u) exceeds limit (%u).", n_channels, BF_MAXCHANNELS);
        return -1;
    }

    bfconf->sampling_rate = sampling_rate;
    bfconf->n_channels = n_channels;

    // setup inputs and outputs
    for (n = 0; n < bfconf->n_channels; n++)
    {
        setup_input(n, in_format);
        setup_output(n, out_format, apply_dither);
    }

    // initialize overflow structure
    memset(overflow, 0, sizeof(struct bfoverflow_t) * bfconf->n_channels);
    memset(last_overflow, 0, sizeof(struct bfoverflow_t) * bfconf->n_channels);

    for (n = 0; n < bfconf->n_channels; n++)
    {
        if (bfconf->outputs[n].bf.sf.isfloat)
        {
            overflow[n].max = 1.0;
            last_overflow[n].max = 1.0;
        }
        else
        {
            overflow[n].max = get_max(bfconf->outputs[n].bf.sf.bytes);
            last_overflow[n].max = get_max(bfconf->outputs[n].bf.sf.bytes);
        }
    }

    return 0;
}

// Initializes the convolver.
//
// Parameters:
//   filter_length  the filter block length
//   filter_blocks  the number of filter blocks
//   realsize       the "float" size
//
// Returns:
//    0 if successful
//   -1 if convolved cannot be initialized
int
brutefir::init_convolver(int filter_length,
                         int filter_blocks,
                         int realsize)
{
    bfconf->filter_length = filter_length;
    bfconf->n_blocks = filter_blocks;
    bfconf->realsize = realsize;

    // initialize dither array
    m_dither = new dither(bfconf->n_channels,
                          bfconf->sampling_rate,
                          bfconf->realsize,
                          bfconf->max_dither_table_size,
                          bfconf->filter_length,
                          bfconf->dither_state);

    // initialize convolver
    try
    {
        m_convolver = new fftw_convolver(bfconf->filter_length,
                                         bfconf->realsize,
                                         m_dither);
    }
    catch (...)
    {
        pinfo("Error initializing convolver.");
        return -1;
    }

    convbufsize = m_convolver->convolver_cbufsize();
    return 0;
}

// Initializes buffers.
//
// Returns:
//    0 if successful
//   -1 if no channels are defined
int
brutefir::init_buffers()
{
    int n, i;
    int memsize;
    uint8_t *memptr;

    if (bfconf->n_channels == 0)
    {
        pinfo("No channels defined.");
        return -1;
    }

    // allocate void *cbuf[n_channels][n_blocks]
    for (n = 0; n < bfconf->n_channels; n++)
    {
        cbuf[n] = (void **) _aligned_malloc(bfconf->n_blocks * sizeof(void *), ALIGNMENT);
    }

    // allocate input/output convolve buffers
    memsize = bfconf->n_channels * convbufsize +         // ocbuf
              2 * bfconf->n_channels * convbufsize +     // input_timecbuf
              bfconf->n_channels * convbufsize +         // input_freqcbuf
              bfconf->n_channels * convbufsize;          // output_freqcbuf

    if (bfconf->n_blocks > 1)
    {
        memsize += bfconf->n_channels * bfconf->n_blocks * convbufsize;  // cbuf
    }

    baseptr = (uint8_t *) _aligned_malloc(memsize, ALIGNMENT);
    memset(baseptr, 0, memsize);

    memptr = baseptr;

    if (bfconf->n_blocks > 1)
    {
        for (n = 0; n < bfconf->n_channels; n++)
        {
            for (i = 0; i < bfconf->n_blocks; i++)
            {
                cbuf[n][i] = memptr;
                memptr += convbufsize;
            }

            ocbuf[n] = memptr;
            memptr += convbufsize;
        }
    }
    else
    {
        for (n = 0; n < bfconf->n_channels; n++)
        {
            cbuf[n][0] = ocbuf[n] = memptr;
            memptr += convbufsize;
        }
    }

    for (n = 0; n < bfconf->n_channels; n++)
    {
        input_timecbuf[n][0] = memptr;
        input_timecbuf[n][1] = memptr + convbufsize;
        memptr += 2 * convbufsize;

        input_freqcbuf[n] = memptr;
        memptr += convbufsize;

        output_freqcbuf[n] = memptr;
        memptr += convbufsize;
    }

    return 0;
}

// Releases buffer memory.
void
brutefir::free_buffers()
{
    int n;

    if (baseptr != NULL)
    {
        _aligned_free(baseptr);
        baseptr = NULL;
    }

    for (n = 0; n < bfconf->n_channels; n++)
    {
        if (cbuf[n] != NULL)
        {
            _aligned_free(cbuf[n]);
            cbuf[n] = NULL;
        }
    }
}

// Releases coefficient memory.
void
brutefir::free_coeff()
{
    int n, i;

    for (n = 0; n < bfconf->n_channels; n++)
    {
        if (bfconf->coeffs[n].data != NULL)
        {
            for (i = 0; i < bfconf->coeffs[n].n_blocks; i++)
            {
                if (bfconf->coeffs[n].data[i] != NULL)
                {
                    _aligned_free(bfconf->coeffs[n].data[i]);
                    bfconf->coeffs[n].data[i] = NULL;
                }
            }

            _aligned_free(bfconf->coeffs[n].data);
            bfconf->coeffs[n].data = NULL;
        }
    }

    m_initialized = false;
}

