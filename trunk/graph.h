/*
 *  graph.h
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

#ifndef GRAPHCLASS
#define GRAPHCLASS

class GraphControlPane;

enum GRAPH_UNITS
{
    UNIT_PIXELS,
    UNIT_ARCSEC,
};

// accumulator for trend line calculation
struct TrendLineAccum
{
    double sum_y;
    double sum_xy;
    double sum_y2;
};

struct S_HISTORY
{
    wxLongLong_t timestamp;
    double dx;
    double dy;
    double ra;
    double dec;
    double raDur;
    double decDur;
    S_HISTORY() { }
    S_HISTORY(const GuideStepInfo& step)
        : timestamp(wxDateTime::UNow().GetValue().GetValue()),
        dx(step.cameraOffset->X), dy(step.cameraOffset->Y), ra(step.mountOffset->X), dec(step.mountOffset->Y),
        raDur(step.durationRA), decDur(step.durationDec) { }
};

class GraphLogClientWindow : public wxWindow
{
    GraphLogClientWindow(wxWindow *parent);
    ~GraphLogClientWindow(void);

    bool SetMinLength(unsigned int minLength);
    bool SetMaxLength(unsigned int maxLength);
    bool SetMinHeight(unsigned int minLength);
    bool SetMaxHeight(unsigned int minHeight);

    wxColour m_raOrDxColor, m_decOrDyColor;
    wxStaticText *m_pRaRMS, *m_pDecRMS, *m_pTotRMS, *m_pOscIndex;

    void AppendData(const GuideStepInfo& step);
    void ResetData(void);
    void RecalculateTrendLines(void);
    void OnPaint(wxPaintEvent& evt);

    unsigned int m_minLength;

    unsigned int m_minHeight;
    unsigned int m_maxHeight;

    static const int m_xSamplesPerDivision = 50;
    static const int m_yDivisions = 3;

    circular_buffer<S_HISTORY> m_history;

    TrendLineAccum m_trendLineAccum[4]; // dx, dy, ra, dec

    int m_raSameSides; // accumulator for RA osc index

    enum GRAPH_MODE
    {
        MODE_RADEC,
        MODE_DXDY,
    } m_mode;

    unsigned int m_length;
    unsigned int m_height;
    GRAPH_UNITS m_heightUnits;

    bool m_showTrendlines;
    bool m_showCorrections;

    friend class GraphLogWindow;

    DECLARE_EVENT_TABLE()
};

class GraphLogWindow : public wxWindow
{
public:
    GraphLogWindow(wxWindow *parent);
    ~GraphLogWindow(void);

    void AppendData(const GuideStepInfo& step);
    void UpdateControls(void);
    void SetState(bool is_active);
    void EnableTrendLines(bool enable);

    void OnPaint(wxPaintEvent& evt);
    void OnButtonSettings(wxCommandEvent& evt);
    void OnRADecDxDy(wxCommandEvent& evt);
    void OnArcsecsPixels(wxCommandEvent& evt);
    void OnRADxColor(wxCommandEvent& evt);
    void OnDecDyColor(wxCommandEvent& evt);
    void OnButtonLength(wxCommandEvent& evt);
    void OnMenuLength(wxCommandEvent& evt);
    void OnButtonHeight(wxCommandEvent& evt);
    void OnMenuHeight(wxCommandEvent& evt);
    void OnButtonClear(wxCommandEvent& evt);
    void OnCheckboxTrendlines(wxCommandEvent& evt);
    void OnCheckboxCorrections(wxCommandEvent& evt);
    void OnButtonZoomIn(wxCommandEvent& evt);
    void OnButtonZoomOut(wxCommandEvent& evt);

    wxStaticText *m_pLabel1, *m_pLabel2;

    wxColor GetRaOrDxColor(void);
    wxColor GetDecOrDyColor(void);

private:
    OptionsButton *m_pLengthButton;
    OptionsButton *m_pHeightButton;
    int m_heightButtonLabelVal; // value currently displayed on height button: <0 for arc-sec, >0 for pixels
    OptionsButton *m_pSettingsButton;
    wxButton *m_pClearButton;
    wxCheckBox *m_pCheckboxTrendlines;
    wxCheckBox *m_pCheckboxCorrections;
    wxStaticText *RALabel;
    wxStaticText *DecLabel;
    wxStaticText *OscIndexLabel;
    wxStaticText *RMSLabel;
    wxBoxSizer *m_pControlSizer;
    GraphControlPane *m_pXControlPane;
    GraphControlPane *m_pYControlPane;
    GraphControlPane *m_pScopePane;

    bool m_visible;
    GraphLogClientWindow *m_pClient;

    int StringWidth(const wxString& string);
    void UpdateHeightButtonLabel(void);

    DECLARE_EVENT_TABLE()
};

class GraphControlPane : public wxWindow
{
public:
    GraphControlPane(wxWindow *pParent, const wxString& label);
    ~GraphControlPane(void);
protected:
    wxBoxSizer *m_pControlSizer;

    int StringWidth(const wxString& string);
    void DoAdd(wxControl *pCtrl, const wxString& lbl);
};

#endif
