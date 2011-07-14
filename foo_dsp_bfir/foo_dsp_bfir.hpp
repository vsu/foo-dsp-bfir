/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _FOO_DSP_BFIR_HPP_
#define _FOO_DSP_BFIR_HPP_

#include "brutefir.hpp"
#include "equalizer.hpp"
#include "common.h"

class dsp_bfir : public dsp_impl_base
{
public:
    dsp_bfir(dsp_preset const & in);
    ~dsp_bfir();

    static GUID g_get_guid();
    static void g_get_name(pfc::string_base & p_out);
    bool on_chunk(audio_chunk * chunk, abort_callback & p_abort);
    void on_endofplayback(abort_callback & p_abort);
    void on_endoftrack(abort_callback & p_abort);
    void flush();
    double get_latency();
    bool need_track_change_mark();
    static bool g_get_default_preset(dsp_preset & p_out);
    static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, 
                                    dsp_preset_edit_callback & p_callback);
    static bool g_have_config_popup();
    static void make_preset(struct dsp_bfir_param_t params, dsp_preset & out);
    static void get_preset(struct dsp_bfir_param_t *params, const dsp_preset & in);
    static double get_scale(int slider_level);

private:
    brutefir *m_filter;
    equalizer *m_equalizer;

    unsigned int m_channels;
    unsigned int m_srate;

    unsigned int m_buffer_count;
    size_t m_bufsize;
    audio_sample *m_inbuf;
    audio_sample *m_outbuf;

    metadb_handle::ptr m_lastTrack;

    double m_phase[BAND_COUNT];
    struct dsp_bfir_param_t m_params;
};

#endif