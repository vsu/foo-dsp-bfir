/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <foobar2000.h>
#include <fstream>
#include <malloc.h>
#include "config.hpp"
#include "../brutefir/preprocessor.hpp"
#include "../brutefir/util.hpp"
#include "../json_spirit/json_spirit.h"

using namespace json_spirit;

CMyDSPPopup::CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback)
    : m_initData(initData), m_callback(callback) {}

BOOL CMyDSPPopup::OnInitDialog(CWindow, LPARAM)
{
    m_slider_eq_level = GetDlgItem(IDC_SLIDER_EQ_LEVEL);
    m_slider_eq_level.SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq[0] = GetDlgItem(IDC_SLIDER_EQ1);
    m_slider_eq[0].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[1] = GetDlgItem(IDC_SLIDER_EQ2);
    m_slider_eq[1].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[2] = GetDlgItem(IDC_SLIDER_EQ3);
    m_slider_eq[2].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[3] = GetDlgItem(IDC_SLIDER_EQ4);
    m_slider_eq[3].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[4] = GetDlgItem(IDC_SLIDER_EQ5);
    m_slider_eq[4].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[5] = GetDlgItem(IDC_SLIDER_EQ6);
    m_slider_eq[5].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[6] = GetDlgItem(IDC_SLIDER_EQ7);
    m_slider_eq[6].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[7] = GetDlgItem(IDC_SLIDER_EQ8);
    m_slider_eq[7].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[8] = GetDlgItem(IDC_SLIDER_EQ9);
    m_slider_eq[8].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[9] = GetDlgItem(IDC_SLIDER_EQ10);
    m_slider_eq[9].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[10] = GetDlgItem(IDC_SLIDER_EQ11);
    m_slider_eq[10].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[11] = GetDlgItem(IDC_SLIDER_EQ12);
    m_slider_eq[11].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[12] = GetDlgItem(IDC_SLIDER_EQ13);
    m_slider_eq[12].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[13] = GetDlgItem(IDC_SLIDER_EQ14);
    m_slider_eq[13].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[14] = GetDlgItem(IDC_SLIDER_EQ15);
    m_slider_eq[14].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[15] = GetDlgItem(IDC_SLIDER_EQ16);
    m_slider_eq[15].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[16] = GetDlgItem(IDC_SLIDER_EQ17);
    m_slider_eq[16].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[17] = GetDlgItem(IDC_SLIDER_EQ18);
    m_slider_eq[17].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[18] = GetDlgItem(IDC_SLIDER_EQ19);
    m_slider_eq[18].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[19] = GetDlgItem(IDC_SLIDER_EQ20);
    m_slider_eq[19].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[20] = GetDlgItem(IDC_SLIDER_EQ21);
    m_slider_eq[20].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[21] = GetDlgItem(IDC_SLIDER_EQ22);
    m_slider_eq[21].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[22] = GetDlgItem(IDC_SLIDER_EQ23);
    m_slider_eq[22].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[23] = GetDlgItem(IDC_SLIDER_EQ24);
    m_slider_eq[23].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[24] = GetDlgItem(IDC_SLIDER_EQ25);
    m_slider_eq[24].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[25] = GetDlgItem(IDC_SLIDER_EQ26);
    m_slider_eq[25].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[26] = GetDlgItem(IDC_SLIDER_EQ27);
    m_slider_eq[26].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[27] = GetDlgItem(IDC_SLIDER_EQ28);
    m_slider_eq[27].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[28] = GetDlgItem(IDC_SLIDER_EQ29);
    m_slider_eq[28].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[29] = GetDlgItem(IDC_SLIDER_EQ30);
    m_slider_eq[29].SetRange(EQRangeMin, EQRangeMax);

    m_slider_eq[30] = GetDlgItem(IDC_SLIDER_EQ31);
    m_slider_eq[30].SetRange(EQRangeMin, EQRangeMax);

    m_slider_level1 = GetDlgItem(IDC_SLIDER_LEVEL1);
    m_slider_level1.SetRange(FileLevelRangeMin, FileLevelRangeMax);

    m_slider_level2 = GetDlgItem(IDC_SLIDER_LEVEL2);
    m_slider_level2.SetRange(FileLevelRangeMin, FileLevelRangeMax);

    m_slider_level3 = GetDlgItem(IDC_SLIDER_LEVEL3);
    m_slider_level3.SetRange(FileLevelRangeMin, FileLevelRangeMax);

    m_checkbox_eq = GetDlgItem(IDC_CHECK_EQ);
    m_checkbox_enable1 = GetDlgItem(IDC_CHECK_FILE1);
    m_checkbox_enable2 = GetDlgItem(IDC_CHECK_FILE2);
    m_checkbox_enable3 = GetDlgItem(IDC_CHECK_FILE3);
    m_checkbox_overflow = GetDlgItem(IDC_CHECK_OVERFLOW);

    m_edit_file1 = GetDlgItem(IDC_EDIT_FILE1);
    m_edit_file2 = GetDlgItem(IDC_EDIT_FILE2);
    m_edit_file3 = GetDlgItem(IDC_EDIT_FILE3);

    dsp_bfir::get_preset(&m_params, m_initData);
    LoadParameters(m_params);

    return TRUE;
}

void CMyDSPPopup::OnButtonOkCancel(UINT, int id, CWindow)
{
    SaveParameters(&m_params);
    UpdatePreset(m_params);
    EndDialog(id);
}

void CMyDSPPopup::OnButtonReset(UINT, int id, CWindow)
{    
    memset(&m_params, 0, sizeof(dsp_bfir_param_t));
    LoadParameters(m_params);
}

void CMyDSPPopup::OnButtonLoad(UINT, int id, CWindow)
{
    FileBrowseLoad();
}

void CMyDSPPopup::OnButtonSave(UINT, int id, CWindow)
{
    FileBrowseSave();
}

void CMyDSPPopup::OnButtonBrowse1(UINT, int id, CWindow)
{
    FileBrowseImpulse(0);
}

void CMyDSPPopup::OnButtonBrowse2(UINT, int id, CWindow)
{
    FileBrowseImpulse(1);
}

void CMyDSPPopup::OnButtonBrowse3(UINT, int id, CWindow)
{
    FileBrowseImpulse(2);
}

void CMyDSPPopup::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    RefreshEqLevelLabel();
}

void CMyDSPPopup::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    RefreshFileLevelLabel();
}

void CMyDSPPopup::SaveParameters(struct dsp_bfir_param_t *params)
{
    // The position value and the displayed scale are inverted
    params->eq_slider_level = -m_slider_eq_level.GetPos();

    for (int i = 0; i < BAND_COUNT; i++)
    {
        params->mag[i] = -(double)m_slider_eq[i].GetPos();
    }

    params->file[0].slider_level = m_slider_level1.GetPos();
    params->file[1].slider_level = m_slider_level2.GetPos();
    params->file[2].slider_level = m_slider_level3.GetPos();

    params->eq_enabled = m_checkbox_eq.GetCheck();
    params->file[0].enabled = m_checkbox_enable1.GetCheck();
    params->file[1].enabled = m_checkbox_enable2.GetCheck();
    params->file[2].enabled = m_checkbox_enable3.GetCheck();
    params->overflow_warnings = m_checkbox_overflow.GetCheck();
}

void CMyDSPPopup::LoadParameters(struct dsp_bfir_param_t params)
{
    // The position value and the displayed scale are inverted
    m_slider_eq_level.SetPos(-params.eq_slider_level);

    for (int i = 0; i < BAND_COUNT; i++)
    {
        m_slider_eq[i].SetPos(-(int)params.mag[i]);
    }

    m_slider_level1.SetPos(params.file[0].slider_level);
    m_slider_level2.SetPos(params.file[1].slider_level);
    m_slider_level3.SetPos(params.file[2].slider_level);

    m_checkbox_eq.SetCheck(params.eq_enabled);
    m_checkbox_enable1.SetCheck(params.file[0].enabled);
    m_checkbox_enable2.SetCheck(params.file[1].enabled);
    m_checkbox_enable3.SetCheck(params.file[2].enabled);
    m_checkbox_overflow.SetCheck(params.overflow_warnings);

    SetDlgItemText(IDC_EDIT_FILE1, params.file[0].filename);
    SetDlgItemText(IDC_EDIT_FILE2, params.file[1].filename);
    SetDlgItemText(IDC_EDIT_FILE3, params.file[2].filename);
    
    std::wstringstream info;
    
    info.str(L"");
    if (wcslen(params.file[0].filename) != 0)
    {
        info << params.file[0].n_frames << L" samples, "
             << params.file[0].n_channels << L" channels, "
             << params.file[0].sampling_rate << L" Hz";
    }

     SetDlgItemText(IDC_LABEL_INFO1, info.str().c_str());

    info.str(L"");
    if (wcslen(params.file[1].filename) != 0)
    {
        info << params.file[1].n_frames << L" samples, "
             << params.file[1].n_channels << L" channels, "
             << params.file[1].sampling_rate << L" Hz";
    }

    SetDlgItemText(IDC_LABEL_INFO2, info.str().c_str());

    info.str(L"");
    if (wcslen(params.file[2].filename) != 0)
    {
        info << params.file[2].n_frames << L" samples, "
             << params.file[2].n_channels << L" channels, "
             << params.file[2].sampling_rate << L" Hz";
    }

    SetDlgItemText(IDC_LABEL_INFO3, info.str().c_str());

    RefreshEqLevelLabel();
    RefreshFileLevelLabel();
}

void CMyDSPPopup::UpdatePreset(struct dsp_bfir_param_t params)
{
    dsp_preset_impl preset;
    dsp_bfir::make_preset(params, preset);

    m_callback.on_preset_changed(preset);
}

void CMyDSPPopup::RefreshEqLevelLabel()
{
    pfc::string_formatter msg;
    float level = (float)-m_slider_eq_level.GetPos() / LEVEL_SLIDER_STEPS;

    msg.reset();
    msg << pfc::format_float(level, 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_EQ_LEVEL, msg);
}

void CMyDSPPopup::RefreshFileLevelLabel()
{
    pfc::string_formatter msg;
    float level[3];

    level[0] = (float)m_slider_level1.GetPos() / LEVEL_SLIDER_STEPS;
    level[1] = (float)m_slider_level2.GetPos() / LEVEL_SLIDER_STEPS;
    level[2] = (float)m_slider_level3.GetPos() / LEVEL_SLIDER_STEPS;

    msg.reset();
    msg << pfc::format_float(level[0], 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL1, msg);

    msg.reset();
    msg << pfc::format_float(level[1], 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL2, msg);

    msg.reset();
    msg << pfc::format_float(level[2], 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_LEVEL3, msg);
}

void CMyDSPPopup::FileBrowseSave()
{
    COMDLG_FILTERSPEC c_rgSaveTypes[] =
    { 
        { _T("JSON Files"), L"*.json" },
        { _T("All Files"), L"*.*"}
    };

    // CoCreate the File Save Dialog object.
    IFileDialog *pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
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
            // Set the default extension to be ".json" file.
            hr = pfd->SetDefaultExtension(L"json");
            if (SUCCEEDED(hr))
            {
                // Show the dialog
                hr = pfd->Show(NULL);
                if (SUCCEEDED(hr))
                {
                    // Obtain the result once the user clicks
                    // the 'Save' button.
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
                            WriteJson(pszFilePath, m_params); 
                            CoTaskMemFree(pszFilePath);
                        }

                        psiResult->Release();
                    }
                }
            }
        }
    }

    pfd->Release();
}

void CMyDSPPopup::FileBrowseLoad()
{
    COMDLG_FILTERSPEC c_rgSaveTypes[] =
    { 
        { _T("JSON Files"), L"*.json" },
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
            // Set the default extension to be ".json" file.
            hr = pfd->SetDefaultExtension(L"json");
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
                            struct dsp_bfir_param_t params;
                                
                            if (ReadJson(pszFilePath, &params))
                            {
                                memcpy(&m_params, &params, sizeof(struct dsp_bfir_param_t));
                                LoadParameters(m_params);
                            }
                            else
                            {
                                TaskDialog(NULL,
                                           NULL,
                                           L"Load Configuration",
                                           pszFilePath,
                                           L"Error loading configuration file.",
                                           TDCBF_OK_BUTTON,
                                           TD_ERROR_ICON,
                                           NULL);
                            }

                            CoTaskMemFree(pszFilePath);
                        }

                        psiResult->Release();
                    }
                }
            }
        }
    }

    pfd->Release();
}

void CMyDSPPopup::FileBrowseImpulse(int index)
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
                            m_params.file[index].enabled = 1;
                            m_params.file[index].slider_level = (int)(attenuation * 10);
                            m_params.file[index].n_channels = n_channels;
                            m_params.file[index].n_frames = n_frames;
                            m_params.file[index].sampling_rate = sampling_rate;
                            wcscpy(m_params.file[index].filename, pszFilePath);

                            LoadParameters(m_params);
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

void CMyDSPPopup::WriteJson(PWSTR filename, struct dsp_bfir_param_t params)
{
    int i;
    std::stringstream mag;

    // prepare equalizer magnitudes string
    for (i = 0; i < BAND_COUNT; i++)
    {
        mag << params.mag[i];
        
        if (i != (BAND_COUNT - 1))
        {
            mag << ',';
        }
    }

    Array file_array;

    for (i = 0; i < 3; i++)
    {
        // convert filename to char
        int size = wcstombs(NULL, params.file[i].filename, 0);
        char *filename_mbs = (char *) _alloca((size + 1) * sizeof(char));
        wcstombs(filename_mbs, params.file[i].filename, size + 1); 

        // build file object
        Object file_obj;
        file_obj.push_back(Pair("enabled", params.file[i].enabled));
        file_obj.push_back(Pair("slider_level", params.file[i].slider_level));
        file_obj.push_back(Pair("n_channels", params.file[i].n_channels));
        file_obj.push_back(Pair("n_frames", params.file[i].n_frames));
        file_obj.push_back(Pair("sampling_rate", params.file[i].sampling_rate));
        file_obj.push_back(Pair("filename", filename_mbs));

        // add to file array
        file_array.push_back(file_obj);
    }

    // build parameters object
    Object params_obj;
    params_obj.push_back(Pair("mag", mag.str()));
    params_obj.push_back(Pair("eq_enabled", params.eq_enabled));
    params_obj.push_back(Pair("eq_slider_level", params.eq_slider_level));  
    params_obj.push_back(Pair("overflow_warnings", params.overflow_warnings));
    params_obj.push_back(Pair("file", file_array));

    // add to parameters array
    Array params_array;
    params_array.push_back(params_obj);

    // write to file
    std::ofstream os(filename);
    write_formatted(params_array, os);
    os.close();
}

BOOL CMyDSPPopup::ReadJson(PWSTR filename, struct dsp_bfir_param_t *params)
{
    BOOL status = FALSE;

    memset(params, 0, sizeof(struct dsp_bfir_param_t));

    std::ifstream is(filename);

    Value value;
    read(is, value);

    const Array& params_array = value.get_array();

    if (params_array.size() == 1)
    {
        const Object& params_obj = params_array[0].get_obj();

        for (Object::size_type i = 0; i != params_obj.size(); ++i)
        {
            const Pair& params_pair = params_obj[i];
                
            const std::string& params_name  = params_pair.name_;
            const Value& params_value = params_pair.value_;

            if (params_name == "mag")
            {
                std::vector<std::string> split_str = util::split(params_value.get_str(), ',');
                std::vector<std::string>::iterator it;
                    
                int i = 0;
                double val;
                for (it = split_str.begin(); it < split_str.end(); it++)
                {
                    std::istringstream(*it) >> val;
                    params->mag[i++] = val;
                }
            }
            else if (params_name == "eq_enabled")
            {
                params->eq_enabled = params_value.get_int();
            }
            else if (params_name == "eq_slider_level")
            {
                params->eq_slider_level = params_value.get_int();
            }
            else if (params_name == "overflow_warnings")
            {
                params->overflow_warnings = params_value.get_int();
            }
            else if (params_name == "file")
            {
                const Array& file_array = params_value.get_array();

                if (file_array.size() == 3)
                {
                    for (Array::size_type j = 0; j != file_array.size(); ++j)
                    {
                        const Object& file_obj = file_array[j].get_obj();

                        for (Object::size_type k = 0; k != file_obj.size(); ++k)
                        {
                            const Pair& file_pair = file_obj[k];
                
                            const std::string& file_name  = file_pair.name_;
                            const Value& file_value = file_pair.value_;

                            if (file_name == "enabled")
                            {
                                params->file[j].enabled = file_value.get_int();
                            }
                            else if (file_name == "slider_level")
                            {
                                params->file[j].slider_level = file_value.get_int();
                            }
                            else if (file_name == "n_channels")
                            {
                                params->file[j].n_channels = file_value.get_int();
                            }
                            else if (file_name == "n_frames")
                            {
                                params->file[j].n_frames = file_value.get_int();
                            }
                            else if (file_name == "sampling_rate")
                            {
                                params->file[j].sampling_rate = file_value.get_int();
                            }
                            else if (file_name == "filename")
                            {
                                // convert filename to wchar_t
                                int size = mbstowcs(NULL, file_value.get_str().c_str(), 0);
                                wchar_t * filename_wcs = (wchar_t *) _alloca((size + 1) * sizeof(wchar_t));
                                mbstowcs(filename_wcs, file_value.get_str().c_str(), size + 1);
                                
                                wcscpy(params->file[j].filename, filename_wcs);
                            }
                        }
                    }

                    status = TRUE;
                }
            }
        }
    }

    is.close();

    return status;
}

