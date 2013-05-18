/*
 *  cam_SXV.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
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
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <wx/choicdlg.h>

#if defined (SXV)
#include "cam_SXV.h"
extern Camera_SXVClass Camera_SXV;

Camera_SXVClass::Camera_SXVClass() {
	Connected = false;
	//	HaveBPMap = false;
	//	NBadPixels=-1;
	Name=_T("Starlight Xpress SXV");
	FullSize = wxSize(1280,1024);
	HasGainControl = false;
	HasGuiderOutput = true;
	Interlaced = false;
	RawData = NULL;
}

#if defined (__APPLE__)

int SXCamAttached (void *cam) {
	// This should return 1 if the cam passed in here is considered opened, 0 otherwise
	//wxMessageBox(wxString::Format("Found SX cam model %d", (int) sxGetCameraModel(cam)));
	//SXVCamera.hCam = cam;
	return 0;
}

void SXCamRemoved (void *cam) {
	//	CameraPresent = false;
	//SXVCamera.hCam = NULL;
}

#endif


bool Camera_SXVClass::Connect() {
	// returns true on error
	
	bool retval = true;
#if defined (__WINDOWS__)
	HANDLE hCams[SXCCD_MAX_CAMS];
	int ncams = sxOpen(hCams);
	if (ncams == 0) return true;  // No cameras
//	wxMessageBox(wxString::Format("%d",ncams));
//	return true;
	// Dialog to choose which Cam if # > 1  (note 0-indexed)
	if (ncams > 1) {
		wxString tmp_name;
		wxArrayString Names;
		int i;
		unsigned short model;
		for (i=0; i<ncams; i++) {
			model = sxGetCameraModel(hCams[i]);
			tmp_name=wxString::Format("SXV-%c%d%c",model & 0x40 ? 'M' : 'H', model & 0x1F, model & 0x80 ? 'C' : '\0');
			if (model == 70)
				tmp_name = _T("SXV-Lodestar");
			Names.Add(tmp_name);
		}
		i=wxGetSingleChoiceIndex(_T("Camera choice"),_T("Available cameras"),Names);
		if (i == -1) return true;
		hCam = hCams[i];
	}
	else
		hCam=hCams[0];
#else  // Mac connect code
	hCam = NULL;

	/*
	// Orig version
	static bool ProbeLoaded = false;
	if (!ProbeLoaded) 
		sxProbe (SXCamAttached,SXCamRemoved);
	ProbeLoaded = true;
	int i, model;
	wxString tmp_name;
	wxArrayString Names;
	int ncams = 0;
	int portstatus;
	portstatus = sxCamPortStatus(0);
	portstatus = sxCamPortStatus(1);
	
	for (i=0; i<4; i++) {
		model = sxCamAvailable(i);
		if (model) {
			ncams++;
			tmp_name=wxString::Format("%d: SXV-%c%d%c",i,model & 0x40 ? 'M' : 'H', model & 0x1F, model & 0x80 ? 'C' : '\0');
			if (model == 70)
				tmp_name = wxString::Format("%d: SXV-Lodestar",i);
			Names.Add(tmp_name);			
		}
	}
	if (ncams > 1) {
		wxString ChoiceString;
		ChoiceString=wxGetSingleChoice(_T("Camera choice"),_T("Available cameras"),Names);
		if (ChoiceString.IsEmpty()) return true;
		ChoiceString=ChoiceString.Left(1);
		long lval;
		ChoiceString.ToLong(&lval);
		hCam = sxOpen((int) lval);
		sxReleaseOthers((int) lval);
	}
	else
		hCam = sxOpen(-1);
	portstatus = sxCamPortStatus(0);
	portstatus = sxCamPortStatus(1);
	*/
	
	
	
	 // New version
	int ncams = sx2EnumDevices();
	if (!ncams) {
		wxMessageBox(_T("No SX cameras found"));
		return true;
	}
	if (ncams > 1) {
		int i, model;
		wxString tmp_name;
		wxArrayString Names;
		void      *htmpCam;
		char devname[32];
		for (i=0; i<ncams; i++) {
	/*		htmpCam = sx2Open(i);
			if (htmpCam) {
				model = sxGetCameraModel(htmpCam);
				tmp_name = wxString::Format("%d: %d",i+1,model);
				Names.Add(tmp_name);
				wxMilliSleep(500);
				sx2Close(htmpCam);
				wxMilliSleep(500);
				htmpCam = NULL;
			}*/
			model = (int) sx2GetID(i);
			if (model) {
				sx2GetName(i,devname);
				tmp_name = wxString::Format("%d: %s",i+1,devname);
				Names.Add(tmp_name);				
			}
		}
		
		wxString ChoiceString;
		ChoiceString=wxGetSingleChoice(_T("Camera choice"),_T("Available cameras"),Names);
		if (ChoiceString.IsEmpty()) return true;
		ChoiceString=ChoiceString.Left(1);
		long lval;
		ChoiceString.ToLong(&lval);
		lval = lval - 1;
		hCam = sx2Open((int) lval);
	}		
	else 
		hCam = sx2Open(0);
	 

	

	if (hCam == NULL) return true;
#endif
	
	retval = false;  // assume all good
		
	// Close all unused cameras if nCams > 1 after picking which one
	//  WRITE
	
	// Load parameters
	sxGetCameraParams(hCam,0,&CCDParams);
	FullSize = wxSize(CCDParams.width, CCDParams.height);
//	PixelSize[0] = CCDParams.pix_width;
//	PixelSize[1] = CCDParams.pix_height;
	
	// deal with what if no porch in there ??
	// WRITE??
	
	CameraModel = sxGetCameraModel(hCam);
	Name=wxString::Format("SXV-%c%d%c",CameraModel & 0x40 ? 'M' : 'H', CameraModel & 0x1F, CameraModel & 0x80 ? 'C' : '\0');
	if (CameraModel == 70)
		Name = _T("SXV-Lodestar");
	SubType = CameraModel & 0x1F;
	if (CameraModel == 39)
		Name = _T("CMOS Guider");
	if (CameraModel & 0x80)  // color
		ColorSensor = true;
	else
		ColorSensor = false;
		
	if (CameraModel & 0x40)
		Interlaced = true;
	else
		Interlaced = false;
	if (SubType == 25)
		Interlaced = false;
		
	if ( Interlaced ) {
		FullSize.SetHeight(FullSize.GetHeight() * 2);  // one of the interlaced CCDs that reports the size of a field
//		PixelSize[1] = PixelSize[1] / 2.0;
	}
	RawData = new unsigned short[FullSize.GetHeight() * FullSize.GetWidth()];
	return retval;
}

bool Camera_SXVClass::Disconnect() {
	if (RawData)
		delete [] RawData;
	RawData = NULL;
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	sxReset(hCam);
#ifdef __APPLE__
	sx2Close(hCam);
#else
	sxClose(hCam);
#endif
	hCam = NULL;
	
	return false;
}
void Camera_SXVClass::InitCapture() {

	
}
bool Camera_SXVClass::CaptureFull(int duration, usImage& img, bool recon) {
	bool UseInternalTimer = false;
	unsigned int i;
	//	unsigned short ExpFlags;
	unsigned short xsize, ysize;
	bool retval = false;
	unsigned short *dataptr;
	unsigned short *rawptr;

	// Interlaced cams will be run in "high speed" mode (vertically binned) to avoid all sorts of crap
	
	if (Interlaced) ysize = FullSize.GetHeight() / 2;
	else ysize = FullSize.GetHeight();
	xsize = FullSize.GetWidth();
	if (CameraModel == 39)
		UseInternalTimer = true;

	wxStopWatch swatch;
	// Do exposure
	if (UseInternalTimer) {
		sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);
		sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_ODD, 0, 0,0 , xsize, ysize, 1,1, duration);
	}
	else {
		sxClearPixels(hCam,0,0);
		swatch.Start();
		if (duration > 100) {
			wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
			wxTheApp->Yield();
		}
		while (swatch.Time() < duration) {
			wxMilliSleep(10);
		}
		sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, 0,0 , xsize, ysize, 1,1);
	}

	int NPixelsToRead = xsize * ysize;

	if (Interlaced)
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData, NPixelsToRead);  // stop exposure and read but only the one frame
#else
		sxReadPixels(hCam, (UInt8 *) RawData, NPixelsToRead, sizeof(unsigned short));  // stop exposure and read but only the one frame
#endif
	else // Progressive read
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData, NPixelsToRead);
#else
		sxReadPixels(hCam, (UInt8 *) RawData, NPixelsToRead, sizeof (unsigned short));
#endif
	
	
	// Re-assemble image
	int output_xsize = FullSize.GetWidth();
	int output_ysize = FullSize.GetHeight();
	if (CameraModel == 39)  // the CMOS guider
		output_xsize = output_xsize - 16;  // crop off 16 from one side
	if (img.NPixels != (output_xsize*output_ysize)) {
		if (img.Init(output_xsize,output_ysize)) {
			wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
			Disconnect();
			return true;
		}
	}		
	
	rawptr = RawData;
	dataptr = img.ImageData;
	if (CameraModel == 39) { // CMOS guider -- crop and clean
		int x, y, val;
		int oddbias, evenbias;

		for (y=0; y<output_ysize; y++) {
			oddbias = evenbias = 0;
			for (x=0; x<16; x+=2) { // Figure the offsets for this line
				oddbias += (int) *rawptr++;
				evenbias += (int) *rawptr++;
			}
			oddbias = oddbias / 8 - 1000;  // Create avg and pre-build in the offset to keep off of the floor
			evenbias = evenbias / 8 - 1000;
			for (x=0; x<output_xsize; x+=2) { // Load value into new image array pulling out right bias
				val = (int) *rawptr++ - oddbias;
				if (val < 0.0) val = 0.0;  //Bounds check
				else if (val > 65535.0) val = 65535.0;
				*dataptr++ = (unsigned short) val;
				val = (int) *rawptr++ - evenbias;
				if (val < 0.0) val = 0.0;  //Bounds check
				else if (val > 65535.0) val = 65535.0;
				*dataptr++ = (unsigned short) val;
			}
		}
	}
	else if (Interlaced) {  // recon 1x2 bin into full-frame
		unsigned int x,y;
		for (y=0; y<ysize; y++) {  // load into image w/skips
			for (x=0; x<xsize; x++, rawptr++, dataptr++) {
				*dataptr = (*rawptr);
			}
			dataptr += xsize;
		}
		// interpolate
		dataptr = img.ImageData + xsize;
		for (y=0; y<(ysize - 1); y++) {
			for (x=0; x<xsize; x++, dataptr++)
				*dataptr = ( *(dataptr - xsize) + *(dataptr + xsize) ) / 2;
			dataptr += xsize;
		}
		for (x=0; x<xsize; x++, dataptr++)
			*dataptr =  *(dataptr - xsize);
		
	}
	else {  // Progressive
		for (i=0; i<img.NPixels; i++, rawptr++, dataptr++) {
			*dataptr = (*rawptr);
		}
	}

	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);
	if (recon) QuickLRecon(img);  // pass 2x2 mean filter over it to help remove noise
	
	if (CCDParams.pix_width != CCDParams.pix_height)
		SquarePixels(img,CCDParams.pix_width,CCDParams.pix_width);

	return false;
}


bool Camera_SXVClass::PulseGuideScope(int direction, int duration) {
	// Guide port values
	// West = 1
	// East = 8
	// North = 2
	// South = 4
	unsigned char dircmd = 0;
	switch (direction) {
		case WEST: 
			dircmd = (unsigned char) 1;
			break;
		case EAST:
			dircmd = (unsigned char) 8;
			break;
		case NORTH:
			dircmd = (unsigned char) 2;
			break;
		case SOUTH:
			dircmd = (unsigned char) 4;
			break;
	}
	sxSetSTAR2000(hCam,dircmd);
	wxMilliSleep(duration);
	dircmd = 0;
	sxSetSTAR2000(hCam,dircmd);
	
	return false;
}












#endif // SXV
