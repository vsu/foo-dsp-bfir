/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _PREFS_EQ_H_
#define _PREFS_EQ_H_

#include "common.h"
#include "../brutefir/equalizer.hpp"

#define default_cfg_eq_enable   0
#define default_cfg_eq_level    0 
#define default_cfg_eq_mag      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"

// {5E352871-7A63-4036-B774-5CB1FFCBE9EE}
static const GUID guid_cfg_eq_enable =
{ 0x5E352871, 0x7A63, 0x4036, { 0xB7, 0x74, 0x5C, 0xB1, 0xFF, 0xCB, 0xE9, 0xEE } };

// {E74DA44B-D1D0-4C06-A3BB-12EBA6291B9F}
static const GUID guid_cfg_eq_level =
{ 0xE74DA44B, 0xD1D0, 0x4C06, { 0xA3, 0xBB, 0x12, 0xEB, 0xA6, 0x29, 0x1B, 0x9F } };

// {49E3777D-1A10-4F88-9B21-B206B93C60EF}
static const GUID guid_cfg_eq_mag =
{ 0x49E3777D, 0x1A10, 0x4F88, { 0x9B, 0x21, 0xB2, 0x06, 0xB9, 0x3C, 0x60, 0xEF } };


class prefs_eq : public CDialogImpl<prefs_eq>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    prefs_eq(preferences_page_callback::ptr callback) : m_callback(callback) {}

    // Note that we don't bother doing anything regarding destruction of our class.
    // The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.

    // dialog resource ID
    enum { IDD = IDD_EQUALIZER };

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state();
    void apply();
    void reset();
    static double get_scale();
    static void get_mag(double *mag);

    // WTL message map
    BEGIN_MSG_MAP(preferences_bfir_eq)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_VSCROLL(OnVScroll)
        COMMAND_HANDLER_EX(IDC_CHECK_EQ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_LOAD, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_SAVE, BN_CLICKED, OnButtonClick)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void OnButtonClick(UINT, int, CWindow);
    void LoadSettings();
    void ShowEqLevel();
    void ShowEqMag(int index);
    void LoadFile();
    void SaveFile();
    void WriteJson(PWSTR filename);
    BOOL ReadJson(PWSTR filename);
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;
    CTrackBarCtrl m_slider_eq_mag[BAND_COUNT];
    CTrackBarCtrl m_slider_eq_level;
};

#endif