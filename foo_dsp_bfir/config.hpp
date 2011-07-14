/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "common.h"
#include "foo_dsp_bfir.hpp"

#define LEVEL_SLIDER_STEPS  10

class CMyDSPPopup : public CDialogImpl<CMyDSPPopup>
{
public:
    CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback);

    enum { IDD = IDD_CONFIG };

    enum
    {
        EQRangeMin = -10,
        EQRangeMax = 10,
        EQLevelRangeMin = -20 * LEVEL_SLIDER_STEPS,
        EQLevelRangeMax = 20 * LEVEL_SLIDER_STEPS,
        FileLevelRangeMin = -20 * LEVEL_SLIDER_STEPS,
        FileLevelRangeMax = 20 * LEVEL_SLIDER_STEPS
    };

    BEGIN_MSG_MAP(CMyDSPPopup)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButtonOkCancel)
    COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButtonOkCancel)
    COMMAND_HANDLER_EX(IDRESET, BN_CLICKED, OnButtonReset)
    COMMAND_HANDLER_EX(IDLOAD, BN_CLICKED, OnButtonLoad)
    COMMAND_HANDLER_EX(IDSAVE, BN_CLICKED, OnButtonSave)
    COMMAND_HANDLER_EX(IDBROWSE1, BN_CLICKED, OnButtonBrowse1)
    COMMAND_HANDLER_EX(IDBROWSE2, BN_CLICKED, OnButtonBrowse2)
    COMMAND_HANDLER_EX(IDBROWSE3, BN_CLICKED, OnButtonBrowse3)
    MSG_WM_VSCROLL(OnVScroll)
    MSG_WM_HSCROLL(OnHScroll)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnButtonOkCancel(UINT, int id, CWindow);
    void OnButtonReset(UINT, int id, CWindow);
    void OnButtonLoad(UINT, int id, CWindow);
    void OnButtonSave(UINT, int id, CWindow);
    void OnButtonBrowse1(UINT, int id, CWindow);
    void OnButtonBrowse2(UINT, int id, CWindow);
    void OnButtonBrowse3(UINT, int id, CWindow);
    void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void SaveParameters(struct dsp_bfir_param_t *params);
    void LoadParameters(struct dsp_bfir_param_t params);
    void UpdatePreset(struct dsp_bfir_param_t params);
    void RefreshEqLevelLabel();
    void RefreshFileLevelLabel();
    void FileBrowseSave();
    void FileBrowseLoad();
    void FileBrowseImpulse(int index);
    void WriteJson(PWSTR filename, struct dsp_bfir_param_t params);
    BOOL ReadJson(PWSTR filename, struct dsp_bfir_param_t *params);

    const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
    dsp_preset_edit_callback & m_callback;
    struct dsp_bfir_param_t m_params;

    CTrackBarCtrl m_slider_eq[BAND_COUNT];
    CTrackBarCtrl m_slider_eq_level, m_slider_level1, m_slider_level2, m_slider_level3;
    CButton m_checkbox_eq, m_checkbox_enable1, m_checkbox_enable2, 
            m_checkbox_enable3, m_checkbox_overflow;
    CEdit m_edit_file1, m_edit_file2, m_edit_file3;
};

#endif