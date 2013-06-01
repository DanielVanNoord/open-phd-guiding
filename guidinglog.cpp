/*
 *  guidinglog.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012-2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

GuidingLog::GuidingLog(bool active)
    : m_image_logging_enabled(false),
      m_logged_image_format(LIF_LOW_Q_JPEG)
{
    if (active)
    {
        EnableLogging();
    }
}

GuidingLog::~GuidingLog(void)
{
}

bool GuidingLog::EnableLogging(void)
{
    bool bError = false;

    if (!m_enabled) {
        try
        {
            wxDateTime now = wxDateTime::Now();
            if (!m_file.IsOpened())
            {
                wxStandardPathsBase& stdpath = wxStandardPaths::Get();

                wxString fileName = stdpath.GetDocumentsDir() + PATHSEPSTR + "PHD_GuideLog" + now.Format(_T("_%Y-%m-%d")) +  now.Format(_T("_%H%M%S"))+ ".txt";

                if (!m_file.Open(fileName, "wb"))
                {
                    throw ERROR_INFO("unable to open file");
                }
            }

            assert(m_file.IsOpened());

            m_file.Write(_T("PHD version ") VERSION PHDSUBVER _T(". Log enabled at ") + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");
            Flush();

            m_enabled = true;

            // persist state
            pConfig->SetBoolean("/LoggingMode", m_enabled);

            // dump guiding header if logging enabled during guide
            if (pFrame && pFrame->pGuider->GetState() == STATE_GUIDING)
                GuidingHeader();
        }
        catch (wxString Msg)
        {
            POSSIBLY_UNUSED(Msg);
            bError = true;
        }
    }

    return bError;
}

bool GuidingLog::EnableLogging(bool enabled)
{
    return enabled ? EnableLogging() : DisableLogging();
}

bool GuidingLog::DisableLogging(void)
{
    bool bError = false;

    try
    {
        if (m_enabled) {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::Now();

            m_file.Write("\n");
            m_file.Write("Log disabled at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");
            Flush();
        }
        m_enabled = false;
        // persist state
        pConfig->SetBoolean("/LoggingMode", m_enabled);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::IsEnabled(void)
{
    return m_enabled;
}

bool GuidingLog::Flush(void)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());

            if (!m_file.Flush())
            {
                throw ERROR_INFO("unable to flush file");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void GuidingLog::Close(void)
{
    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::Now();

            m_file.Write("\n");
            m_file.Write("Log closed at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

bool GuidingLog::StartCalibration(Mount *pCalibrationMount)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::Now();

            m_file.Write("\n");
            m_file.Write("Calibration Begins at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");

            assert(pCalibrationMount && pCalibrationMount->IsConnected());

            if (pCamera)
                m_file.Write("Camera is " + pCamera->Name + "\n");
             m_file.Write("Mount is " + pCalibrationMount->Name() + "\n");

            m_file.Write(wxString::Format("Lock position = %.3f, %.3f, Star position = %.3f, %.3f\n",
                        pFrame->pGuider->LockPosition().X,
                        pFrame->pGuider->LockPosition().Y,
                        pFrame->pGuider->CurrentPosition().X,
                        pFrame->pGuider->CurrentPosition().Y));
            m_file.Write("Direction,Step,dx,dy,x,y,Dist\n");

            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::CalibrationFailed(Mount *pCalibrationMount, wxString msg)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write(msg); m_file.Write("\n");
            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::CalibrationStep(Mount *pCalibrationMount, wxString direction,
    int steps, double dx, double dy, const PHD_Point &xy, double dist)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            // Direction,Step,dx,dy,x,y,Dist
            m_file.Write(wxString::Format("%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                direction,
                steps,
                dx, dy,
                xy.X, xy.Y,
                dist));
            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::CalibrationDirectComplete(Mount *pCalibrationMount, wxString direction, double angle, double rate)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write(wxString::Format("%s calibration complete. Angle = %.3f, Rate = %.3f\n",
                direction, angle, rate));
            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::CalibrationComplete(Mount *pCalibrationMount)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write(wxString::Format("Calibration complete, mount = %s.\n", pCalibrationMount->Name()));
            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}


bool GuidingLog::StartGuiding()
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::UNow();

            m_frame = 1;
            m_guidingStarted = now;

            m_file.Write("\n");
            m_file.Write("Guiding Begins at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");

            // add common guiding header
            GuidingHeader();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::GuidingHeader(void)
    // output guiding header to log file
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write(pFrame->GetSettingsSummary());
            m_file.Write(pFrame->pGuider->GetSettingsSummary());

            if (pCamera) {
                m_file.Write(pCamera->GetSettingsSummary());
            }

            if (pMount) {
                m_file.Write(pMount->GetSettingsSummary());
            }

            if (pSecondaryMount) {
                m_file.Write("Secondary " + pSecondaryMount->GetSettingsSummary());
            }

            m_file.Write(wxString::Format("Lockposition = %.3f, %.3f\n",
                        pFrame->pGuider->LockPosition().X,
                        pFrame->pGuider->LockPosition().Y));
            m_file.Write("Frame,Time,mount,dx,dy,Theta,RADuration,RADistance,RADirection,DECDuration,DECDistance,DECDirection,StarMass,SNR,ErrorCode\n");

            Flush();
       }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}


bool GuidingLog::GuideStep(Mount *pGuideMount, const PHD_Point& vectorEndpoint,
        double RADuration, double RADistance,
        double DECDuration, double DECDistance, int errorCode)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
           assert(m_file.IsOpened());
            m_file.Write(wxString::Format("%d,%.3lf,%s,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%s,%.3lf,%.3lf,%s,%.lf,%.2f,%d\n",
                    m_frame,
                    (wxDateTime::UNow() - m_guidingStarted).GetMilliseconds().ToDouble() / 1000.0,
                    pGuideMount->Name(),
                    vectorEndpoint.X, vectorEndpoint.Y,
                    vectorEndpoint.Angle(PHD_Point(0,0)),
                    RADuration, RADistance, (RADistance > 0 ? "E" : RADistance < 0 ? "W" : ""),
                    DECDuration, DECDistance, (DECDistance > 0 ? "S" : DECDistance < 0 ? "N" : ""),
                    pFrame->pGuider->StarMass(), pFrame->pGuider->SNR(),

                    errorCode));
            Flush();
            m_frame += 1;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}


bool GuidingLog::StartEntry(void)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write("\n");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::EnableImageLogging(LOGGED_IMAGE_FORMAT fmt)
{
    m_image_logging_enabled = true;
    m_logged_image_format = fmt;
    return true;
}

bool GuidingLog::DisableImageLogging(void)
{
    m_image_logging_enabled = false;
    return true;
}

bool GuidingLog::IsImageLoggingEnabled(void)
{
    return m_image_logging_enabled;
}

LOGGED_IMAGE_FORMAT GuidingLog::LoggedImageFormat(void)
{
    return m_logged_image_format;
}

bool GuidingLog::ServerGuidingDithered(Guider* guider, double dx, double dy)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Server received DITHER, dithered by %.3f, %.3f, new lock pos = %.4f, %.3f\n",
                dx, dy, guider->LockPosition().X, guider->LockPosition().Y));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::ServerSetLockPosition(Guider* guider, const PHD_Point &xy)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Server received SET LOCK POSITION, new lock pos = %.4f, %.3f\n",
                guider->LockPosition().X, guider->LockPosition().Y));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::ServerCommand(Guider* guider, wxString cmd)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Server received %s\n", cmd));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::SetGuidingParam(wxString name, double val)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Guiding parameter change, %s = %f\n", name, val));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::SetGuidingParam(wxString name, int val)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Guiding parameter change, %s = %d\n", name, val));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::SetGuidingParam(wxString name, wxString val)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            m_file.Write(wxString::Format("Guiding parameter change, %s = %s\n", name, val));
            Flush();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}
