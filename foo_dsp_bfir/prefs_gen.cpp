/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "prefs_gen.h"

cfg_int cfg_cli_port(guid_cfg_cli_port, default_cfg_cli_port);
cfg_int cfg_overflow_enable(guid_cfg_overflow_enable, default_cfg_overflow_enable);

BOOL prefs_gen::OnInitDialog(CWindow, LPARAM)
{
    ::SendMessage(GetDlgItem(IDC_EDIT_CLI_PORT), EM_SETLIMITTEXT, 5, 0 );
    SetDlgItemInt(IDC_EDIT_CLI_PORT, cfg_cli_port, FALSE);

    CheckDlgButton(IDC_CHECK_OVERFLOW, cfg_overflow_enable);

    return FALSE;
}

void prefs_gen::OnFieldChange(UINT, int, CWindow)
{
    OnChanged();
}

void prefs_gen::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

t_uint32 prefs_gen::get_state()
{
    t_uint32 state = preferences_state::resettable;
    if (HasChanged()) state |= preferences_state::changed;
    return state;
}

void prefs_gen::reset()
{
    SetDlgItemInt(IDC_EDIT_CLI_PORT, default_cfg_cli_port, FALSE);
    CheckDlgButton(IDC_CHECK_OVERFLOW, default_cfg_overflow_enable);

    OnChanged();
}

void prefs_gen::apply()
{
    cfg_cli_port = GetDlgItemInt(IDC_EDIT_CLI_PORT, NULL, FALSE);
    cfg_overflow_enable = IsDlgButtonChecked(IDC_CHECK_OVERFLOW);

    //g_apply_preferences();

    OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool prefs_gen::HasChanged()
{
    return
        (GetDlgItemInt(IDC_EDIT_CLI_PORT, NULL, FALSE) != cfg_cli_port) ||
        (IsDlgButtonChecked(IDC_CHECK_OVERFLOW) != cfg_overflow_enable);
}

void prefs_gen::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}