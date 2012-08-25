/*
 *  prefs.cpp
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
#include <wx/config.h>
#include <wx/fileconf.h>

// Some specific camera includes
#if defined (LE_PARALLEL_CAMERA)
#include "cam_LEwebcam.h"
extern Camera_LEwebcamClass Camera_LEwebcamParallel;
extern Camera_LEwebcamClass Camera_LEwebcamLXUSB;
#endif

#if defined (INDI_CAMERA)
#include "cam_INDI.h"
#endif

#if defined (GUIDE_INDI)
#include "tele_INDI.h"
#endif

void MyFrame::ReadPreferences(wxString fname) {
	long lval;
	wxString sval;
	//bool bval = false;
	//double dval;
	wxConfig *config;
	
	if (fname.IsEmpty()) 
		config = new wxConfig(_T("PHDGuiding"));
	else {
		config = (wxConfig*) new wxFileConfig(_T("PHDGuiding"),"",fname);
	}
//	wxConfig *config = new wxConfig(_T("PHDGuiding"));

	// Current mount
	lval=0;
	if (!config->Read(_T("Mount"),&lval)) {
		wxMessageBox("This does not appear to be a valid PHD Settings file");
		return;
	}
	switch (lval) {
		case 0:
			mount_menu->Check(MOUNT_ASCOM,true);
			break;
		case 1: mount_menu->Check(MOUNT_GPUSB,true); break;
		case 2: mount_menu->Check(MOUNT_GPINT3BC,true); break;
		case 3: mount_menu->Check(MOUNT_GPINT378,true); break;
		case 4: mount_menu->Check(MOUNT_GPINT278,true); break;
		case 5: mount_menu->Check(MOUNT_CAMERA,true); break;
		case 6: mount_menu->Check(MOUNT_VOYAGER,true); break;
		case 7: mount_menu->Check(MOUNT_EQUINOX,true); break;
		case 8: mount_menu->Check(MOUNT_GCUSBST4,true); break;
		case 9: mount_menu->Check(MOUNT_INDI,true); break;
		case 10: mount_menu->Check(MOUNT_EQMAC,true); break;
//		default: mount_menu->Check(MOUNT_ASCOM,true);
	}
	config->Read(_T("RA Aggressiveness"),&RA_aggr);
	config->Read(_T("RA Hysteresis"),&RA_hysteresis);
	lval = (long) Cal_duration;
	config->Read(_T("Cal Duration"), &lval);
	Cal_duration = (int) lval;
	lval = (long) SearchRegion;
	config->Read(_T("Search Region"), &lval);
	SearchRegion = (int) lval;
	config->Read(_T("Min Motion"),&MinMotion);
	config->Read(_T("Star Mass Tolerance"),&StarMassChangeRejectThreshold);
	config->Read(_T("Log"),&Log_Data);
	config->Read(_T("Dither RA Only"),&DitherRAOnly);
	config->Read(_T("Dither Scale Factor"),&DitherScaleFactor);
	config->Read(_T("Subframes"),&GuideCameraPrefs::UseSubframes);
	lval = (long) Dec_guide;
	config->Read(_T("Dec guide mode"),&lval);
	Dec_guide = (int) lval;
	lval = (long) Dec_algo;
	config->Read(_T("Dec algorithm"),&lval);
	Dec_algo = (int) lval;
	lval = (long) Max_Dec_Dur;
	config->Read(_T("Dec slope weight"),&Dec_slopeweight);
	config->Read(_T("Max Dec Dur"),&lval);
	Max_Dec_Dur = (int) lval;
	lval = (long) Max_RA_Dur;
	config->Read(_T("Max RA Dur"),&lval);
	Max_RA_Dur = (int) lval;
	lval = (long) Time_lapse;
	config->Read(_T("Time Lapse"),&lval);
    Time_lapse = (int) lval;
	lval = (long) GuideCameraPrefs::GuideCameraGain;
	config->Read(_T("Gain"),&lval);
    GuideCameraPrefs::GuideCameraGain = (int) lval;
	lval = (long) GuideCameraPrefs::NR_mode;
	config->Read(_T("NRMode"),&lval);
    GuideCameraPrefs::NR_mode = (int) lval;
	config->Read(_T("Gamma"),&Stretch_gamma);
#if defined (LE_PARALLEL_CAMERA)
	lval = (long) Camera_LEwebcamParallel.Port;
	config->Read(_T("LEwebP port"),&lval);
	Camera_LEwebcamParallel.Port = (short) lval;
	lval = (long) Camera_LEwebcamParallel.Delay;
	config->Read(_T("LEwebP delay"),&lval);
	Camera_LEwebcamParallel.Delay = (short) lval;
	lval = (long) Camera_LEwebcamLXUSB.Delay;
	config->Read(_T("LEwebLXUSB delay"),&lval);
	Camera_LEwebcamLXUSB.Delay = (short) lval;
#endif
#if defined INDI_CAMERA
    config->Read(_T("INDIcam"), &Camera_INDI.indi_name);
#endif
#if defined GUIDE_INDI
    config->Read(_T("INDImount"), &INDIScope.indi_name);
    config->Read(_T("INDImount_port"), &INDIScope.serial_port);
#endif
	lval = AdvDlg_fontsize;
	config->Read(_T("Advanced Dialog Fontsize"),&lval);
	AdvDlg_fontsize = (int) lval;
	lval = (long) ServerMode;
	config->Read(_T("Enable Server"),&lval);
	ServerMode = lval > 0;
	sval = GraphLog->RA_Color.GetAsString();
	config->Read(_T("RAColor"),&sval);
	if (!sval.IsEmpty()) GraphLog->RA_Color.Set(sval);
	sval = GraphLog->DEC_Color.GetAsString();
	config->Read(_T("DECColor"),&sval);
	if (!sval.IsEmpty()) GraphLog->DEC_Color.Set(sval);

	delete config;
	return;
}

void MyFrame::WritePreferences(wxString fname) {
	wxConfig *config;
	
	if (fname.IsEmpty()) 
		config = new wxConfig(_T("PHDGuiding"));
	else {
		config = (wxConfig* ) new wxFileConfig(_T("PHDGuiding"),"",fname);
	}


	// Current mount
	long mount=0;
	if (mount_menu->IsChecked(MOUNT_ASCOM)) mount = 0;
	else if (mount_menu->IsChecked(MOUNT_GPUSB)) mount = 1;
	else if (mount_menu->IsChecked(MOUNT_GPINT3BC)) mount = 2;
	else if (mount_menu->IsChecked(MOUNT_GPINT378)) mount = 3;
	else if (mount_menu->IsChecked(MOUNT_GPINT278)) mount = 4;
	else if (mount_menu->IsChecked(MOUNT_CAMERA)) mount = 5;
	else if (mount_menu->IsChecked(MOUNT_VOYAGER)) mount = 6;
	else if (mount_menu->IsChecked(MOUNT_EQUINOX)) mount = 7;
	else if (mount_menu->IsChecked(MOUNT_GCUSBST4)) mount = 8;
	else if (mount_menu->IsChecked(MOUNT_INDI)) mount = 9;
	else if (mount_menu->IsChecked(MOUNT_EQMAC)) mount = 10;
	config->Write(_T("Mount"),mount);
	config->Write(_T("RA Aggressiveness"),RA_aggr);
	config->Write(_T("RA Hysteresis"),RA_hysteresis);
	config->Write(_T("Cal Duration"),(long) Cal_duration);
	config->Write(_T("Min Motion"),MinMotion);
	config->Write(_T("Star Mass Tolerance"),StarMassChangeRejectThreshold);
	config->Write(_T("Search Region"),(long) SearchRegion);
	config->Write(_T("Time Lapse"),(long) Time_lapse);
	config->Write(_T("Gain"),(long) GuideCameraPrefs::GuideCameraGain);
	config->Write(_T("NRMode"),(long) GuideCameraPrefs::NR_mode);
	config->Write(_T("Log"),Log_Data);
	config->Write(_T("Dither RA Only"),DitherRAOnly);
	config->Write(_T("Dither Scale Factor"),DitherScaleFactor);
	config->Write(_T("Dec guide mode"),(long) Dec_guide);
	config->Write(_T("Dec algorithm"),(long) Dec_algo);
	config->Write(_T("Dec slope weight"),Dec_slopeweight);
	config->Write(_T("Max Dec Dur"), (long) Max_Dec_Dur);
	config->Write(_T("Max RA Dur"), (long) Max_RA_Dur);
	config->Write(_T("Subframes"),GuideCameraPrefs::UseSubframes);
#if defined (LE_PARALLEL_CAMERA)
	config->Write(_T("LEwebP port"),(long) Camera_LEwebcamParallel.Port);
	config->Write(_T("LEwebP delay"),(long) Camera_LEwebcamParallel.Delay);
	config->Write(_T("LEwebLXUSB delay"),(long) Camera_LEwebcamLXUSB.Delay);
#endif
#if defined INDI_CAMERA
    config->Write(_T("INDIcam"), Camera_INDI.indi_name);
#endif
#if defined GUIDE_INDI
    config->Write(_T("INDImount"), INDIScope.indi_name);
    config->Write(_T("INDImount_port"), INDIScope.serial_port);
#endif
	config->Write(_T("Advanced Dialog Fontsize"),(long)AdvDlg_fontsize);
	config->Write(_T("Gamma"),Stretch_gamma);
	config->Write(_T("Enable Server"),(long) ServerMode);
	config->Write(_T("RAColor"),GraphLog->RA_Color.GetAsString());
	config->Write(_T("DECColor"),GraphLog->DEC_Color.GetAsString());
	
	delete config;
}

void MyFrame::OnSettings(wxCommandEvent &evt) {
	wxString fname;
	if (evt.GetId() == MENU_SAVESETTINGS) {
		fname = wxFileSelector(_T("Save settings file"),wxEmptyString, wxEmptyString,
							   _T("txt"), wxT("Text settings files (*.txt)|*.txt"),
							   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!fname.EndsWith(_T(".txt")))
			fname = fname + _T(".txt");
		WritePreferences(fname);
	}
	else if (evt.GetId() == MENU_LOADSETTINGS) {
		fname = wxFileSelector(_T("Load settings file"),wxEmptyString, wxEmptyString,
							   _T("txt"), wxT("Text settings files (*.txt)|*.txt"),
							   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		//if (~fname.EndsWith(_T(".txt")))
		//	fname = fname + _T(".txt");
		ReadPreferences(fname);
	}
	
}
