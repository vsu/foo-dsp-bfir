/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _PREFS_FILE_H_
#define _PREFS_FILE_H_

#include "common.h"

#define default_cfg_file1_enable     0
#define default_cfg_file2_enable     0
#define default_cfg_file3_enable     0
 
#define default_cfg_file1_level      0
#define default_cfg_file2_level      0
#define default_cfg_file3_level      0

// {29CCE08F-CB74-47EF-8EDD-6B823EDF726B}
static const GUID guid_cfg_file1_enable =
{ 0x29CCE08F, 0xCB74, 0x47EF, { 0x8E, 0xDD, 0x6B, 0x82, 0x3E, 0xDF, 0x72, 0x6B } };

// {B325912C-2DE4-4BF2-B9AC-F474732B33EB}
static const GUID guid_cfg_file2_enable =
{ 0xB325912C, 0x2DE4, 0x4BF2, { 0xB9, 0xAC, 0xF4, 0x74, 0x73, 0x2B, 0x33, 0xEB } };

// {0A468D64-1C8E-4E0C-97C7-36BACDE66A3E}
static const GUID guid_cfg_file3_enable =
{ 0x0A468D64, 0x1C8E, 0x4E0C, { 0x97, 0xC7, 0x36, 0xBA, 0xCD, 0xE6, 0x6A, 0x3E } };

// {E05EF49E-E643-48DA-B1C8-FD62EE0515BD}
static const GUID guid_cfg_file1_level =
{ 0xE05EF49E, 0xE642, 0x48DA, { 0xB1, 0xC8, 0xFD, 0x62, 0xEE, 0x05, 0x15, 0xBD } };

// {340A3A9D-FDD2-492D-82AB-8BE9069B8F3A}
static const GUID guid_cfg_file2_level =
{ 0x340A3A9D, 0xFDD2, 0x492D, { 0x82, 0xAB, 0x8B, 0xE9, 0x06, 0x9B, 0x8F, 0x3A } };

// {D970E188-14DC-4654-B3C0-4406883F5818}
static const GUID guid_cfg_file3_level =
{ 0xD970E188, 0x14DC, 0x4654, { 0xB3, 0xC0, 0x44, 0x06, 0x88, 0x3F, 0x58, 0x18 } };

// {71658593-A6DF-463F-8DE7-C66BCA542EBC}
static const GUID guid_cfg_file1_filename =
{ 0x71658593, 0xA6DF, 0x463F, { 0x8D, 0xE7, 0xC6, 0x6B, 0xCA, 0x54, 0x2E, 0xBC } };

// {4C886D48-607F-4164-BB74-80733BE66534}
static const GUID guid_cfg_file2_filename =
{ 0x4C886D48, 0x607F, 0x4164, { 0xBB, 0x74, 0x80, 0x73, 0x3B, 0xE6, 0x65, 0x34 } };

// {138FFBA6-8612-46B3-98BA-6CF7AC8F5221}
static const GUID guid_cfg_file3_filename =
{ 0x138FFBA6, 0x8612, 0x46B3, { 0x98, 0xBA, 0x6C, 0xF7, 0xAC, 0x8F, 0x52, 0x21 } };

// {1E473382-DCE3-4B43-A2EC-5786FBEE818D}
static const GUID guid_cfg_file1_metadata =
{ 0x1E473382, 0xDCE3, 0x4B43, { 0xA2, 0xEC, 0x57, 0x86, 0xFB, 0xEE, 0x81, 0x8D } };

// {0916C284-2865-4BF5-B5BC-8358A54C47F7}
static const GUID guid_cfg_file2_metadata =
{ 0x0916C284, 0x2865, 0x4BF5, { 0xB5, 0xBC, 0x83, 0x58, 0xA5, 0x4C, 0x47, 0xF7 } };

// {DE9C5CBB-A19C-423D-9D14-78AEA401AE1A}
static const GUID guid_cfg_file3_metadata =
{ 0xDE9C5CBB, 0xA19C, 0x423D, { 0x9D, 0x14, 0x78, 0xAE, 0xA4, 0x01, 0xAE, 0x1A } };


class prefs_file : public CDialogImpl<prefs_file>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    prefs_file(preferences_page_callback::ptr callback) : m_callback(callback) {}

    // Note that we don't bother doing anything regarding destruction of our class.
    // The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.

    // dialog resource ID
    enum { IDD = IDD_FILE };

    enum
    {
        FileLevelRangeMin = -20 * FILE_LEVEL_STEPS_PER_DB,
        FileLevelRangeMax = 20 * FILE_LEVEL_STEPS_PER_DB
    };

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state();
    void apply();
    void reset();
    static double get_file1_scale();
    static double get_file2_scale();
    static double get_file3_scale();

    // WTL message map
    BEGIN_MSG_MAP(prefs_file)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_HSCROLL(OnHScroll)
        COMMAND_HANDLER_EX(IDC_CHECK_FILE1, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CHECK_FILE2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CHECK_FILE3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_BROWSE1, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_BROWSE2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_BROWSE3, BN_CLICKED, OnButtonClick)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void OnButtonClick(UINT, int, CWindow);
    void LoadFile(int filename_id, int metadata_id, int level_id, int enable_id);
    void RefreshFileLevelLabel();
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;
    CTrackBarCtrl m_slider_level1, m_slider_level2, m_slider_level3;
    CEdit m_edit_file1, m_edit_file2, m_edit_file3;
};

#endif