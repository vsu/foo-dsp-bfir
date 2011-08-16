/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include "prefs_eq.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "../brutefir/util.hpp"
#include "../brutefir/equalizer.hpp"
#include "../json_spirit/json_spirit.h"

cfg_int cfg_eq_enable(guid_cfg_eq_enable, default_cfg_eq_enable);
cfg_int cfg_eq_level(guid_cfg_eq_level, default_cfg_eq_level);
cfg_string cfg_eq_mag(guid_cfg_eq_mag, default_cfg_eq_mag);

BOOL prefs_eq::OnInitDialog(CWindow, LPARAM)
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

    LoadSettings();

    return FALSE;
}

void prefs_eq::LoadSettings()
{    
    CheckDlgButton(IDC_CHECK_EQ, cfg_eq_enable);
    
    m_slider_eq_level.SetPos(cfg_eq_level);

    std::vector<std::string> mags = util::split(cfg_eq_mag.get_ptr(), ',');

    for (unsigned int ix = 0; ix < mags.size(); ix++)
    {
        int slider_level;
        try
        {
            slider_level = boost::lexical_cast<int>(mags[ix]);
        }
        catch (const boost::bad_lexical_cast &)
        {
            slider_level = 0;
        }

        m_slider_eq[ix].SetPos(slider_level);
    }

    RefreshEqLevelLabel();
}

void prefs_eq::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    RefreshEqLevelLabel();
    OnChanged();
}

void prefs_eq::OnButtonClick(UINT, int id, CWindow)
{
    switch (id)
    {
    case IDC_LOAD:
        LoadFile();
        break;
    case IDC_SAVE:
        SaveFile();
        break;
    default:
        break;
    }

    OnChanged();
}

void prefs_eq::RefreshEqLevelLabel()
{
    pfc::string_formatter msg;
    
    // The position value and the displayed scale are inverted
    // for vertical scrollbars
    float level = (float)-m_slider_eq_level.GetPos() / EQ_LEVEL_STEPS_PER_DB;

    msg.reset();
    msg << pfc::format_float(level, 0, 1) << "dB";
    ::uSetDlgItemText(*this, IDC_SLIDER_LABEL_EQ_LEVEL, msg);
}

void prefs_eq::LoadFile()
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
                            if (ReadJson(pszFilePath))
                            {
                                LoadSettings();
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

void prefs_eq::SaveFile()
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
                            WriteJson(pszFilePath); 
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

void prefs_eq::WriteJson(PWSTR filename)
{
    // build parameters object
    json_spirit::Object params_obj;
    params_obj.push_back(json_spirit::Pair("cfg_eq_level", cfg_eq_level.get_value()));
    params_obj.push_back(json_spirit::Pair("cfg_eq_mag", cfg_eq_mag.get_ptr()));

    // add to parameters array
    json_spirit::Array params_array;
    params_array.push_back(params_obj);

    // write to file
    std::ofstream os(filename);
    json_spirit::write_formatted(params_array, os);
    os.close();
}

BOOL prefs_eq::ReadJson(PWSTR filename)
{
    BOOL status = FALSE;

    std::ifstream is(filename);

    json_spirit::Value value;
    read(is, value);

    const json_spirit::Array& params_array = value.get_array();

    if (params_array.size() == 1)
    {
        const json_spirit::Object& params_obj = params_array[0].get_obj();

        for (json_spirit::Object::size_type i = 0; i != params_obj.size(); ++i)
        {
            const json_spirit::Pair& params_pair = params_obj[i];
                
            const std::string& params_name  = params_pair.name_;
            const json_spirit::Value& params_value = params_pair.value_;

            if (params_name == "cfg_eq_level")
            {
                cfg_eq_level = params_value.get_int();
            }
            else if (params_name == "cfg_eq_mag")
            {
                cfg_eq_mag.set_string((params_value.get_str()).c_str());
            }
        }

        status = TRUE;
    }

    is.close();

    return status;
}

t_uint32 prefs_eq::get_state()
{
    t_uint32 state = preferences_state::resettable;
    if (HasChanged()) state |= preferences_state::changed;
    return state;
}

void prefs_eq::reset()
{
    CheckDlgButton(IDC_CHECK_EQ, default_cfg_eq_enable);

    m_slider_eq_level.SetPos(default_cfg_eq_level);

    std::vector<std::string> mags = util::split(default_cfg_eq_mag, ',');

    for (unsigned int ix = 0; ix < mags.size(); ix++)
    {
        int slider_level;
        try
        {
            slider_level = boost::lexical_cast<int>(mags[ix]);
        }
        catch (const boost::bad_lexical_cast &)
        {
            slider_level = 0;
        }

        m_slider_eq[ix].SetPos(slider_level);
    }

    RefreshEqLevelLabel();

    OnChanged();
}

void prefs_eq::apply()
{
    cfg_eq_enable = IsDlgButtonChecked(IDC_CHECK_EQ);

    cfg_eq_level = m_slider_eq_level.GetPos();

    std::stringstream out;
    for (int ix = 0; ix < BAND_COUNT; ix++)
    {
        out << m_slider_eq[ix].GetPos();
        
        if (ix != BAND_COUNT - 1)
        {
            out << ",";
        }
    }

    cfg_eq_mag.set_string(out.str().c_str());

    //g_apply_preferences();

    OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool prefs_eq::HasChanged()
{
    bool has_changed = false;

    std::vector<std::string> mags = util::split(cfg_eq_mag.get_ptr(), ',');

    for (unsigned int ix = 0; ix < mags.size(); ix++)
    {
        int mag;
        try
        {
            mag = boost::lexical_cast<int>(mags[ix]);
        }
        catch (const boost::bad_lexical_cast &)
        {
            mag = 0;
        }

        if (m_slider_eq[ix].GetPos() != mag)
        {
            has_changed = true;
            break;
        }
    }

    return
        has_changed ||
        (IsDlgButtonChecked(IDC_CHECK_EQ) != cfg_eq_enable) ||
        (m_slider_eq_level.GetPos() != cfg_eq_level);
}

void prefs_eq::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

double prefs_eq::get_scale()
{
    // The slider level and display scale are inverted
    // for vertical scrollbars
    return FROM_DB((double)(-cfg_eq_level) / EQ_LEVEL_STEPS_PER_DB);
}
 
void prefs_eq::get_mag(double *mag)
{
    std::vector<std::string> mags = util::split(cfg_eq_mag.get_ptr(), ',');

    for (unsigned int ix = 0; ix < mags.size(); ix++)
    {
        int slider_level;
        try
        {
            slider_level = boost::lexical_cast<int>(mags[ix]);
        }
        catch (const boost::bad_lexical_cast &)
        {
            slider_level = 0;
        }

        // The slider level and display scale are inverted 
        // for vertical scrollbars
        mag[ix] = (double)(-slider_level) / EQ_MAG_STEPS_PER_DB;
    }
}