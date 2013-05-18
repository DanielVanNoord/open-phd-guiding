/*
 *  target.cpp
 *  PHD Guiding
 *
 *  Created by Sylvain Girard
 *  Copyright (c) 2013 Sylvain Girard
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
 *    Neither the name of Bret McKee, Dad Dog Development Ltd, nor the names of its
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
 *
 */

#include "phd.h"

BEGIN_EVENT_TABLE(TargetWindow, wxWindow)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH,TargetWindow::OnButtonLength)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR,TargetWindow::OnButtonClear)
    EVT_BUTTON(BUTTON_GRAPH_ZOOMIN,TargetWindow::OnButtonZoomIn)
    EVT_BUTTON(BUTTON_GRAPH_ZOOMOUT,TargetWindow::OnButtonZoomOut)
END_EVENT_TABLE()

TargetWindow::TargetWindow(wxWindow *parent) :
    wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize, 0,_("Target"))
{
    //SetFont(wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
    SetBackgroundColour(*wxBLACK);

    m_visible = false;
    m_pClient = new TargetClient(this);

    wxBoxSizer *pMainSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pLeftSizer = new wxBoxSizer(wxVERTICAL);

    pMainSizer->Add(pLeftSizer);

    LengthButton = new wxButton(this,BUTTON_GRAPH_LENGTH,_T("100"),wxDefaultPosition,wxSize(80,-1));
    LengthButton->SetToolTip(_("# of frames of history to display"));

    wxBoxSizer *pZoomSizer = new wxBoxSizer(wxHORIZONTAL);

    ZoomInButton = new wxButton(this,BUTTON_GRAPH_ZOOMIN,_T("+"),wxDefaultPosition,wxSize(40,-1));
    ZoomInButton->SetToolTip(_("Increase vertical scale"));

    ZoomOutButton = new wxButton(this,BUTTON_GRAPH_ZOOMOUT,_T("-"),wxDefaultPosition,wxSize(40,-1));
    ZoomOutButton->SetToolTip(_("Deccrease vertical scale"));

    pZoomSizer->Add(ZoomInButton);
    pZoomSizer->Add(ZoomOutButton);

    ClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_("Clear"),wxDefaultPosition,wxSize(80,-1));
    ClearButton->SetToolTip(_("Clear graph data"));

    pLeftSizer->Add(LengthButton, wxSizerFlags().Center().Border(wxTOP | wxRIGHT | wxLEFT,5));
    pLeftSizer->Add(pZoomSizer, wxSizerFlags().Center().Border(wxRIGHT | wxLEFT,5));
    pLeftSizer->Add(ClearButton, wxSizerFlags().Center().Border(wxRIGHT | wxLEFT,5));

    pMainSizer->Add(m_pClient, wxSizerFlags().Border(wxALL,3).Expand().Proportion(1));

    SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(this);
}

TargetWindow::~TargetWindow()
{
    delete m_pClient;
}

void TargetWindow::SetState(bool is_active)
{
    m_visible = is_active;
    if (is_active)
        Refresh();
}

void TargetWindow::AppendData(double ra, double dec)
{
    m_pClient->AppendData(ra, dec);

    if (this->m_visible)
    {
        Refresh();
    }
}

void TargetWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_length *= 2;

    if (m_pClient->m_length > m_pClient->m_maxLength)
    {
            m_pClient->m_length = m_pClient->m_minLength;
    }

    LengthButton->SetLabel(wxString::Format(_T("%3d"),m_pClient->m_length));
    Refresh();
}

void TargetWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_nItems = 0;
    Refresh();
}

void TargetWindow::OnButtonZoomIn(wxCommandEvent& evt)
{
    if (m_pClient->m_zoom < 3) m_pClient->m_zoom *= 2;
    Refresh();
}

void TargetWindow::OnButtonZoomOut(wxCommandEvent& evt)
{
    if (m_pClient->m_zoom > 0.25) m_pClient->m_zoom /= 2;
    Refresh();
}

BEGIN_EVENT_TABLE(TargetClient, wxWindow)
    EVT_PAINT(TargetClient::OnPaint)
END_EVENT_TABLE()

TargetClient::TargetClient(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(201,201), wxFULL_REPAINT_ON_RESIZE )
{
    m_minLength = 50;
    m_maxLength = 400;

    m_nItems = 0;
    m_length = 100;
    m_zoom = 1;
}

TargetClient::~TargetClient(void)
{
}

void TargetClient::AppendData(double ra, double dec)
{
    memmove(&m_history, &m_history[1], sizeof(m_history[0])*(m_maxHistorySize-1));

    m_history[m_maxHistorySize-1].ra = ra;
    m_history[m_maxHistorySize-1].dec = dec;

    if (m_nItems < m_maxHistorySize)
    {
        m_nItems++;
    }
}

void TargetClient::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    double sampling = pFrame->GetSampling();

    wxColour Grey(128,128,128);
    wxPen GreySolidPen = wxPen(Grey,1, wxSOLID);
    wxPen GreyDashPen = wxPen(Grey,1, wxDOT);

    dc.SetTextForeground(wxColour(200,200,200));
    dc.SetFont(wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));

    wxSize size = GetClientSize();
    wxPoint center(size.x/2, size.y/2);
    int radius_max = ((size.x < size.y ? size.x : size.y) - 6) / 2;

    int leftEdge     = center.x - radius_max;
    int rightEdge    = center.x + radius_max;

    int topEdge      = center.y - radius_max;
    int bottomEdge   = center.y + radius_max;

    // Draw axes
    dc.SetPen(GreySolidPen);
    dc.SetBrush(* wxTRANSPARENT_BRUSH);

    dc.DrawLine(leftEdge, center.y , rightEdge, center.y);
    dc.DrawLine(center.x, topEdge, center.x, bottomEdge);

    // Draw circles
    wxString l;
    wxSize sl;
    radius_max -= 18;

    dc.DrawCircle(center, radius_max);
    l = wxString::Format(_T("%g%s"), 2.0 / m_zoom, sampling != 1 ? "''" : "");
    sl = dc.GetTextExtent(l);
    dc.DrawText(l, center.x - radius_max - sl.GetX(), center.y);

    double r = radius_max / (2 / m_zoom);
    dc.DrawCircle(center, r);
    l = wxString::Format(_T("1%s"), sampling != 1 ? "''" : "");
    sl = dc.GetTextExtent(l);
    dc.DrawText(l, center.x - r - sl.GetX(), center.y);

    // Draw labels
    dc.DrawText(_("RA"), leftEdge, center.y - 13);
    dc.DrawText(_("Dec"), center.x + 2, topEdge - 3);

    // Draw impacts
    double scale = radius_max / 2 * sampling;
    int startPoint = m_maxHistorySize - m_length;

    if (m_nItems < m_length)
    {
        startPoint = m_maxHistorySize - m_nItems;
    }

    int dotSize = 1;

    if (startPoint == m_maxHistorySize)
    {
        dc.DrawCircle(center, dotSize);
    }

    dc.SetPen(wxPen(wxColour(127,127,255),1, wxSOLID));
    for(int i=startPoint; i<m_maxHistorySize;i++)
    {
        int ximpact = center.x + m_history[i].ra * scale * m_zoom;
        int yimpact = center.y + m_history[i].dec * scale * m_zoom;
        if (i == m_maxHistorySize-1)
        {
            int lcrux = 4;
            dc.SetPen(*wxRED_PEN);
            dc.DrawLine(ximpact + lcrux, yimpact + lcrux, ximpact - lcrux - 1, yimpact - lcrux - 1);
            dc.DrawLine(ximpact + lcrux, yimpact - lcrux, ximpact - lcrux - 1, yimpact + lcrux + 1);
            //dc.DrawLine(ximpact, yimpact + lcrux, ximpact, yimpact - lcrux);
            //dc.DrawLine(ximpact + lcrux, yimpact, ximpact - lcrux, yimpact);
        }
        else
            dc.DrawCircle(ximpact, yimpact, dotSize);
    }
}
