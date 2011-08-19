/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _PREFS_GEN_H_
#define _PREFS_GEN_H_

#include "common.h"

#define default_cfg_cli_enable       0
#define default_cfg_cli_port         3000
#define default_cfg_overflow_enable  0

// {D902F8AB-AB37-4322-B723-9F685B559DD4}
static const GUID guid_cfg_cli_enable =
{ 0xD902F8AB, 0xAB37, 0x4322, { 0xB7, 0x23, 0x9F, 0x68, 0x5B, 0x55, 0x9D, 0xD4 } };

// {C65C32AC-E647-4C21-A483-17DD06F6D8B6}
static const GUID guid_cfg_cli_port =
{ 0xC65C32AC, 0xE647, 0x4C21, { 0xA4, 0x83, 0x17, 0xDD, 0x06, 0xF6, 0xD8, 0xB6 } };

// {7F4E8298-5DC2-4124-8DE9-3EC2F56F5A42}
static const GUID guid_cfg_overflow_enable =
{ 0x7F4E8298, 0x5DC2, 0x4124, { 0x8D, 0xE9, 0x3E, 0xC2, 0xF5, 0x6F, 0x5A, 0x42 } };


class prefs_gen : public CDialogImpl<prefs_gen>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    prefs_gen(preferences_page_callback::ptr callback) : m_callback(callback) {}

    // Note that we don't bother doing anything regarding destruction of our class.
    // The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.

    // dialog resource ID
    enum { IDD = IDD_GENERAL };

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state();
    void apply();
    void reset();

    // WTL message map
    BEGIN_MSG_MAP(prefs_gen)
        MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_CHECK_CLI_ENABLE, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_EDIT_CLI_PORT, EN_CHANGE, OnFieldChange)
		COMMAND_HANDLER_EX(IDC_CHECK_OVERFLOW, BN_CLICKED, OnButtonClick)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
	void OnFieldChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;
};

#endif