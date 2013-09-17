/*
 *  cam_QHY5II.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2012 Craig Stark.
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
#if defined (QHY5II)
#include "camera.h"
//#include "time.h"
#include "image_math.h"
#include "cam_QHY5II.h"

typedef DWORD (CALLBACK* Q5II_DW_V)(void);
//EXPORT DWORD _stdcall openUSB;  Open USB for the selected camera. Returnd 1 is a camera was found, 0 otherwise
Q5II_DW_V Q5II_OpenUSB;
//EXPORT DWORD _stdcall isExposing();  Indicates if the camera is currently performing an exposure
Q5II_DW_V Q5II_IsExposing;

typedef void (CALLBACK* Q5II_V_V)(void);
//EXPORT void _stdcall CancelExposure();  Cancels the current exposure
Q5II_V_V Q5II_CancelExposure;
//EXPORT void _stdcall closeUSB;  Closes the USB connection.
Q5II_V_V Q5II_CloseUSB;
//EXPORT void _stdcall StopCapturing();  Stops any started capturing Thread
Q5II_V_V Q5II_StopCapturing;
//EXPORT void _stdcall SingleExposure();  Clears the buffer and starts a new exposure, stops capturing after one image
Q5II_V_V Q5II_SingleExposure;
//EXPORT void _stdcall SetAutoBlackLevel();  Set automatic black level, QHY5-II Only
// Seems not in there...
//Q5II_V_V Q5II_SetAutoBlackLevel;

typedef void (CALLBACK* Q5II_V_DW)(DWORD);
//EXPORT void _stdcall SetBlackLevel( DWORD n );  QHY5-II Only Sted the blacklevel of the camera (direct register write)
// Seems to actually be the auto black level
Q5II_V_DW Q5II_SetBlackLevel;
//EXPORT void _stdcall SetGain(DWORD x );  Set gainlevel (0-100)
Q5II_V_DW Q5II_SetGain;
//EXPORT void _stdcall SetExposureTime( DWORD milisec ) ;  Set exposuretime in miliseconds
Q5II_V_DW Q5II_SetExposureTime;
//EXPORT void _stdcall SetSpeed( DWORD n );  Set camera speed (0=slow, 1=medium, 2=fast). QHY5L may not support all speeds in higher resolutions.
Q5II_V_DW Q5II_SetSpeed;
//EXPORT void _stdcall SetHBlank( DWORD hBlank) ; Sets the USB delay factor to a value between 0 and 2047
Q5II_V_DW Q5II_SetHBlank;
//EXPORT void _stdcall SetLineremoval ( DWORD lineremoval ) ; Enable line noise removal algorithm. QHY5-II Only
Q5II_V_DW Q5II_SetLineRemoval;


//EXPORT DWORD _stdcall CancelGuide( DWORD Axis );  Cancel guides on a specific axis

typedef DWORD (CALLBACK* Q5II_GFD)(PUCHAR, DWORD);
//EXPORT DWORD _stdcall getFrameData(PUCHAR _buffer , DWORD size ) ;  Gets the data after a SingleExposure(), 8 bit data
Q5II_GFD Q5II_GetFrameData;

typedef DWORD (CALLBACK* Q5II_GC)(DWORD, DWORD, DWORD);
//EXPORT DWORD _stdcall GuideCommand(DWORD GC, DWORD PulseTimeX, DWORD PulseTimeY) ;
Q5II_GC Q5II_GuideCommand;


Camera_QHY5IIClass::Camera_QHY5IIClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
	Name=_T("QHY 5-II");
	FullSize = wxSize(1280,1024);
	HasGuiderOutput = true;
	HasGainControl = true;
}



bool Camera_QHY5IIClass::Connect() {
// returns true on error

	if (Name=="QHY 5L-II") 
		CameraDLL = LoadLibrary(TEXT("qhy5LIIdll"));
	else
		CameraDLL = LoadLibrary(TEXT("qhy5IIdll"));

	if (CameraDLL == NULL) {
		wxMessageBox(_T("Cannot load qhy5IIdll.dll"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_OpenUSB = (Q5II_DW_V)GetProcAddress(CameraDLL,"openUSB");
	if (!Q5II_OpenUSB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have openUSB"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_IsExposing = (Q5II_DW_V)GetProcAddress(CameraDLL,"isExposing");
	if (!Q5II_IsExposing) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have isExposing"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_CancelExposure = (Q5II_V_V)GetProcAddress(CameraDLL,"CancelExposure");
	if (!Q5II_CancelExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have CancelExposure"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_CloseUSB = (Q5II_V_V)GetProcAddress(CameraDLL,"closeUSB");
	if (!Q5II_CloseUSB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have closeUSB"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_StopCapturing = (Q5II_V_V)GetProcAddress(CameraDLL,"StopCapturing");
	if (!Q5II_StopCapturing) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have StopCapturing"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_SingleExposure = (Q5II_V_V)GetProcAddress(CameraDLL,"SingleExposure");
	if (!Q5II_SingleExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SingleExposure"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	/*Q5II_SetAutoBlackLevel = (Q5II_V_V)GetProcAddress(CameraDLL,"SetAutoBlackLevel");
	if (!Q5II_SetAutoBlackLevel) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetAutoBlackLevel"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}*/
	Q5II_SetBlackLevel = (Q5II_V_DW)GetProcAddress(CameraDLL,"SetBlackLevel");
	if (!Q5II_SetBlackLevel) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetBlackLEvel"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_SetGain = (Q5II_V_DW)GetProcAddress(CameraDLL,"SetGain");
	if (!Q5II_SetGain) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetGain"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_SetExposureTime = (Q5II_V_DW)GetProcAddress(CameraDLL,"SetExposureTime");
	if (!Q5II_SetExposureTime) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetExposureTime"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_SetSpeed = (Q5II_V_DW)GetProcAddress(CameraDLL,"SetSpeed");
	if (!Q5II_SetSpeed) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetSpeed"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_SetHBlank = (Q5II_V_DW)GetProcAddress(CameraDLL,"SetHBlank");
	if (!Q5II_SetHBlank) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have SetHBlank"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_GetFrameData = (Q5II_GFD)GetProcAddress(CameraDLL,"getFrameData");
	if (!Q5II_GetFrameData) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have getFrameData"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q5II_GuideCommand = (Q5II_GC)GetProcAddress(CameraDLL,"GuideCommand");
	if (!Q5II_GuideCommand) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy5IIdll.dll does not have GuideCommand"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	if (!Q5II_OpenUSB()) {
		wxMessageBox(_T("No camera"));
		return true;
	}

	if (RawBuffer) 
		delete [] RawBuffer;
	RawBuffer = new unsigned char[1280*1024];

	//Q5II_SetAutoBlackLevel();
	Q5II_SetBlackLevel(1);
	Q5II_SetSpeed(0);
	Connected = true;
	return false;
}

bool Camera_QHY5IIClass::PulseGuideScope(int direction, int duration) {
	DWORD reg = 0;
	DWORD dur = (DWORD) duration / 10;
	DWORD ptx, pty;
	
	//if (dur >= 255) dur = 254; // Max guide pulse is 2.54s -- 255 keeps it on always
	switch (direction) {
		case WEST: reg = 0x80; ptx = dur; pty = 0xFFFFFFFF; break;	
		case NORTH: reg = 0x20; pty = dur; ptx = 0xFFFFFFFF; break;	
		case SOUTH: reg = 0x40; pty = dur; ptx = 0xFFFFFFFF; break;	
		case EAST: reg = 0x10;	ptx = dur; pty = 0xFFFFFFFF; break;	
		default: return true; // bad direction passed in
	}
	Q5II_GuideCommand(reg,ptx,pty);
	wxMilliSleep(duration + 10);
	return false;
}

void Camera_QHY5IIClass::ClearGuidePort() {
	//Q5II_CancelGuide(3); // 3 clears on both axes
}

void Camera_QHY5IIClass::InitCapture() {
	Q5II_SetGain(GuideCameraGain);
}

bool Camera_QHY5IIClass::Disconnect() {
	Q5II_CloseUSB();
	Connected = false;
	if (RawBuffer)
		delete [] RawBuffer;
	RawBuffer = NULL;
	FreeLibrary(CameraDLL);

	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;

}

bool Camera_QHY5IIClass::CaptureFull(int duration, usImage& img, bool recon) {
// Only does full frames still
	static int last_dur = 0;
	static int last_gain = 60;
	unsigned char *bptr;
	unsigned short *dptr;
	int  x,y;
	int xsize = FullSize.GetWidth();
	int ysize = FullSize.GetHeight();
//	bool firstimg = true;

	if (img.NPixels != (xsize*ysize)) {
		if (img.Init(xsize,ysize)) {
 			wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
			Disconnect();
			return true;
		}
	}
	if (duration != last_dur) {
		Q5II_SetExposureTime(duration);
		last_dur = duration;
	}
	else if (GuideCameraGain != last_gain) {
		Q5II_SetGain(GuideCameraGain);
		last_gain = GuideCameraGain;
	}

	Q5II_SingleExposure();
	wxMilliSleep(duration);
	//int foo = 0;
	while (Q5II_IsExposing()) {
		//frame->SetStatusText(wxString::Format("%d",foo));
		wxMilliSleep(100);
		//foo++;
	}

	Q5II_GetFrameData(RawBuffer,xsize*ysize);

	bptr = RawBuffer;
	// Load and crop from the 800 x 525 image that came in
	dptr = img.ImageData;
	for (y=0; y<ysize; y++) {
		for (x=0; x<xsize; x++, bptr++, dptr++) { // CAN SPEED THIS UP
			*dptr=(unsigned short) *bptr;
		}
	}

	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);

	return false;
}

/*bool Camera_QHY5IIClass::CaptureCrop(int duration, usImage& img) {
	GenericCapture(duration, img, CROPXSIZE,CROPYSIZE,CropX,CropY);

return false;
}

bool Camera_QHY5IIClass::CaptureFull(int duration, usImage& img) {
	GenericCapture(duration, img, FullSize.GetWidth(),FullSize.GetHeight(),0,0);

	return false;
}
*/

#endif