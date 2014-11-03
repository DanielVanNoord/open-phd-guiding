/*
 *  config_INDI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
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
#include "scope.h"

#include "config_INDI.h"

#if defined (INDI_CAMERA) || defined (GUIDE_INDI)

#if defined (INDI_CAMERA)
#include "cam_INDI.h"
#endif

//#if defined (GUIDE_INDI)
//	#include "scope_INDI.h"
//#endif

#include "libindiclient/indi.h"
#include "libindiclient/indigui.h"

#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

struct indi_t *INDIClient = NULL;
long INDIport = 7624;
wxString INDIhost = _T("localhost");
wxString INDICameraName;
wxString INDIMountName;

#define MCONNECT 101

#define POS(r, c)        wxGBPosition(r,c)
#define SPAN(r, c)       wxGBSpan(r,c)
static void config_new_device_cb(struct indi_device_t * /*idev */, void *data);

class INDIConfig : public wxDialog
{
public:
    INDIConfig(wxWindow *parent);
    void SaveSettings();
    void UpdateDevices();
    void ConnectServer();
private:
    void FillDevices(wxComboBox *combo, wxString str);
    void OnButton(wxCommandEvent& evt);
    wxTextCtrl *host;
    wxTextCtrl *port;
    wxStaticText *connect_status;
#if defined (INDI_CAMERA)
    wxComboBox *cam;
    wxTextCtrl *camport;
#endif
#if defined (GUIDE_INDI)
    wxComboBox *mount;
    wxTextCtrl *mountport;
#endif
    DECLARE_EVENT_TABLE()
};

void INDIConfig::FillDevices(wxComboBox *combo, wxString str)
{
    indi_list *isl;

    combo->Clear();
    combo->Append(str);
    if (INDIClient) {
        for (isl = il_iter(INDIClient->devices); ! il_is_last(isl); isl = il_next(isl)) {
            struct indi_device_t *idev = (struct indi_device_t *)il_item(isl);
            wxString name = wxString::FromAscii(idev->name);
            if (name != str)
                combo->Append(wxString::FromAscii(idev->name));
        }
    }
    combo->SetSelection(0);
}

void INDIConfig::UpdateDevices()
{
#if defined (INDI_CAMERA)
	FillDevices(cam, pConfig->Profile.GetString("/indi/INDIcam", _T("")));
#endif
#if defined (GUIDE_INDI)
	FillDevices(mount, pConfig->Profile.GetString("/indi/INDImount", _T("")));
#endif
}

INDIConfig::INDIConfig(wxWindow *parent) : wxDialog(parent, wxID_ANY, (const wxString)_T("INDI Configuration"))
{
    int pos;
    wxGridBagSizer *gbs = new wxGridBagSizer(0, 20);
    wxBoxSizer *sizer;

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Hostname:")),
             POS(0, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    host = new wxTextCtrl(this, wxID_ANY, pConfig->Profile.GetString("/indi/INDIhost",_T("localhost")));
    gbs->Add(host, POS(0, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Port:")),
             POS(1, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    port = new wxTextCtrl(this, wxID_ANY, pConfig->Profile.GetString("/indi/INDIport", _T("7624")));
    gbs->Add(port, POS(1, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
    connect_status = new wxStaticText(this, wxID_ANY, _T("Disconnected"));
    gbs->Add(connect_status,POS(2, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    gbs->Add(new wxButton(this, MCONNECT, _T("Connect")), POS(2, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
    pos = 3;
#if defined (INDI_CAMERA)
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Camera Driver:")),
             POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    cam =  new wxComboBox(this, wxID_ANY, _T(""));
    gbs->Add(cam, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Camera Port:")),
             POS(pos + 1, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    camport = new wxTextCtrl(this, wxID_ANY, pConfig->Profile.GetString("/indi/INDIcam_port", _T("")));
    gbs->Add(camport, POS(pos+ 1, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
    pos += 2;
#endif

#if defined (GUIDE_INDI)
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Telescope Driver:")),
             POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    mount =  new wxComboBox(this, wxID_ANY, _T(""));
    gbs->Add(mount, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Telescope Port:")),
             POS(pos + 1, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    mountport = new wxTextCtrl(this, wxID_ANY, pConfig->Profile.GetString("/indi/INDImount_port", _T("")));
    gbs->Add(mountport, POS(pos + 1, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
#endif
    UpdateDevices();

    sizer = new wxBoxSizer(wxVERTICAL) ;
    sizer->Add(gbs);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
    SetSizer(sizer);
    sizer->SetSizeHints(this) ;
    sizer->Fit(this) ;
}
BEGIN_EVENT_TABLE(INDIConfig, wxDialog)
EVT_BUTTON(MCONNECT, INDIConfig::OnButton)
END_EVENT_TABLE()

void INDIConfig::OnButton(wxCommandEvent& WXUNUSED(event)) {
    ConnectServer();
}

void INDIConfig::ConnectServer()
{
    if (INDIClient) {
	if (INDIClient->ClientCount > 0) {
		connect_status->SetLabel(_T("Connected"));
		return;
	}
	indi_remove_cb(INDIClient, (IndiPropCB)config_new_device_cb);
	indi_close(INDIClient);
	delete INDIClient;
	INDIClient = 0;
    } 
    if (! INDIClient) {
	INDIClient = indi_init(host->GetLineText(0).mb_str(wxConvUTF8).data(), atol(port->GetLineText(0).mb_str(wxConvUTF8).data()), "PHDGuiding");
    }
    if (INDIClient) {
	indi_new_device_cb(INDIClient, (IndiPropCB)config_new_device_cb, this);
	connect_status->SetLabel(_T("Connected"));
    }
}

void INDIConfig::SaveSettings()
{
    INDIhost = host->GetLineText(0);
    pConfig->Profile.SetString("/indi/INDIhost", INDIhost);

    port->GetLineText(0).ToLong(&INDIport);
    pConfig->Profile.SetLong("/indi/INDIport", INDIport);

#if defined (INDI_CAMERA)
    INDICameraName = cam->GetValue();
    pConfig->Profile.SetString("/indi/INDIcam", INDICameraName);

/*	long tempPort;
    camport->GetValue().ToLong(&tempPort);

    config->Write(_T("INDIcam_port"), (short) tempPort);*/
    pConfig->Profile.SetString("/indi/INDIcam_port",camport->GetLineText(0));
#endif

#if defined (GUIDE_INDI)
    INDIMountName = mount->GetValue();
    pConfig->Profile.SetString("/indi/INDImount", INDIMountName);

    pConfig->Profile.SetString("/indi/INDImount_port", mountport->GetLineText(0) );
#endif
}

void MyFrame::OnINDIConfig(wxCommandEvent& WXUNUSED(event))
{
   INDI_Setup();
}

void MyFrame::OnINDIDialog(wxCommandEvent& WXUNUSED(event))
{
   INDI_Dialog();
}

void config_new_device_cb(struct indi_device_t * /*idev */, void *data)
{
    INDIConfig *indi_config = (INDIConfig *)data;
    indi_config->UpdateDevices();
}

void INDI_Setup()
{
    INDIConfig *indiDlg = new INDIConfig(wxGetActiveWindow());
    indiDlg->ConnectServer();
    if (indiDlg->ShowModal() == wxID_OK) {
    	indiDlg->SaveSettings();
    }
    indiDlg->Destroy();
    delete indiDlg;
}

void INDI_Dialog()
{
    if (! INDIClient) INDI_Setup();
    if (INDIClient) 
    {
       indigui_show_dialog(INDIClient);
    }
}

#endif
