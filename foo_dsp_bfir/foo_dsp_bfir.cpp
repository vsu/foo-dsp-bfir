/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"
#include <string>
#include <vector>
#include <malloc.h>
#include "foo_dsp_bfir.hpp"
#include "config.hpp"
#include "../brutefir/brutefir.hpp"
#include "../brutefir/equalizer.hpp"
#include "../brutefir/coeff.hpp"
#include "../brutefir/buffer.hpp"
#include "../brutefir/preprocessor.hpp"
#include "../brutefir/app_path.hpp"
#include "../brutefir/util.hpp"
#include "../brutefir/pinfo.h"

static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, 
                              dsp_preset_edit_callback & p_callback);

class initquit_bfir : public initquit
{
    void on_init()
    {
        // Prepare the application file path
        std::string app_path;
        app_path.append(core_api::get_profile_path());
        app_path.append("\\");
        app_path.append(core_api::get_my_file_name());

        // Remove the "file://" prefix
        app_path.erase(0, 7);

        // Convert file path to wchar_t
        int size = mbstowcs(NULL, app_path.c_str(), 0);
        wchar_t * app_path_wcs = (wchar_t *) _alloca((size + 1) * sizeof(wchar_t));
        mbstowcs(app_path_wcs, app_path.c_str(), size + 1);
        
        // Set application file path
        app_path::set_path(app_path_wcs);

        // Set print output callback
        set_print_callback(&console::print);
    }

    void on_quit()
    {
        // Clean up application file path
        app_path::clean_path();
    }
};

static initquit_factory_t<initquit_bfir> g_initquit_bfir_factory;

dsp_bfir::dsp_bfir(dsp_preset const & in)
    : m_channels(0), m_srate(0), m_buffer_count(0)
{
    // Initialize arrays
    memset(m_phase, 0, BAND_COUNT * sizeof(double));

    // Get saved settings
    get_preset(&m_params, in);

    // Initialize buffers.  These will be reallocated later when
    // the number of channels is determined.
    m_inbuf = (audio_sample *)  _aligned_malloc(1, ALIGNMENT);
    m_outbuf = (audio_sample *) _aligned_malloc(1, ALIGNMENT);
}

dsp_bfir::~dsp_bfir()
{
    // Free all allocated memory
    _aligned_free(m_outbuf);
    _aligned_free(m_inbuf);
    delete m_filter;
    delete m_equalizer;
}

GUID dsp_bfir::g_get_guid()
{
    // {B76B3F1F-FF50-437E-9ABC-88DDEEEDD5B9}
    static const GUID guid = { 0xB76B3F1F, 0xFF50, 0x437E, { 0x9A, 0xBC, 0x88, 0xDD, 0xEE, 0xED, 0xD5, 0xB9 } };
    return guid;
}

void dsp_bfir::g_get_name(pfc::string_base & p_out)
{
    p_out = COMPONENT_NAME;
}

bool dsp_bfir::on_chunk(audio_chunk * chunk, abort_callback & p_abort)
{
    bool first_init = false;
    bool re_init = false;

    // This block can be used to determine when a next track is started
    //metadb_handle::ptr curTrack;
    //if (get_cur_file(curTrack) && curTrack != m_lastTrack)
    //{
    //    m_lastTrack = curTrack;
    //}

    if (chunk->get_channels() != m_channels)
    {
        if (m_channels == 0)
        {
            first_init = true;
        }
        else
        {
            re_init = true;
        }

        m_channels = chunk->get_channels();
    }

    if (chunk->get_srate() != m_srate)
    {
        if (m_srate == 0)
        {
            first_init = true;
        }
        else
        {
            re_init = true;
        }

        m_srate = chunk->get_srate();
    }

    if (first_init || re_init)
    {
        // Free memory before re-initializing for settings change
        if (re_init)
        {
            console::print("Reinitializing filter.");
            delete m_filter;
            delete m_equalizer;
        }

        // Instantiate equalizer
        m_equalizer = new equalizer(FILTER_LEN,
                                    EQ_FILTER_BLOCKS, 
                                    REALSIZE, 
                                    m_channels, 
                                    m_srate);

        std::vector<struct impulse_info> impulse_info;
        struct impulse_info info;

        // Load equalizer
        if (m_params.eq_enabled != 0)
        {
            info.filename = m_equalizer->generate(ISO_BANDS_SIZE,
                                                    (double *) iso_bands,
                                                    m_params.mag,
                                                    m_phase);

            info.scale = get_scale(m_params.eq_slider_level);
            impulse_info.push_back(info);
        }

        // Load DRC impulse response files
        if ((m_params.file[0].enabled != 0) && (wcslen(m_params.file[0].filename) != 0))
        {
            if (buffer::check_snd_file(m_params.file[0].filename, m_channels, m_srate))
            {
                info.filename.assign(m_params.file[0].filename);
                info.scale = get_scale(m_params.file[0].slider_level);
                impulse_info.push_back(info);
            }
        }

        if ((m_params.file[1].enabled != 0) && (wcslen(m_params.file[1].filename) != 0))
        {
            if (buffer::check_snd_file(m_params.file[1].filename, m_channels, m_srate))
            {
                info.filename.assign(m_params.file[1].filename);
                info.scale = get_scale(m_params.file[1].slider_level);
                impulse_info.push_back(info);
            }
        }

        if ((m_params.file[2].enabled != 0) && (wcslen(m_params.file[2].filename) != 0))
        {
            if (buffer::check_snd_file(m_params.file[2].filename, m_channels, m_srate))
            {
                info.filename.assign(m_params.file[2].filename);
                info.scale = get_scale(m_params.file[2].slider_level);
                impulse_info.push_back(info);
            }
        }

        std::wstring filename = L"";
        double scale;

        if (impulse_info.size() == 1)
        {
            filename.assign(impulse_info.front().filename);
            scale = impulse_info.front().scale;
        }
        else if (impulse_info.size() > 1)
        {
            // Preconvolve impulse files into a single file
            filename = preprocessor::convolve_impulses(impulse_info, FILTER_LEN, REALSIZE);
            scale = 1.0;
        }

        if (!filename.empty())
        {
            int n_channels, n_frames, sampling_rate;
            
            // Get impulse file parameters
            if (buffer::get_snd_file_params(filename.c_str(), 
                                            &n_channels, 
                                            &n_frames, 
                                            &sampling_rate))
            {
                // calculate filter blocks
                int length = util::get_next_multiple(n_frames, FILTER_LEN);
                int filter_blocks = length / FILTER_LEN;
            
                // Instantiate filter
                m_filter = new brutefir(FILTER_LEN,
                                        filter_blocks,
                                        REALSIZE,
                                        m_channels, 
                                        BF_SAMPLE_FORMAT_FLOAT_LE, 
                                        BF_SAMPLE_FORMAT_FLOAT_LE, 
                                        m_srate, 
                                        false);
           
                // Assign filter coefficients
                m_filter->set_coeff(filename.c_str(), filter_blocks, scale);

                // Reallocate input and output buffers
                m_bufsize = FILTER_LEN * m_channels * sizeof(audio_sample);
                m_inbuf = (audio_sample *) _aligned_realloc(m_inbuf, m_bufsize, ALIGNMENT);
                m_outbuf = (audio_sample *) _aligned_realloc(m_outbuf, m_bufsize, ALIGNMENT);

                console::printf("Filter length: %u samples, %u blocks.", FILTER_LEN, filter_blocks);
                console::printf("Format: %u channels, %u Hz.", m_channels, m_srate);
            }
        }
    }

    // Check if initialization completed successfully
    if ((m_filter != NULL) && (m_filter->is_initialized()))
    {
        t_size sample_count = chunk->get_sample_count();
        const audio_sample *src = chunk->get_data();
        audio_sample *dst;

        while (sample_count)
        {
            unsigned int todo = FILTER_LEN - m_buffer_count;

            if (todo > sample_count)
            {
                todo = sample_count;
            }

            dst = m_inbuf + m_buffer_count * m_channels;

            for (unsigned int i = 0, j = todo * m_channels; i < j; i++)
            {
                *dst++ = *src++;
            }

            sample_count -= todo;
            m_buffer_count += todo;

            if (m_buffer_count == FILTER_LEN)
            {
                if (m_filter->run(m_inbuf, m_outbuf) == 0)
                {
                    audio_chunk *chk = insert_chunk(m_buffer_count * m_channels);
                    chk->set_data_32((float *)m_outbuf, m_buffer_count, m_channels, m_srate);

                    if (m_params.overflow_warnings != 0)
                    {
                        m_filter->check_overflows();
                    }
                }
                else
                {
                    console::print("Filter processing error.");
                }

                m_buffer_count = 0;
            }
        }
    }
    else
    {
        // If initialization error or no coefficients enabled, just 
        // pass audio chunk through
        return true;
    }

    // Return false since we replace the original audio chunk
    // with the processed ones
    return false;
}

void dsp_bfir::on_endofplayback(abort_callback & p_abort) {}
void dsp_bfir::on_endoftrack(abort_callback & p_abort) {}

void dsp_bfir::flush()
{
    m_buffer_count = 0;
}

double dsp_bfir::get_latency()
{
    return 0;
}

bool dsp_bfir::need_track_change_mark()
{
    return false;
}

bool dsp_bfir::g_get_default_preset(dsp_preset & p_out)
{
    p_out.set_owner(g_get_guid());

    struct dsp_bfir_param_t params;
    memset(&params, 0, sizeof(struct dsp_bfir_param_t));

    p_out.set_data(&params, sizeof(struct dsp_bfir_param_t));

    return true;
}

void dsp_bfir::g_show_config_popup(const dsp_preset & p_data, HWND p_parent, 
                                   dsp_preset_edit_callback & p_callback)
{
    ::RunDSPConfigPopup(p_data, p_parent, p_callback);
}

bool dsp_bfir::g_have_config_popup()
{
    return true;
}

void dsp_bfir::make_preset(struct dsp_bfir_param_t params, dsp_preset & out)
{
    out.set_owner(g_get_guid());
    out.set_data(&params, sizeof(struct dsp_bfir_param_t));
}

void dsp_bfir::get_preset(struct dsp_bfir_param_t *params, const dsp_preset & in)
{
    if ((in.get_owner() == g_get_guid()) &&
        (in.get_data_size() == sizeof(struct dsp_bfir_param_t)))
    {
        memcpy(params, in.get_data(), sizeof(struct dsp_bfir_param_t));
    }
    else
    {
        memset(params, 0, sizeof(struct dsp_bfir_param_t));
    }
}

double dsp_bfir::get_scale(int slider_level)
{
    return FROM_DB((double)slider_level / LEVEL_SLIDER_STEPS);
}

static dsp_factory_t<dsp_bfir> g_dsp_bfir_factory;

static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, 
                              dsp_preset_edit_callback & p_callback)
{
    CMyDSPPopup popup(p_data, p_callback);

    if (popup.DoModal(p_parent) != IDOK)
    {
        p_callback.on_preset_changed(p_data);
    }
}

DECLARE_COMPONENT_VERSION(COMPONENT_NAME, COMPONENT_VERSION, COMPONENT_NAME" v"COMPONENT_VERSION);
VALIDATE_COMPONENT_FILENAME("foo_dsp_bfir.dll");
