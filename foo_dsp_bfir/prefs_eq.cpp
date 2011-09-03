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

// Vertical sliders produce negative values when the position
// is above center and positive values when the position is
// below center.  The configuration value for level and magnitudes
// is the negative of the slider position.

cfg_int cfg_eq_enable(guid_cfg_eq_enable, default_cfg_eq_enable);
cfg_int cfg_eq_level(guid_cfg_eq_level, default_cfg_eq_level);
cfg_string cfg_eq_mag(guid_cfg_eq_mag, default_cfg_eq_mag);

std::string eq_freq_label[] =
{
    "20 Hz",
    "25 Hz",
    "31.5 Hz",
    "40 Hz",
    "50 Hz",
    "63 Hz",
    "80 Hz",
    "100 Hz",
    "125 Hz",
    "160 Hz",
    "200 Hz",
    "250 Hz",
    "315 Hz",
    "400 Hz",
    "500 Hz",
    "630 Hz",
    "800 Hz",
    "1 kHz",
    "1.25 kHz",
    "1.6 kHz",
    "2 kHz",
    "2.5 kHz",
    "3.15 kHz",
    "4 kHz",
    "5 kHz",
    "6.3 kHz",
    "8 kHz",
    "10 kHz",
    "12.5 kHz",
    "16 kHz",
    "20 kHz"
};

BOOL prefs_eq::OnInitDialog(CWindow, LPARAM)
{
    m_slider_eq_level = GetDlgItem(IDC_SLIDER_EQ_LEVEL);
    m_slider_eq_level.SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[0] = GetDlgItem(IDC_SLIDER_EQ1);
    m_slider_eq_mag[0].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[1] = GetDlgItem(IDC_SLIDER_EQ2);
    m_slider_eq_mag[1].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[2] = GetDlgItem(IDC_SLIDER_EQ3);
    m_slider_eq_mag[2].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[3] = GetDlgItem(IDC_SLIDER_EQ4);
    m_slider_eq_mag[3].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[4] = GetDlgItem(IDC_SLIDER_EQ5);
    m_slider_eq_mag[4].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[5] = GetDlgItem(IDC_SLIDER_EQ6);
    m_slider_eq_mag[5].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[6] = GetDlgItem(IDC_SLIDER_EQ7);
    m_slider_eq_mag[6].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[7] = GetDlgItem(IDC_SLIDER_EQ8);
    m_slider_eq_mag[7].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[8] = GetDlgItem(IDC_SLIDER_EQ9);
    m_slider_eq_mag[8].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[9] = GetDlgItem(IDC_SLIDER_EQ10);
    m_slider_eq_mag[9].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[10] = GetDlgItem(IDC_SLIDER_EQ11);
    m_slider_eq_mag[10].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[11] = GetDlgItem(IDC_SLIDER_EQ12);
    m_slider_eq_mag[11].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[12] = GetDlgItem(IDC_SLIDER_EQ13);
    m_slider_eq_mag[12].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[13] = GetDlgItem(IDC_SLIDER_EQ14);
    m_slider_eq_mag[13].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[14] = GetDlgItem(IDC_SLIDER_EQ15);
    m_slider_eq_mag[14].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[15] = GetDlgItem(IDC_SLIDER_EQ16);
    m_slider_eq_mag[15].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[16] = GetDlgItem(IDC_SLIDER_EQ17);
    m_slider_eq_mag[16].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[17] = GetDlgItem(IDC_SLIDER_EQ18);
    m_slider_eq_mag[17].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[18] = GetDlgItem(IDC_SLIDER_EQ19);
    m_slider_eq_mag[18].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[19] = GetDlgItem(IDC_SLIDER_EQ20);
    m_slider_eq_mag[19].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[20] = GetDlgItem(IDC_SLIDER_EQ21);
    m_slider_eq_mag[20].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[21] = GetDlgItem(IDC_SLIDER_EQ22);
    m_slider_eq_mag[21].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[22] = GetDlgItem(IDC_SLIDER_EQ23);
    m_slider_eq_mag[22].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[23] = GetDlgItem(IDC_SLIDER_EQ24);
    m_slider_eq_mag[23].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[24] = GetDlgItem(IDC_SLIDER_EQ25);
    m_slider_eq_mag[24].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[25] = GetDlgItem(IDC_SLIDER_EQ26);
    m_slider_eq_mag[25].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[26] = GetDlgItem(IDC_SLIDER_EQ27);
    m_slider_eq_mag[26].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[27] = GetDlgItem(IDC_SLIDER_EQ28);
    m_slider_eq_mag[27].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[28] = GetDlgItem(IDC_SLIDER_EQ29);
    m_slider_eq_mag[28].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[29] = GetDlgItem(IDC_SLIDER_EQ30);
    m_slider_eq_mag[29].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    m_slider_eq_mag[30] = GetDlgItem(IDC_SLIDER_EQ31);
    m_slider_eq_mag[30].SetRange(EQLevelRangeMin, EQLevelRangeMax);

    LoadSettings();

    SetDlgItemText(IDC_LABEL_ADJUST, L"");

    return FALSE;
}

void prefs_eq::LoadSettings()
{
    CheckDlgButton(IDC_CHECK_EQ, cfg_eq_enable);

    m_slider_eq_level.SetPos(-cfg_eq_level);

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

        m_slider_eq_mag[ix].SetPos(-slider_level);
    }

    SetDlgItemText(IDC_LABEL_ADJUST, L"");
}

void prefs_eq::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    switch(pScrollBar.GetDlgCtrlID())
    {
    case IDC_SLIDER_EQ_LEVEL:
        ShowEqLevel();
        break;
    case IDC_SLIDER_EQ1:
        ShowEqMag(0);
        break;
    case IDC_SLIDER_EQ2:
        ShowEqMag(1);
        break;
    case IDC_SLIDER_EQ3:
        ShowEqMag(2);
        break;
    case IDC_SLIDER_EQ4:
        ShowEqMag(3);
        break;
    case IDC_SLIDER_EQ5:
        ShowEqMag(4);
        break;
    case IDC_SLIDER_EQ6:
        ShowEqMag(5);
        break;
    case IDC_SLIDER_EQ7:
        ShowEqMag(6);
        break;
    case IDC_SLIDER_EQ8:
        ShowEqMag(7);
        break;
    case IDC_SLIDER_EQ9:
        ShowEqMag(8);
        break;
    case IDC_SLIDER_EQ10:
        ShowEqMag(9);
        break;
    case IDC_SLIDER_EQ11:
        ShowEqMag(10);
        break;
    case IDC_SLIDER_EQ12:
        ShowEqMag(11);
        break;
    case IDC_SLIDER_EQ13:
        ShowEqMag(12);
        break;
    case IDC_SLIDER_EQ14:
        ShowEqMag(13);
        break;
    case IDC_SLIDER_EQ15:
        ShowEqMag(14);
        break;
    case IDC_SLIDER_EQ16:
        ShowEqMag(15);
        break;
    case IDC_SLIDER_EQ17:
        ShowEqMag(16);
        break;
    case IDC_SLIDER_EQ18:
        ShowEqMag(17);
        break;
    case IDC_SLIDER_EQ19:
        ShowEqMag(18);
        break;
    case IDC_SLIDER_EQ20:
        ShowEqMag(19);
        break;
    case IDC_SLIDER_EQ21:
        ShowEqMag(20);
        break;
    case IDC_SLIDER_EQ22:
        ShowEqMag(21);
        break;
    case IDC_SLIDER_EQ23:
        ShowEqMag(22);
        break;
    case IDC_SLIDER_EQ24:
        ShowEqMag(23);
        break;
    case IDC_SLIDER_EQ25:
        ShowEqMag(24);
        break;
    case IDC_SLIDER_EQ26:
        ShowEqMag(25);
        break;
    case IDC_SLIDER_EQ27:
        ShowEqMag(26);
        break;
    case IDC_SLIDER_EQ28:
        ShowEqMag(27);
        break;
    case IDC_SLIDER_EQ29:
        ShowEqMag(28);
        break;
    case IDC_SLIDER_EQ30:
        ShowEqMag(29);
        break;
    case IDC_SLIDER_EQ31:
        ShowEqMag(30);
        break;
    }

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

void prefs_eq::ShowEqLevel()
{
    pfc::string_formatter msg;

    float level = (float)-m_slider_eq_level.GetPos() / EQ_LEVEL_STEPS_PER_DB;

    msg.reset();
    msg << "Level: " << pfc::format_float(level, 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_LABEL_ADJUST, msg);
}

void prefs_eq::ShowEqMag(int index)
{
    pfc::string_formatter msg;

    float level = (float)-m_slider_eq_mag[index].GetPos() / EQ_LEVEL_STEPS_PER_DB;

    msg.reset();
    msg << eq_freq_label[index].c_str() << ": " << pfc::format_float(level, 0, 1) << " dB";
    ::uSetDlgItemText(*this, IDC_LABEL_ADJUST, msg);
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

    m_slider_eq_level.SetPos(-default_cfg_eq_level);

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

        m_slider_eq_mag[ix].SetPos(-slider_level);
    }

    SetDlgItemText(IDC_LABEL_ADJUST, L"");
    OnChanged();
}

void prefs_eq::apply()
{
    cfg_eq_enable = IsDlgButtonChecked(IDC_CHECK_EQ);

    cfg_eq_level = -m_slider_eq_level.GetPos();

    std::stringstream out;
    for (int ix = 0; ix < BAND_COUNT; ix++)
    {
        out << -m_slider_eq_mag[ix].GetPos();

        if (ix != BAND_COUNT - 1)
        {
            out << ",";
        }
    }

    cfg_eq_mag.set_string(out.str().c_str());

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

        if (-m_slider_eq_mag[ix].GetPos() != mag)
        {
            has_changed = true;
            break;
        }
    }

    return
        has_changed ||
        (IsDlgButtonChecked(IDC_CHECK_EQ) != cfg_eq_enable) ||
        (-m_slider_eq_level.GetPos() != cfg_eq_level);
}

void prefs_eq::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

double prefs_eq::get_scale()
{
    return FROM_DB((double)(cfg_eq_level) / EQ_LEVEL_STEPS_PER_DB);
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

        mag[ix] = (double)(slider_level) / EQ_LEVEL_STEPS_PER_DB;
    }
}