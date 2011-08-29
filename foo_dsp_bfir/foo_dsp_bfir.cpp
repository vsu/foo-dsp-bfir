/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "common.h"
#include "foo_dsp_bfir.h"
#include "prefs_gen.h"
#include "prefs_eq.h"
#include "prefs_file.h"

#include <string>
#include <vector>
#include <malloc.h>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

#include "../brutefir/brutefir.hpp"
#include "../brutefir/equalizer.hpp"
#include "../brutefir/coeff.hpp"
#include "../brutefir/buffer.hpp"
#include "../brutefir/preprocessor.hpp"
#include "../brutefir/bfir_path.hpp"
#include "../brutefir/util.hpp"
#include "../brutefir/pinfo.h"
#include "../cli_server/server.hpp"

cli::server::server * cli_server;
boost::thread * cli_server_thread = NULL;

std::string app_path;


class initquit_bfir : public initquit
{
    void on_init()
    {
        // Prepare the application file path
        app_path.append(core_api::get_profile_path());
        app_path.append("\\");
        app_path.append(core_api::get_my_file_name());

        // Remove the "file://" prefix
        app_path.erase(0, 7);
        
        // Create te application file path
        boost::filesystem::create_directories(app_path);

        // Set BruteFIR file path
        bfir_path::set_path(util::str2wstr(app_path));

        // Set print output callback
        set_print_callback(&console::print);

        // Start command line interface server
        if (cfg_cli_enable.get_value() != 0)
        {
            g_start_server();
        }
    }

    void on_quit()
    {
        // Stop command line interface server
        g_stop_server();

        // Clean up BruteFIR file path
        bfir_path::clean_path();
    }
};

static initquit_factory_t<initquit_bfir> g_initquit_bfir_factory;


class dsp_bfir : public dsp_impl_base
{
public:
    dsp_bfir()
        : m_channels(0), m_srate(0), m_buffer_count(0), m_filter(NULL)
    {
        // Initialize arrays
        memset(m_phase, 0, BAND_COUNT * sizeof(double));

        // Initialize buffers.  These will be reallocated later when
        // the number of channels is determined.
        m_inbuf = (audio_sample *)  _aligned_malloc(1, ALIGNMENT);
        m_outbuf = (audio_sample *) _aligned_malloc(1, ALIGNMENT);
    }

    ~dsp_bfir()
    {
        // Free all allocated memory
        _aligned_free(m_outbuf);
        _aligned_free(m_inbuf);
        delete m_filter;
        delete m_equalizer;
    }

    bool on_chunk(audio_chunk * chunk, abort_callback & p_abort)
    {
        bool first_init = false;
        bool re_init = false;

        // This block can be used to determine when a new track is started
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
            if (cfg_eq_enable.get_value() != 0)
            {
                prefs_eq::get_mag(m_mag);

                info.filename = m_equalizer->generate(ISO_BANDS_SIZE,
                                                      (double *) iso_bands,
                                                      m_mag,
                                                      m_phase);

                info.scale = prefs_eq::get_scale();
                impulse_info.push_back(info);
            }

            std::wstring filename;

            // Load DRC impulse response files
            filename = util::str2wstr(cfg_file1_filename.get_ptr());

            if ((cfg_file1_enable.get_value() != 0) && (wcslen(filename.c_str()) != 0))
            {
                if (buffer::check_snd_file(filename.c_str(), m_channels, m_srate))
                {
                    info.filename.assign(filename);
                    info.scale = prefs_file::get_file1_scale();
                    impulse_info.push_back(info);
                }
            }

            filename = util::str2wstr(cfg_file2_filename.get_ptr());

            if ((cfg_file2_enable.get_value() != 0) && (wcslen(filename.c_str()) != 0))
            {
                if (buffer::check_snd_file(filename.c_str(), m_channels, m_srate))
                {
                    info.filename.assign(filename);
                    info.scale = prefs_file::get_file2_scale();
                    impulse_info.push_back(info);
                }
            }

            filename = util::str2wstr(cfg_file3_filename.get_ptr());

            if ((cfg_file3_enable.get_value() != 0) && (wcslen(filename.c_str()) != 0))
            {
                if (buffer::check_snd_file(filename.c_str(), m_channels, m_srate))
                {
                    info.filename.assign(filename);
                    info.scale = prefs_file::get_file3_scale();
                    impulse_info.push_back(info);
                }
            }

            filename = L"";
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
        if (m_filter != NULL)
        {
            if (m_filter->is_initialized())
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

                            if (cfg_overflow_enable.get_value() != 0)
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

    void on_endofplayback(abort_callback & p_abort) {}
    void on_endoftrack(abort_callback & p_abort) {}

    void flush()
    {
        m_buffer_count = 0;
    }

    double get_latency()
    {
        return 0;
    }

    bool need_track_change_mark()
    {
        return false;
    }

    static GUID g_get_guid()
    {
        // {B76B3F1F-FF50-437E-9ABC-88DDEEEDD5B9}
        static const GUID guid = { 0xB76B3F1F, 0xFF50, 0x437E, { 0x9A, 0xBC, 0x88, 0xDD, 0xEE, 0xED, 0xD5, 0xB9 } };
        return guid;
    }

    static void g_get_name(pfc::string_base & p_out)
    {
        p_out = COMPONENT_NAME;
    }

private:
    brutefir *m_filter;
    equalizer *m_equalizer;

    unsigned int m_channels;
    unsigned int m_srate;

    unsigned int m_buffer_count;
    size_t m_bufsize;
    audio_sample *m_inbuf;
    audio_sample *m_outbuf;

    double m_mag[BAND_COUNT];
    double m_phase[BAND_COUNT];

    metadb_handle::ptr m_lastTrack;
};

static dsp_factory_nopreset_t<dsp_bfir> g_dsp_bfir_factory;


static const GUID g_guid_bfir_branch =
{ 0x0A468D64, 0x1C8E, 0x4E0C, { 0x97, 0xC7, 0x36, 0xBA, 0xCD, 0xE6, 0x6A, 0x3E } };

static preferences_branch_factory g_mybranch(g_guid_bfir_branch, preferences_page::guid_playback, COMPONENT_NAME);

class preferences_page_gen : public preferences_page_impl<prefs_gen>
{
    // preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
    const char * get_name()
    {
        return "General";
    }

    GUID get_guid()
    {
        // {219D16AD-579E-4D77-9C94-85ACEA4C1E99}
        static const GUID guid = { 0x219D16AD, 0x579E, 0x4D77, { 0x9C, 0x94, 0x85, 0xAC, 0xEA, 0x4C, 0x1E, 0x99 } };
        return guid;
    }

    GUID get_parent_guid()
    {
        return g_guid_bfir_branch;
    }

    double get_sort_priority() 
    {
        return 0;
    }
};

static preferences_page_factory_t<preferences_page_gen> g_preferences_page_bfir_gen_factory;


class preferences_page_eq : public preferences_page_impl<prefs_eq>
{
    // preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
    const char * get_name()
    {
        return "Equalizer";
    }

    GUID get_guid()
    {
        // {933EE163-21FA-427F-A282-6CC55D3E1A4B}
        static const GUID guid = { 0x933EE163, 0x21FA, 0x427F, { 0xA2, 0x82, 0x6C, 0xC5, 0x5D, 0x3E, 0x1A, 0x4B } };
        return guid;
    }

    GUID get_parent_guid()
    {
        return g_guid_bfir_branch;
    }

    double get_sort_priority() 
    {
        return 1;
    }
};

static preferences_page_factory_t<preferences_page_eq> g_preferences_page_bfir_eq_factory;


class preferences_page_file : public preferences_page_impl<prefs_file>
{
    // preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
    const char * get_name()
    {
        return "Impulse Files";
    }

    GUID get_guid()
    {
        // {EE4CD899-26E5-45E9-ABCE-A7F718E7F7DD}
        static const GUID guid = { 0xEE4CD899, 0x26E5, 0x45E9, { 0xAB, 0xCE, 0xA7, 0xF7, 0x18, 0xE7, 0xF7, 0xDD } };
        return guid;
    }

    GUID get_parent_guid()
    {
        return g_guid_bfir_branch;
    }

    double get_sort_priority() 
    {
        return 2;
    }
};

static preferences_page_factory_t<preferences_page_file> g_preferences_page_bfir_file_factory;


void g_cli_server_thread_worker()
{
    cli_server = new cli::server::server("0.0.0.0", cfg_cli_port.get_value(), app_path);
    cli_server->run();
}

void g_start_server()
{
    cli_server_thread = new boost::thread(g_cli_server_thread_worker);
}

void g_stop_server()
{
    if (cli_server_thread != NULL)
    {
        cli_server->stop();
        cli_server_thread->join();
        cli_server_thread = NULL;
    }
}

void g_apply_preferences()
{
    g_stop_server();

    if (cfg_cli_enable.get_value() != 0)
    {
        g_start_server();
    }
}


DECLARE_COMPONENT_VERSION(COMPONENT_NAME, COMPONENT_VERSION, COMPONENT_NAME" v"COMPONENT_VERSION);
VALIDATE_COMPONENT_FILENAME("foo_dsp_bfir.dll");
