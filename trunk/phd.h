/*
 *  phd.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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

 //#define ORION


#include <wx/wx.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/html/helpctrl.h>
#include <wx/utils.h>
#include <wx/textfile.h>
#include <wx/socket.h>
#include "usImage.h"
#include "graph.h"
#include "camera.h"
#include "scope.h"
#define VERSION _T("1.12.4")

#if defined (__WINDOWS__)
#pragma warning(disable:4189)
#pragma warning(disable:4018)
#pragma warning(disable:4305)
#pragma warning(disable:4100)
#pragma warning(disable:4996)
#endif

WX_DEFINE_ARRAY_INT(int, ArrayOfInts);
WX_DEFINE_ARRAY_DOUBLE(double, ArrayOfDbl);


#if defined (__WINDOWS__)
#define PATHSEPCH '\\'
#define PATHSEPSTR "\\"
#endif

#if defined (__APPLE__)
#define PATHSEPCH '/'
#define PATHSEPSTR "/"
#endif

#if defined (__WXGTK__)
#define PATHSEPCH '/'
#define PATHSEPSTR _T("/")
#endif

#if !defined (PI)
#define PI 3.1415926
#endif


#define CROPXSIZE 100
#define CROPYSIZE 100


#define ROUND(x) (int) floor(x + 0.5)


class MyApp: public wxApp
{
  public:
    MyApp(void){};
    bool OnInit(void);
};

// Canvas area for image -- can take events
class MyCanvas: public wxWindow {
  public:
	int State;  // see STATE enum
	wxImage	*Displayed_Image;
	double	ScaleFactor;
	bool binned;

   MyCanvas(wxWindow *parent);
    ~MyCanvas(void);

   void OnPaint(wxPaintEvent& evt);
	void FullFrameToDisplay();
  private:
	void OnLClick(wxMouseEvent& evt);
	void OnErase(wxEraseEvent& evt);
	DECLARE_EVENT_TABLE()
};

class MyFrame: public wxFrame {
public:
	MyFrame(const wxString& title);
	virtual ~MyFrame();

	MyCanvas *canvas;
	wxMenuBar *Menubar;
	wxMenu	*tools_menu, *mount_menu; // need access to this...
	wxChoice	*Dur_Choice;
	wxCheckBox *HotPixel_Checkbox;
	wxButton	*Setup_Button, *Dark_Button;
	wxBitmapButton *Brain_Button, *Cam_Button, *Scope_Button, *Loop_Button, *Guide_Button, *Stop_Button;
	wxHtmlHelpController *help;
	wxSlider *Gamma_Slider;
	GraphLogWindow *GraphLog;
	ProfileWindow *Profile;

	void OnQuit(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnAbout(wxCommandEvent& evt);
	void OnHelp(wxCommandEvent& evt);
	void OnOverlay(wxCommandEvent& evt);
	void OnInstructions(wxCommandEvent& evt);
	void OnSave(wxCommandEvent& evt);
	void OnLog(wxCommandEvent& evt);
	void OnConnectScope(wxCommandEvent& evt);
	void OnConnectCamera(wxCommandEvent& evt);
	void OnLoopExposure(wxCommandEvent& evt);
	void OnButtonStop(wxCommandEvent& evt);
	void OnDark(wxCommandEvent& evt);
	void OnClearDark(wxCommandEvent& evt);
	void OnGuide(wxCommandEvent& evt);
	void OnAdvanced(wxCommandEvent& evt);
	void OnIdle(wxIdleEvent& evt);
	void OnTestGuide(wxCommandEvent& evt);
	void OnEEGG(wxCommandEvent& evt);
	void OnSetupCamera(wxCommandEvent& evt);
	void OnGammaSlider(wxScrollEvent& evt);
	void OnServerEvent(wxSocketEvent& evt);
	void OnSocketEvent(wxSocketEvent& evt);
	void OnServerMenu(wxCommandEvent& evt);
#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
	void OnINDIConfig(wxCommandEvent& evt);
	void OnINDIDialog(wxCommandEvent& evt);
#endif

#if defined (V4L_CAMERA)
	 void OnSaveSettings(wxCommandEvent& evt);
	 void OnRestoreSettings(wxCommandEvent& evt);
#endif

	bool StartServer(bool state);
	void OnGraph(wxCommandEvent& evt);
	void OnStarProfile(wxCommandEvent& evt);
	void OnAutoStar(wxCommandEvent& evt);
	void SetExpDuration();
	void ReadPreferences();
	void WritePreferences();
	bool Voyager_Connect();
	void OnVoyagerEvent(wxSocketEvent& evt);
private:

	DECLARE_EVENT_TABLE()
};


class GuideCamera {
public:
	wxString			Name;					// User-friendly name
	wxSize			FullSize;			// Size of current image
	bool				Connected;
	bool			HasGuiderOutput;
	bool			HasPropertyDialog;
	bool			HasPortNum;
	bool			HasDelayParam;
	bool			HasGainControl;
	short			Port;
	int				Delay;

	virtual bool	CaptureFull(int WXUNUSED(duration), usImage& WXUNUSED(img), bool WXUNUSED(recon)) { return true; }
	virtual bool	CaptureFull(int duration, usImage& img) { return CaptureFull(duration, img, true); }	// Captures a full-res shot
//	virtual bool	CaptureCrop(int duration, usImage& img) { return true; }	// Captures a 160 x 120 cropped portion
	virtual bool	Connect() { return true; }		// Opens up and connects to camera
	virtual bool	Disconnect() { return true; }	// Disconnects, unloading any DLLs loaded by Connect
	virtual void	InitCapture() { return; }		// Gets run at the start of any loop (e.g., reset stream, set gain, etc).
	virtual bool	PulseGuideScope (int WXUNUSED(direction), int WXUNUSED(duration)) { return true; }
	virtual void	ShowPropertyDialog() { return; }
	GuideCamera() { Connected = FALSE;  Name=_T("");
		HasGuiderOutput = false; HasPropertyDialog = false; HasPortNum = false; HasDelayParam = false;
		HasGainControl = false; }
	~GuideCamera(void) {};


};

enum {
	STATE_NONE = 0,
	STATE_SELECTED,
	STATE_CALIBRATING,
	STATE_GUIDING_LOCKED,
	STATE_GUIDING_LOST
};

enum {
	STAR_OK = 0,
	STAR_SATURATED,
	STAR_LOWSNR,
	STAR_LOWMASS,
	STAR_MASSCHANGE,
	STAR_LARGEMOTION
};

enum {
	MENU_SHOWHELP = 101,
	MOUNT_ASCOM,
	MOUNT_CAMERA,
	MOUNT_GPUSB,
	MOUNT_GPINT3BC,
	MOUNT_GPINT378,
	MOUNT_GPINT278,
	MOUNT_NEB,
	MOUNT_VOYAGER,
	MOUNT_EQUINOX,
	MOUNT_GCUSBST4,
	MOUNT_INDI,
	BUTTON_SCOPE,
	BUTTON_CAMERA,
	BUTTON_CAL,
	BUTTON_DARK,
	BUTTON_LOOP,
	BUTTON_GUIDE,
	BUTTON_STOP,
	BUTTON_DURATION,
	BUTTON_DETAILS,
	CTRL_GAMMA,
	WIN_VFW,  // Dummy event to capture VFW streams
	MGUIDE_N,
	MGUIDE_S,
	MGUIDE_E,
	MGUIDE_W,
	MENU_MANGUIDE,
	MENU_XHAIR0,
	MENU_XHAIR1,
	MENU_XHAIR2,
	MENU_XHAIR3,
	MENU_XHAIR4,
	MENU_XHAIR5,
	MENU_CLEARDARK,
	MENU_LOG,
	MENU_LOGIMAGES,
	MENU_DEBUG,
	MENU_SERVER,
	MENU_GRAPH,
	MENU_STARPROFILE,
	MENU_AUTOSTAR,
	MENU_INDICONFIG,
	MENU_INDIDIALOG,
	MENU_V4LSAVESETTINGS,
	MENU_V4LRESTORESETTINGS,
	BUTTON_GRAPH_LENGTH,
	BUTTON_GRAPH_MODE,
	BUTTON_GRAPH_HIDE,
	BUTTON_GRAPH_CLEAR,
	GRAPH_RAA,
	GRAPH_RAH,
	GRAPH_MM,
	GRAPH_DSW,
	GRAPH_MDD,
	GRAPH_MRAD,
	GRAPH_DM,
//	EEGG_FITSSAVE,
	EEGG_TESTGUIDEDIR,
	EEGG_MANUALCAL,
	EEGG_CLEARCAL,
	EEGG_MANUALLOCK,
	EEGG_FLIPRACAL,
	EEGG_RANDOMMOTION
};

enum {
	NORTH = 0,	// Dec+  RA+(W), Dec+(N), Dec-(S), RA-(E)
	SOUTH,		// Dec-
	EAST,		// RA-
	WEST		// RA+
};

enum {
	DEC_OFF = 0,
	DEC_AUTO,
	DEC_NORTH,
	DEC_SOUTH
};

enum {
	DEC_LOWPASS = 0,
	DEC_RESISTSWITCH,
	DEC_LOWPASS2
};

enum {
	NR_NONE,
	NR_2x2MEAN,
	NR_3x3MEDIAN
};

enum {
	SERVER_ID = 100,
	SOCKET_ID,
	VOYAGER_ID
};

extern MyFrame *frame;
#if defined (__WINDOWS__)
extern IDispatch *ScopeDriverDisplay;
#endif
extern wxString ScopeName;
extern wxTextFile *LogFile;
extern int ScopeConnected;
extern bool ScopeCanPulseGuide;
extern bool CheckPulseGuideMotion;
extern bool GuideCameraConnected;
extern bool Calibrated;	// Do we know cal parameters?
extern bool	CaptureActive; // Is camera looping captures?
extern bool UseSubframes; // Use subframes if possible from camera
extern double StarMass;
extern double StarSNR;
extern double StarMassChangeRejectThreshold;
extern double RA_rate;
extern double RA_angle;	// direction of positive (west) RA engagement
extern double Dec_rate;
extern double Dec_angle;
extern double RA_hysteresis;
extern double Dec_slopeweight;
extern int Max_Dec_Dur;
extern int Max_RA_Dur;
extern double RA_aggr;
extern int	Cal_duration;
extern int Dec_guide;
extern int Dec_algo;
extern int NR_mode;
extern int AdvDlg_fontsize;
extern bool DisableGuideOutput;
extern bool DitherRAOnly;
extern bool Log_Data;
extern bool Log_Images;
extern GuideCamera *CurrentGuideCamera;
extern usImage CurrentFullFrame;
extern usImage CurrentDarkFrame;
extern bool HaveDark;
extern int	DarkDur;
extern int CropX;		// U-left corner of crop position
extern int CropY;
extern double StarX;	// Where the star is in full-res coords
extern double StarY;
extern double LastdX;	// Star movement on last frame
extern double LastdY;
extern double dX;		// Delta between current and locked star position
extern double dY;
extern double LockX;	// Place where we should be locked to -- star's starting point
extern double LockY;
extern bool ManualLock;	// In manual lock position mode?  (If so, don't re-lock on start of guide)
extern double MinMotion; // Minimum star motion to trigger a pulse
extern int SearchRegion; // how far u/d/l/r do we do the initial search
extern bool FoundStar;	// Do we think we have a star?
extern int Abort;		// Flag turned true when Abort button hit.  1=Abort, 2=Abort Loop and start guiding
extern int	ExpDur; // exposure duration
extern int  Time_lapse;		// Delay between frames (useful for vid cameras)
extern int	GuideCameraGain;
extern double Stretch_gamma;
extern int XWinSize;
extern int YWinSize;
extern int OverlayMode;
extern bool Paused;	// has PHD been told to pause guiding?
extern bool ServerMode;
extern bool RandomMotionMode;
extern wxSocketServer *SocketServer;
extern int SocketConnections;
extern double CurrentError;

