/*
 *  scope.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "phd.h"

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"


static const int DefaultCalibrationDuration = 750;
static const int DefaultMaxDecDuration  = 1000;
static const int DefaultMaxRaDuration  = 1000;

static const DEC_GUIDE_MODE DefaultDecGuideMode = DEC_AUTO;
static const GUIDE_ALGORITHM DefaultRaGuideAlgorithm = GUIDE_ALGORITHM_HYSTERESIS;
static const GUIDE_ALGORITHM DefaultDecGuideAlgorithm = GUIDE_ALGORITHM_RESIST_SWITCH;

static const double DEC_BACKLASH_DISTANCE = 3.0;
static const int MAX_CALIBRATION_STEPS = 60;
static const double MAX_CALIBRATION_DISTANCE = 25.0;

Scope::Scope(void)
{
    m_calibrationSteps = 0;

    wxString prefix = "/" + GetMountClassName();

    int calibrationDuration = pConfig->GetInt(prefix + "/CalibrationDuration", DefaultCalibrationDuration);
    SetCalibrationDuration(calibrationDuration);

    int maxRaDuration  = pConfig->GetInt(prefix + "/MaxRaDuration", DefaultMaxRaDuration);
    SetMaxRaDuration(maxRaDuration);

    int maxDecDuration = pConfig->GetInt(prefix + "/MaxDecDuration", DefaultMaxDecDuration);
    SetMaxDecDuration(maxDecDuration);

    int decGuideMode = pConfig->GetInt(prefix + "/DecGuideMode", DefaultDecGuideMode);
    SetDecGuideMode(decGuideMode);

    int raGuideAlgorithm = pConfig->GetInt(prefix + "/XGuideAlgorithm", DefaultRaGuideAlgorithm);
    SetXGuideAlgorithm(raGuideAlgorithm);

    int decGuideAlgorithm = pConfig->GetInt(prefix + "/YGuideAlgorithm", DefaultDecGuideAlgorithm);
    SetYGuideAlgorithm(decGuideAlgorithm);
}

Scope::~Scope(void)
{
}

int Scope::GetCalibrationDuration(void)
{
    return m_calibrationDuration;
}

bool Scope::SetCalibrationDuration(int calibrationDuration)
{
    bool bError = false;

    try
    {
        if (calibrationDuration <= 0.0)
        {
            throw ERROR_INFO("invalid calibrationDuration");
        }

        m_calibrationDuration = calibrationDuration;

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_calibrationDuration = DefaultCalibrationDuration;
    }

    pConfig->SetInt("/scope/CalibrationDuration", m_calibrationDuration);

    return bError;
}

int Scope::GetMaxDecDuration(void)
{
    return m_maxDecDuration;
}

bool Scope::SetMaxDecDuration(int maxDecDuration)
{
    bool bError = false;

    try
    {
        if (maxDecDuration < 0)
        {
            throw ERROR_INFO("maxDecDuration < 0");
        }

        m_maxDecDuration = maxDecDuration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_maxDecDuration = DefaultMaxDecDuration;
    }

    pConfig->SetInt("/scope/MaxDecDuration", m_maxDecDuration);

    return bError;
}

int Scope::GetMaxRaDuration(void)
{
    return m_maxRaDuration;
}

bool Scope::SetMaxRaDuration(double maxRaDuration)
{
    bool bError = false;

    try
    {
        if (maxRaDuration < 0)
        {
            throw ERROR_INFO("maxRaDuration < 0");
        }

        m_maxRaDuration =  maxRaDuration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_maxRaDuration = DefaultMaxRaDuration;
    }

    pConfig->SetInt("/scope/MaxRaDuration", m_maxRaDuration);

    return bError;
}

DEC_GUIDE_MODE Scope::GetDecGuideMode(void)
{
    return m_decGuideMode;
}

bool Scope::SetDecGuideMode(int decGuideMode)
{
    bool bError = false;

    try
    {
        switch (decGuideMode)
        {
            case DEC_NONE:
            case DEC_AUTO:
            case DEC_NORTH:
            case DEC_SOUTH:
                break;
            default:
                throw ERROR_INFO("invalid decGuideMode");
                break;
        }

        m_decGuideMode = (DEC_GUIDE_MODE)decGuideMode;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_decGuideMode = (DEC_GUIDE_MODE)DefaultDecGuideMode;
    }

    pConfig->SetInt("/scope/DecGuideMode", m_decGuideMode);
    if (pFrame)
        pFrame->UpdateCalibrationStatus();

    return bError;
}

wxArrayString Scope::List(void)
{
    wxArrayString ScopeList;

    ScopeList.Add(_T("None"));
#ifdef GUIDE_ASCOM
    wxArrayString ascomScopes = ScopeASCOM::EnumAscomScopes();
    for (unsigned int i = 0; i < ascomScopes.Count(); i++)
        ScopeList.Add(ascomScopes[i]);
#endif
#ifdef GUIDE_ONCAMERA
    ScopeList.Add(_T("On-camera"));
#endif
#ifdef GUIDE_ONSTEPGUIDER
    ScopeList.Add(_T("On-AO"));
#ifdef GUIDE_GPUSB
    ScopeList.Add(_T("GPUSB"));
#endif
#ifdef GUIDE_GPINT
    ScopeList.Add(_T("GPINT 3BC"));
    ScopeList.Add(_T("GPINT 378"));
    ScopeList.Add(_T("GPINT 278"));
#endif
#endif
#ifdef GUIDE_VOYAGER
    ScopeList.Add(_T("Voyager"));
#endif
#ifdef GUIDE_EQUINOX
    ScopeList.Add(_T("Equinox 6"));
#endif
#ifdef GUIDE_EQUINOX
    ScopeList.Add(_T("EQMAC"));
#endif
#ifdef GUIDE_GCUSBST4
    ScopeList.Add(_T("GC USB ST4"));
#endif

    return ScopeList;
}

Scope *Scope::Factory(wxString choice)
{
    Scope *pReturn = NULL;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("ScopeFactory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("ScopeFactory(%s)", choice));

        if (false) // so else ifs can follow
        {
        }
#ifdef GUIDE_ASCOM
        // do ASCOM first since it includes choices that could match stings belop like Simulator
        else if (choice.Find(_T("ASCOM")) != wxNOT_FOUND) {
            pReturn = new ScopeASCOM(choice);
        }
#endif
        else if (choice.Find(_T("None")) + 1) {
        }
#ifdef GUIDE_ONCAMERA
        else if (choice.Find(_T("On-camera")) + 1) {
            pReturn = new ScopeOnCamera();
        }
#endif
#ifdef GUIDE_ONSTEPGUIDER
        else if (choice.Find(_T("On-AO")) + 1) {
            pReturn = new ScopeOnStepGuider();
        }
#endif
#ifdef GUIDE_GPUSB
        else if (choice.Find(_T("GPUSB")) + 1) {
            pReturn = new ScopeGpUsb();
        }
#endif
#ifdef GUIDE_GPINT
        else if (choice.Find(_T("GPINT 3BC")) + 1) {
            pReturn = new ScopeGpInt((short) 0x3BC);
        }
        else if (choice.Find(_T("GPINT 378")) + 1) {
            pReturn = new ScopeGpInt((short) 0x378);
        }
        else if (choice.Find(_T("GPINT 278")) + 1) {
            pReturn = new ScopeGpInt((short) 0x278);
        }
#endif
#ifdef GUIDE_VOYAGER
        else if (choice.Find(_T("Voyager")) + 1) {
            This needs work.  We have to move the setting of the IP address
                into the connect routine
            ScopeVoyager *pVoyager = new ScopeVoyager();
        }
#endif
#ifdef GUIDE_EQUINOX
        else if (choice.Find(_T("Equinox 6")) + 1) {
            pReturn = new ScopeEquinox();
        }
#endif
#ifdef GUIDE_EQMAC
        else if (choice.Find(_T("EQMAC")) + 1) {
            pReturn = new ScopeEQMac();
        }
#endif
#ifdef GUIDE_GCUSBST4
        else if (choice.Find(_T("GC USB ST4")) + 1) {
            pReturn = new ScopeGCUSBST4();
        }
#endif
#ifdef GUIDE_INDI
        This is broken
#endif
        else {
            throw ERROR_INFO("ScopeFactory: Unknown Scope choice");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (pReturn)
        {
            delete pReturn;
            pReturn = NULL;
        }
    }

    return pReturn;
}

bool Scope::GuidingCeases(void)
{
    // for scopes, we have nothing special to do when guiding stops
    return false;
}

double Scope::GetDeclination(void)
{
    return Mount::GetDeclination();
}

bool Scope::RequiresCamera(void)
{
    return false;
}

bool Scope::RequiresStepGuider(void)
{
    return false;
}

bool Scope::CalibrationMove(GUIDE_DIRECTION direction)
{
    bool bError = false;

    Debug.AddLine(wxString::Format("scope calibration move dir= %d dur= %d", direction, m_calibrationDuration));

    try
    {
        double duration = Move(direction, m_calibrationDuration, false);

        if (duration < 0)
        {
            throw THROW_INFO("Duration < 0");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

double Scope::Move(GUIDE_DIRECTION direction, double duration, bool normalMove)
{
    try
    {
        Debug.AddLine("Move(%d, %lf, %d)", direction, duration, normalMove);

        if (!m_guidingEnabled)
        {
            duration = 0.0;
            throw THROW_INFO("Guiding disabled");
        }

        // Compute the actual guide durations

        switch (direction)
        {
            case NORTH:
            case SOUTH:

                // Enforce dec guiding mode and max dec duration for normal moves
                if (normalMove)
                {
                    if ((m_decGuideMode == DEC_NONE) ||
                        (direction == SOUTH && m_decGuideMode == DEC_NORTH) ||
                        (direction == NORTH && m_decGuideMode == DEC_SOUTH))
                    {
                        duration = 0.0;
                        Debug.AddLine("duration set to 0.0 by GuideMode");
                    }

                    if  (duration > m_maxDecDuration)
                    {
                        duration = m_maxDecDuration;
                        Debug.AddLine("duration set to %lf by maxDecDuration", duration);
                    }
                }
                break;
            case EAST:
            case WEST:

                if (normalMove)
                {
                    // enforce max RA duration for normal moves
                    if (duration > m_maxRaDuration)
                    {
                        duration = m_maxRaDuration;
                        Debug.AddLine("duration set to %lf by maxRaDuration", duration);
                    }
                }

                break;
        }

        // Actually do the guide
        assert(duration >= 0.0);
        if (duration > 0.0)
        {
            if (Guide(direction, duration))
            {
                throw ERROR_INFO("guide failed");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        duration = -1.0;
    }

    Debug.AddLine(wxString::Format("Move returns %.2f", duration));

    return duration;
}

void Scope::ClearCalibration(void)
{
    Mount::ClearCalibration();

    m_calibrationState = CALIBRATION_STATE_CLEARED;
}

bool Scope::BeginCalibration(const PHD_Point& currentLocation)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Not connected");
        }

        if (!currentLocation.IsValid())
        {
            throw ERROR_INFO("Must have a valid lock position");
        }

        ClearCalibration();
        m_calibrationSteps = 0;
        m_calibrationStartingLocation = currentLocation;
        m_calibrationState = CALIBRATION_STATE_GO_WEST;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Scope::SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination)
{
    m_calibrationXAngle = xAngle;
    m_calibrationXRate = xRate;
    m_calibrationYAngle = yAngle;
    m_calibrationYRate = yRate;
    Mount::SetCalibration(xAngle, yAngle, xRate, yRate, declination);
}

bool Scope::IsCalibrated(void)
{
    if (!Mount::IsCalibrated())
        return false;

    switch (m_decGuideMode)
    {
    case DEC_NONE:
        return true;
    case DEC_AUTO:
    case DEC_NORTH:
    case DEC_SOUTH:
        {
            bool have_ns_calibration = m_calibrationYAngle != 0.0 || m_calibrationYRate != 1.0;
            return have_ns_calibration;
        }
    default:
        assert(false);
        return true;
    }
}

bool Scope::UpdateCalibrationState(const PHD_Point &currentLocation)
{
    bool bError = false;

    try
    {
        wxString status0, status1;
        double dX = m_calibrationStartingLocation.dX(currentLocation);
        double dY = m_calibrationStartingLocation.dY(currentLocation);
        double dist = m_calibrationStartingLocation.Distance(currentLocation);
        double dist_crit = wxMin(pCamera->FullSize.GetHeight() * 0.05, MAX_CALIBRATION_DISTANCE);

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_CLEARED:
                assert(false);
                break;
            case CALIBRATION_STATE_GO_WEST:
                if (dist < dist_crit)
                {
                    if (m_calibrationSteps++  > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(_("RA Calibration Failed: star did not move enough"));
                        wxMessageBox(msg, _("Alert"), wxOK | wxICON_ERROR);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("Calibrate failed");
                    }
                    status0.Printf(_("West step %3d"), m_calibrationSteps);
                    GuideLog.CalibrationStep(this, "West", m_calibrationSteps, dX, dY, currentLocation, dist);
                    pFrame->ScheduleCalibrationMove(this, WEST);
                    break;
                }

                m_calibrationXAngle = m_calibrationStartingLocation.Angle(currentLocation);
                m_calibrationXRate = dist/(m_calibrationSteps * m_calibrationDuration);

                Debug.AddLine(wxString::Format("WEST calibration completes with angle=%.2f rate=%.4f", m_calibrationXAngle, m_calibrationXRate));
                status1.Printf(_("angle=%.2f rate=%.4f"), m_calibrationXAngle, m_calibrationXRate);
                GuideLog.CalibrationDirectComplete(this, "West", m_calibrationXAngle, m_calibrationXRate);

                m_calibrationState = CALIBRATION_STATE_GO_EAST;
                // fall through
                Debug.AddLine("Falling Through to state GO_EAST");
            case CALIBRATION_STATE_GO_EAST:
                if (m_calibrationSteps > 0)
                {
                    status0.Printf(_("East step %3d"), m_calibrationSteps);
                    GuideLog.CalibrationStep(this, "East", m_calibrationSteps, dX, dY, currentLocation, dist);
                    m_calibrationSteps--;
                    pFrame->ScheduleCalibrationMove(this, EAST);
                    break;
                }
                m_calibrationSteps = 0; dist = 0.0;
                m_calibrationStartingLocation = currentLocation;

                if (m_decGuideMode == DEC_NONE)
                {
                    m_calibrationState = CALIBRATION_STATE_COMPLETE;
                    // these values uniquely indicate lack of Dec calibration data.
                    // If you change them, you need to update Scope::IsCalibrated.
                    m_calibrationYAngle = 0.0;
                    m_calibrationYRate = 1.0;
                    break;
                }

                m_calibrationState = CALIBRATION_STATE_CLEAR_BACKLASH;
                // fall through
                Debug.AddLine("Falling Through to state CLEAR_BACKLASH");
            case CALIBRATION_STATE_CLEAR_BACKLASH:
                if (dist < DEC_BACKLASH_DISTANCE)
                {
                    if (m_calibrationSteps++ > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(_("Backlash Clearing Failed: star did not move enough"));
                        wxMessageBox(msg, _("Alert"), wxOK | wxICON_ERROR);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("Calibrate failed");
                    }
                    status0.Printf(_("Clear backlash step %3d"), m_calibrationSteps);
                    GuideLog.CalibrationStep(this, "Backlash", m_calibrationSteps, dX, dY, currentLocation, dist);
                    pFrame->ScheduleCalibrationMove(this, NORTH);
                    break;
                }
                m_calibrationSteps = 0;
                dist = 0.0;
                m_calibrationStartingLocation = currentLocation;
                m_calibrationState = CALIBRATION_STATE_GO_NORTH;
                // fall through
                Debug.AddLine("Falling Through to state GO_NORTH");
            case CALIBRATION_STATE_GO_NORTH:
                if (dist < dist_crit)
                {
                    if (m_calibrationSteps++ > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(_("DEC Calibration Failed: star did not move enough"));
                        wxMessageBox(msg, _("Alert"), wxOK | wxICON_ERROR);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("Calibrate failed");
                    }
                    status0.Printf(_("North step %3d"), m_calibrationSteps);
                    GuideLog.CalibrationStep(this, "North", m_calibrationSteps, dX, dY, currentLocation, dist);
                    pFrame->ScheduleCalibrationMove(this, NORTH);
                    break;
                }

                // note: this calculation is reversed from the ra calculation, becase
                // that one was calibrating WEST, but the angle is really relative
                // to EAST
                m_calibrationYAngle = currentLocation.Angle(m_calibrationStartingLocation);
                m_calibrationYRate = dist/(m_calibrationSteps * m_calibrationDuration);

                Debug.AddLine(wxString::Format("NORTH calibration completes with angle=%.2f rate=%.4f", m_calibrationYAngle, m_calibrationYRate));
                status1.Printf(_("angle=%.2f rate=%.4f"), m_calibrationYAngle, m_calibrationYRate);
                GuideLog.CalibrationDirectComplete(this, "North", m_calibrationYAngle, m_calibrationYRate);

                m_calibrationState = CALIBRATION_STATE_GO_SOUTH;
                // fall through
                Debug.AddLine("Falling Through to state GO_SOUTH");
            case CALIBRATION_STATE_GO_SOUTH:
                if (m_calibrationSteps > 0)
                {
                    status0.Printf(_("South step %3d"), m_calibrationSteps);
                    GuideLog.CalibrationStep(this, "South", m_calibrationSteps, dX, dY, currentLocation, dist);
                    m_calibrationSteps--;
                    pFrame->ScheduleCalibrationMove(this, SOUTH);
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_COMPLETE;
                // fall through
                Debug.AddLine("Falling Through to state CALIBRATION_COMPLETE");
            case CALIBRATION_STATE_COMPLETE:
                SetCalibration(m_calibrationXAngle, m_calibrationYAngle,
                               m_calibrationXRate,  m_calibrationYRate,
                               GetDeclination());
                pFrame->SetSampling();
                pFrame->SetStatusText(_("calibration complete"),1);
                GuideLog.CalibrationComplete(this);
                EvtServer.NotifyCalibrationComplete(this);
                Debug.AddLine("Calibration Complete");
                break;
        }

        if (m_calibrationState != CALIBRATION_STATE_COMPLETE)
        {
            if (status1.IsEmpty())
            {
                double dX = m_calibrationStartingLocation.dX(currentLocation);
                double dY = m_calibrationStartingLocation.dY(currentLocation);
                double dist = m_calibrationStartingLocation.Distance(currentLocation);

                status1.Printf(_("dx=%4.1f dy=%4.1f dist=%4.1f"), dX, dY, dist);
            }
        }

        if (!status0.IsEmpty())
        {
            pFrame->SetStatusText(status0, 0);
        }

        if (!status1.IsEmpty())
        {
            pFrame->SetStatusText(status1, 1);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        ClearCalibration();

        bError = true;
    }

    return bError;
}

wxString Scope::GetSettingsSummary() {
    // return a loggable summary of current mount settings
    return Mount::GetSettingsSummary() +
        wxString::Format("Calibration step %d, Max RA duration = %d, Max DEC duration = %d, DEC guide mode = %s\n",
            GetCalibrationDuration(),
            GetMaxRaDuration(),
            GetMaxDecDuration(),
            GetDecGuideMode() == DEC_NONE ? "off" : GetDecGuideMode() == DEC_AUTO ? "Auto" :
            GetDecGuideMode() == DEC_NORTH ? "north" : "south"
        );
}

wxString Scope::GetMountClassName() const
{
    return wxString("scope");
}

ConfigDialogPane *Scope::GetConfigDialogPane(wxWindow *pParent)
{
    return new ScopeConfigDialogPane(pParent, this);
}

Scope::ScopeConfigDialogPane::ScopeConfigDialogPane(wxWindow *pParent, Scope *pScope)
    : MountConfigDialogPane(pParent, "Scope", pScope)
{
    int width;

    m_pScope = pScope;

    width = StringWidth(_T("00000"));
    m_pCalibrationDuration = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 1000,_T("Cal_Dur"));

    DoAdd(_("Calibration step (ms)"), m_pCalibrationDuration,
        _("How long a guide pulse should be used during calibration? Default = 750ms, increase for short f/l scopes and decrease for longer f/l scopes"));

    width = StringWidth(_T("00000"));
    m_pMaxRaDuration = new wxSpinCtrl(pParent,wxID_ANY,_T("foo"),wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 2000, 150, _T("MaxDec_Dur"));
    DoAdd(_("Max RA Duration"),  m_pMaxRaDuration,
          _("Longest length of pulse to send in RA\nDefault = 1000 ms. "));

    width = StringWidth(_T("00000"));
    m_pMaxDecDuration = new wxSpinCtrl(pParent,wxID_ANY,_T("foo"),wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS,0,2000,150,_T("MaxDec_Dur"));
    DoAdd(_("Max Dec Duration"),  m_pMaxDecDuration,
          _("Longest length of pulse to send in declination\nDefault = 100 ms.  Increase if drift is fast."));

    wxString dec_choices[] = {
        _("Off"),_("Auto"),_("North"),_("South")
    };
    width = StringArrayWidth(dec_choices, WXSIZEOF(dec_choices));
    m_pDecMode= new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(dec_choices), dec_choices);
    DoAdd(_("Dec guide mode"), m_pDecMode,
          _("Guide in declination as well?"));
}

Scope::ScopeConfigDialogPane::~ScopeConfigDialogPane(void)
{
}

void Scope::ScopeConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationDuration->SetValue(m_pScope->GetCalibrationDuration());
    m_pMaxRaDuration->SetValue(m_pScope->GetMaxRaDuration());
    m_pMaxDecDuration->SetValue(m_pScope->GetMaxDecDuration());
    m_pDecMode->SetSelection(m_pScope->GetDecGuideMode());

}

void Scope::ScopeConfigDialogPane::UnloadValues(void)
{
    m_pScope->SetCalibrationDuration(m_pCalibrationDuration->GetValue());
    m_pScope->SetMaxRaDuration(m_pMaxRaDuration->GetValue());
    m_pScope->SetMaxDecDuration(m_pMaxDecDuration->GetValue());
    m_pScope->SetDecGuideMode(m_pDecMode->GetSelection());

    MountConfigDialogPane::UnloadValues();
}

GraphControlPane *Scope::GetGraphControlPane(wxWindow *pParent, wxString label)
{
    return new ScopeGraphControlPane(pParent, this, label);
}

Scope::ScopeGraphControlPane::ScopeGraphControlPane(wxWindow *pParent, Scope *pScope, wxString label)
    : GraphControlPane(pParent, label)
{
    int width;
    m_pScope = pScope;

    width = StringWidth(_T("0000"));
    m_pMaxRaDuration = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width+30, -1),
        wxSP_ARROW_KEYS, 0,2000,0);
    m_pMaxRaDuration->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &Scope::ScopeGraphControlPane::OnMaxRaDurationSpinCtrl, this);
    DoAdd(m_pMaxRaDuration, _("Mx RA"));

    width = StringWidth(_T("0000"));
    m_pMaxDecDuration = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width+30, -1),
        wxSP_ARROW_KEYS, 0,2000,0);
    m_pMaxDecDuration->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &Scope::ScopeGraphControlPane::OnMaxRaDurationSpinCtrl, this);
    DoAdd(m_pMaxDecDuration, _("Mx DEC"));

    wxString dec_choices[] = { _("Off"),_("Auto"),_("North"),_("South") };
    m_pDecMode = new wxChoice(this, wxID_ANY,
        wxDefaultPosition,wxDefaultSize, WXSIZEOF(dec_choices), dec_choices );
    m_pDecMode->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &Scope::ScopeGraphControlPane::OnDecModeChoice, this);
    m_pControlSizer->Add(m_pDecMode);

    m_pMaxRaDuration->SetValue(m_pScope->GetMaxRaDuration());
    m_pMaxDecDuration->SetValue(m_pScope->GetMaxDecDuration());
    m_pDecMode->SetSelection(m_pScope->GetDecGuideMode());
}

Scope::ScopeGraphControlPane::~ScopeGraphControlPane()
{
}

void Scope::ScopeGraphControlPane::OnMaxRaDurationSpinCtrl(wxSpinEvent& WXUNUSED(evt))
{
    m_pScope->SetMaxRaDuration(m_pMaxRaDuration->GetValue());
    GuideLog.SetGuidingParam("Max RA duration", m_pMaxRaDuration->GetValue());
}

void Scope::ScopeGraphControlPane::OnMaxDecDurationSpinCtrl(wxSpinEvent& WXUNUSED(evt))
{
    m_pScope->SetMaxDecDuration(m_pMaxDecDuration->GetValue());
    GuideLog.SetGuidingParam("Max DEC duration", m_pMaxDecDuration->GetValue());
}

void Scope::ScopeGraphControlPane::OnDecModeChoice(wxCommandEvent& WXUNUSED(evt))
{
    m_pScope->SetDecGuideMode(m_pDecMode->GetSelection());
    wxString dec_choices[] = { _("Off"),_("Auto"),_("North"),_("South") };
    GuideLog.SetGuidingParam("DEC guide mode", dec_choices[m_pDecMode->GetSelection()]);
}
