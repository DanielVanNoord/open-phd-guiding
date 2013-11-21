/*
 *  eegg.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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
#include "drift_tool.h"
#include "manualcal_dialog.h"
#include "calrestore_dialog.h"

void TestGuide() {

#ifdef BRET_TODO
    wxMessageBox(_("W RA+")); wxGetApp().Yield(); pMount->Guide(WEST,2000); wxGetApp().Yield();
    wxMessageBox(_("N Dec+"));  wxGetApp().Yield(); pMount->Guide(NORTH,2000);wxGetApp().Yield();
    wxMessageBox(_("E RA-"));  wxGetApp().Yield(); pMount->Guide(EAST,2000);wxGetApp().Yield();
    wxMessageBox(_("S Dec-"));  wxGetApp().Yield(); pMount->Guide(SOUTH,2000);wxGetApp().Yield();
    wxMessageBox(_("Done"));
#endif
}

void MyFrame::OnEEGG(wxCommandEvent &evt)
{
    if (evt.GetId() == EEGG_TESTGUIDEDIR)
    {
        if (!pMount || !pMount->IsConnected())
        {
            wxMessageBox(_("Please connect a Mount to Manual Guide."));
            return;
        }
        TestGuide();
    }
    else if (evt.GetId() == EEGG_RANDOMMOTION)
    {
        RandomMotionMode = !RandomMotionMode;
        wxMessageBox(wxString::Format(_T("Random motion mode set to %d"),(int) RandomMotionMode));
    }
    else if (evt.GetId() == EEGG_RESTORECAL)
    {
        wxString savedCal = pConfig->Profile.GetString("/scope/calibration/timestamp", wxEmptyString);
        if (savedCal.IsEmpty())
        {
            wxMessageBox(_("There is no calibration data available."));
            return;
        }

        CalrestoreDialog dlg;
        dlg.Show();
        if (dlg.ShowModal() == wxID_OK)
        {
            pFrame->LoadCalibration();
        }
    }
    else if (evt.GetId() == EEGG_MANUALCAL)
    {
        if (pMount)
        {
            double xRate  = pMount->xRate();
            double yRate  = pMount->yRate();
            double xAngle = pMount->xAngle();
            double yAngle = pMount->yAngle();
            double declination = pMount->GetDeclination();

            if (!pMount->IsCalibrated())
            {
                xRate       = 1.0;
                yRate       = 1.0;
                xAngle      = 0.0;
                yAngle      = M_PI/2;
                declination = 0.0;
            }

            ManualCalDialog manualcal(xRate, yRate, xAngle, yAngle, declination);
            if (manualcal.ShowModal () == wxID_OK)
            {
                manualcal.GetValues(&xRate, &yRate, &xAngle, &yAngle, &declination);
                pMount->SetCalibration(xAngle, yAngle, xRate, yRate, declination, pMount->SideOfPier());
            }
        }
    }
    else if (evt.GetId() == EEGG_CLEARCAL)
    {
        if (pMount)
        {
            pMount->ClearCalibration();
        }
        if (pSecondaryMount)
        {
            pSecondaryMount->ClearCalibration();
        }
    }
    else if (evt.GetId() == EEGG_FLIPRACAL)
    {
        Mount *mount = pSecondaryMount ? pSecondaryMount : pMount;

        if (mount)
        {
            double xorig = mount->xAngle() * 180. / M_PI;
            double yorig = mount->yAngle() * 180. / M_PI;

            if (FlipRACal())
            {
                wxMessageBox(_("Failed to flip RA calibration"));
            }
            else
            {
                double xnew = mount->xAngle() * 180. / M_PI;
                double ynew = mount->yAngle() * 180. / M_PI;
                wxMessageBox(wxString::Format(_("RA calibration angle flipped: (%.2f, %.2f) to (%.2f, %.2f)"),
                    xorig, yorig, xnew, ynew));
            }
        }
    }
    else if (evt.GetId() == EEGG_MANUALLOCK)
    {
        if (!pMount || !pMount->IsCalibrated() || !pCamera || !pCamera->Connected)
        {
            wxMessageBox(_("Entering a manual lock position requires a camera and mount to be connected and calibrated."));
            return;
        }
        if (pGuider->GetState() > STATE_SELECTED)
        {
            wxMessageBox(_("Entering a manual lock position cannot be done while calibrating or guiding."));
            return;  // must not be calibrating or guiding already
        }

        double LockX=0.0, LockY=0.0;
        PHD_Point curLock = pFrame->pGuider->LockPosition();
        wxString tmpstr;
        if (curLock.IsValid())
            tmpstr = wxString::Format("%.3f", curLock.X);
        do
        {
            tmpstr = wxGetTextFromUser(_("Enter x-lock position, or 0 for center"), _("X-lock position"), tmpstr);
        } while (!tmpstr.IsEmpty() && !tmpstr.ToDouble(&LockX));
        if (tmpstr.IsEmpty())
            return;
        LockX = fabs(LockX);
        if (LockX < 0.0001) {
            LockX = pCamera->FullSize.GetWidth() / 2;
            LockY = pCamera->FullSize.GetHeight() / 2;
        }
        else {
            tmpstr = "";
            if (curLock.IsValid())
                tmpstr = wxString::Format("%.3f", curLock.Y);
            do {
                tmpstr = wxGetTextFromUser(_("Enter y-lock position"), _("Y-lock position"), tmpstr);
            } while (!tmpstr.IsEmpty() && !tmpstr.ToDouble(&LockY));
            if (tmpstr.IsEmpty())
                return;
            LockY = fabs(LockY);
        }
        pFrame->pGuider->SetLockPosition(PHD_Point(LockX, LockY));
        pFrame->pGuider->SetLockPosIsSticky(true);
        pFrame->tools_menu->FindItem(EEGG_STICKY_LOCK)->Check(true);
    }
    else if (evt.GetId() == EEGG_STICKY_LOCK)
    {
        bool sticky = evt.IsChecked();
        pFrame->pGuider->SetLockPosIsSticky(sticky);
        pConfig->Global.SetBoolean("/StickyLockPosition", sticky);
    }
    else
    {
        evt.Skip();
    }
}

void MyFrame::OnDriftTool(wxCommandEvent& WXUNUSED(evt))
{
    if (!pDriftTool)
    {
        pDriftTool = DriftTool::CreateDriftToolWindow();
    }

    pDriftTool->Show();
}
