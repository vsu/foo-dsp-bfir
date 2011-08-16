/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "prefs_file.h"
#include <string>
#include <sstream>
#include "../brutefir/util.hpp"
#include "../brutefir/preprocessor.hpp"

cfg_int cfg_file1_enable(guid_cfg_file1_enable, default_cfg_file1_enable);
cfg_int cfg_file2_enable(guid_cfg_file2_enable, default_cfg_file2_enable);
cfg_int cfg_file3_enable(guid_cfg_file3_enable, default_cfg_file3_enable);

cfg_int cfg_file1_level(guid_cfg_file1_level, default_cfg_file1_level);
cfg_int cfg_file2_level(guid_cfg_file2_level, default_cfg_file2_level);
cfg_int cfg_file3_level(guid_cfg_file3_level, default_cfg_file3_level);

cfg_string cfg_file1_filename(guid_cfg_file1_filename, "");
cfg_string cfg_file2_filename(guid_cfg_file2_filename, "");
cfg_string cfg_file3_filename(guid_cfg_file3_filename, "");

cfg_string cfg_file1_metadata(guid_cfg_file1_metadata, "");
cfg_string cfg_file2_metadata(guid_cfg_file2_metadata, "");
cfg_string cfg_file3_metadata(guid_cfg_file3_metadata, "");

BOOL prefs_file::OnInitDialog(CWindow, LPARAM)
{
    CheckDlgButton(IDC_CHECK_FILE1, cfg_file1_enable);
    CheckDlgButton(IDC_CHECK_FILE2, cfg_file2_enable);
    CheckDlgButton(IDC_CHECK_FILE3, cfg_file3_enable);

    m_slider_level1 = GetDlgItem(IDC_SLIDER_LEVEL1);
    m_slider_level1.SetRange(FileLevelRangeMin, FileLevelRangeMax);
    m_slider_level1.SetPos(cfg_file1_level);

    m_slider_level2 = GetDlgItem(IDC_SLIDER_LEVEL2);
    m_slider_level2.SetRange(FileLevelRangeMin, FileLevelRangeMax);
    m_slider_level2.SetPos(cfg_file2_level);

    m_slider_level3 = GetDlgItem(IDC_SLIDER_LEVEL3);
    m_slider_level3.SetRange(FileLevelRangeMin, FileLevelRangeMax);
    m_slider_level3.SetPos(cfg_file3_level);

    // Convert filenames to wchar_t
    std::wstring filename1 = util::str2wstr(cfg_file1_filename.get_ptr());
    std::wstring filename2 = util::str2wstr(cfg_file2_filename.get_ptr());
    std::wstring filename3 = util::str2wstr(cfg_file3_filename.get_ptr());

    SetDlgItemText(IDC_EDIT_FILE1, filename1.c_str());
    SetDlgItemText(IDC_EDIT_FILE2, filename2.c_str());
    SetDlgItemText(IDC_EDIT_FILE3, filename3.c_str());

    // Convert metadata strings to wchar_t
    if (wcslen(filename1.c_str()) > 0)
    {
        SetDlgItemText(IDC_LABEL_INFO1, (util::str2wstr(cfg_file1_metadata.get_ptr())).c_str());
    }

    if (wcslen(filename2.c_str()) > 0)
    {
        SetDlgItemText(IDC_LABEL_INFO2, (util::str2wstr(cfg_file2_metadata.get_ptr())).c_str());
    }

    if (wcslen(filename3.c_str()) > 0)
    {
        SetDlgItemText(IDC_LABEL_INFO3, (util::str2wstr(cfg_file3_metadata.get_ptr())).c_str());
    }

    RefreshFileLevelLabel();
    
    return FALSE;
}

void prefs_file::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    RefreshFileLevelLabel();
    OnChanged();
}

void prefs_file::OnButtonClick(UINT, int id, CWindow)
{
    switch (id)
    {
    case IDC_BROWSE1:
        LoadFile(IDC_EDIT_FILE1, IDC_LABEL_INFO1, IDC_SLIDER_LEVEL1, IDC_CHECK_FILE1);
        break;
    case IDC_BROWSE2:
        LoadFile(IDC_EDIT_FILE2, IDC_LABEL_INFO2, IDC_SLIDER_LEVEL2, IDC_CHECK_FILE2);
        break;
    case IDC_BROWSE3:
        LoadFile(IDC_EDIT_FILE3, IDC_LABEL_INFO3, IDC_SLIDER_LEVEL3, IDC_CHECK_FILE3);
        break;
    default:
        break;
    }

    OnChanged();
}

void prefs_file::LoadFile(int filename_id, int metadata_id, 
                          int level_id, int enable_id)
{
    COMDLG_FILTERSPEC c_rgSaveTypes[] =
    { 
        { _T("Audio Files"), L"*.aiff;*.au;*.caf;*.flac;*.iff;*.mat;*.nist;*.ogg;*.paf;*.rf64;*.sf;*.snd;*.sph;*.svx;*.voc;*.w64;*.wav" },
        { _T("All Files"), L"*.*"}
    };

    // CoCreate the File Open Dialog object.
    IFileDialog *pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr))
    {
        // Set the file types to display only. 
        // Notice that this is a 1-based array.
        hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
        if (SUCCEEDED(hr))
        {
            // Show the dialog
            hr = pfd->Show(NULL);
            if (SUCCEEDED(hr))
            {
                // Obtain the result once the user clicks
                // the 'Open' button.
                // The result is an IShellItem object.
                IShellItem *psiResult;
                hr = pfd->GetResult(&psiResult);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath = NULL;
                    hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,
                                                   &pszFilePath);
                    if (SUCCEEDED(hr))
                    {
                        int n_channels;
                        int n_frames;
                        int sampling_rate;
                        double attenuation;

                        // Calculate the optimum attentuation to prevent clipping
                        if (preprocessor::calculate_attenuation(pszFilePath, 
                                                                FILTER_LEN, 
                                                                REALSIZE,
                                                                &attenuation,
                                                                &n_channels,
                                                                &n_frames,
                                                                &sampling_rate))
                        {
                            SetDlgItemText(filename_id, pszFilePath);

                            std::wstringstream info;

                            info << n_frames << L" samples, "
                                 << n_channels << L" channels, "
                                 << sampling_rate << L" Hz";

                            SetDlgItemText(metadata_id, info.str().c_str());

                            CTrackBarCtrl slider = GetDlgItem(IDC_SLIDER_LEVEL1);
                            slider.SetPos((int)(attenuation * FILE_LEVEL_STEPS_PER_DB));

                            CheckDlgButton(enable_id, 1);

                            RefreshFileLevelLabel();
                        }

                        CoTaskMemFree(pszFilePath);
                    }

                    psiResult->Release();
                }
            }
        }
    }

    pfd->Release();
}

void prefs_file::RefreshFileLevelLabel()
{
    pfc::string_formatter msg;
    float level[3];

    level[0] = (float)m_slider_level1.GetPos() / FILE_LEVEL_STEPS_PER_DB;
    level[1] = (float)m_slider_level2.GetPos() / FILE_LEVEL_STEPS_PER_DB;
    level[2] = (float)m_slider_level3.GetPos() / FILE_LEVEL_STEPS_PER_DB;

    msg.reset();
    msg << pfc::format_float(level[0], 0, 1) << "dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL1, msg);

    msg.reset();
    msg << pfc::format_float(level[1], 0, 1) << "dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL2, msg);

    msg.reset();
    msg << pfc::format_float(level[2], 0, 1) << "dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL3, msg);
}

t_uint32 prefs_file::get_state()
{
    t_uint32 state = preferences_state::resettable;
    if (HasChanged()) state |= preferences_state::changed;
    return state;
}

void prefs_file::reset()
{
    CheckDlgButton(IDC_CHECK_FILE1, default_cfg_file1_enable);
    CheckDlgButton(IDC_CHECK_FILE2, default_cfg_file2_enable);
    CheckDlgButton(IDC_CHECK_FILE3, default_cfg_file3_enable);

    m_slider_level1.SetPos(default_cfg_file1_level);
    m_slider_level2.SetPos(default_cfg_file2_level);
    m_slider_level3.SetPos(default_cfg_file3_level);

    SetDlgItemText(IDC_EDIT_FILE1, L"");
    SetDlgItemText(IDC_EDIT_FILE2, L"");
    SetDlgItemText(IDC_EDIT_FILE3, L"");

    SetDlgItemText(IDC_LABEL_INFO1, L"");
    SetDlgItemText(IDC_LABEL_INFO2, L"");
    SetDlgItemText(IDC_LABEL_INFO3, L"");

    RefreshFileLevelLabel();

    OnChanged();
}

void prefs_file::apply()
{
    cfg_file1_enable = IsDlgButtonChecked(IDC_CHECK_FILE1);
    cfg_file2_enable = IsDlgButtonChecked(IDC_CHECK_FILE2);
    cfg_file3_enable = IsDlgButtonChecked(IDC_CHECK_FILE3);

    cfg_file1_level = m_slider_level1.GetPos();
    cfg_file2_level = m_slider_level2.GetPos();
    cfg_file3_level = m_slider_level3.GetPos();

    wchar_t wstr[1024];

    // convert filenames to char
    GetDlgItemText(IDC_EDIT_FILE1, (LPTSTR)wstr, sizeof(wstr));
    cfg_file1_filename.set_string((util::wstr2str(wstr)).c_str());

    GetDlgItemText(IDC_EDIT_FILE2, (LPTSTR)wstr, sizeof(wstr));
    cfg_file2_filename.set_string((util::wstr2str(wstr)).c_str());

    GetDlgItemText(IDC_EDIT_FILE3, (LPTSTR)wstr, sizeof(wstr));
    cfg_file3_filename.set_string((util::wstr2str(wstr)).c_str());

    // convert metadata string to char
    GetDlgItemText(IDC_LABEL_INFO1, (LPTSTR)wstr, sizeof(wstr));
    cfg_file1_metadata.set_string((util::wstr2str(wstr)).c_str());

    GetDlgItemText(IDC_LABEL_INFO2, (LPTSTR)wstr, sizeof(wstr));
    cfg_file2_metadata.set_string((util::wstr2str(wstr)).c_str());

    GetDlgItemText(IDC_LABEL_INFO3, (LPTSTR)wstr, sizeof(wstr));
    cfg_file3_metadata.set_string((util::wstr2str(wstr)).c_str());
    
    //g_apply_preferences();

    OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool prefs_file::HasChanged()
{
    wchar_t wstr[1024];
    bool has_changed;

    GetDlgItemText(IDC_EDIT_FILE1, (LPTSTR)wstr, sizeof(wstr));
    has_changed = wcscmp(wstr, (util::str2wstr(cfg_file1_filename.get_ptr())).c_str()) != 0;

    GetDlgItemText(IDC_EDIT_FILE2, (LPTSTR)wstr, sizeof(wstr));
    has_changed |= wcscmp(wstr, (util::str2wstr(cfg_file2_filename.get_ptr())).c_str()) != 0;

    GetDlgItemText(IDC_EDIT_FILE3, (LPTSTR)wstr, sizeof(wstr));
    has_changed |= wcscmp(wstr, (util::str2wstr(cfg_file3_filename.get_ptr())).c_str()) != 0;

    return
        has_changed ||
        (IsDlgButtonChecked(IDC_CHECK_FILE1) != cfg_file1_enable) ||
        (IsDlgButtonChecked(IDC_CHECK_FILE2) != cfg_file2_enable) ||
        (IsDlgButtonChecked(IDC_CHECK_FILE3) != cfg_file3_enable) ||
        (m_slider_level1.GetPos() != cfg_file1_level) ||
        (m_slider_level2.GetPos() != cfg_file2_level) ||
        (m_slider_level3.GetPos() != cfg_file3_level);
}

void prefs_file::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

double prefs_file::get_file1_scale()
{
    return FROM_DB((double)(cfg_file1_level) / FILE_LEVEL_STEPS_PER_DB);
}

double prefs_file::get_file2_scale()
{
    return FROM_DB((double)(cfg_file2_level) / FILE_LEVEL_STEPS_PER_DB);
}

double prefs_file::get_file3_scale()
{
    return FROM_DB((double)(cfg_file3_level) / FILE_LEVEL_STEPS_PER_DB);
}

