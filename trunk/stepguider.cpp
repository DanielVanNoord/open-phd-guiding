/*
   stepguider.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
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

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"

static const int DefaultSamplesToAverage = 3;
static const int DefaultBumpPercentage = 80;
static const double DefaultBumpMaxStepsPerCycle = 1.00;
static const int DefaultCalibrationStepsPerIteration = 4;
static const int DefaultGuideAlgorithm = GUIDE_ALGORITHM_IDENTITY;

StepGuider::StepGuider(void)
{
    m_xOffset = 0;
    m_yOffset = 0;
    m_bumpStepWeight = 1.0;

    wxString prefix = "/" + GetMountClassName();

    int samplesToAverage = pConfig->Profile.GetInt(prefix + "/SamplesToAverage", DefaultSamplesToAverage);
    SetSamplesToAverage(samplesToAverage);

    int bumpPercentage = pConfig->Profile.GetInt(prefix + "/BumpPercentage", DefaultBumpPercentage);
    SetBumpPercentage(bumpPercentage);

    int bumpMaxStepsPerCycle = pConfig->Profile.GetDouble(prefix + "/BumpMaxStepsPerCycle", DefaultBumpMaxStepsPerCycle);
    SetBumpMaxStepsPerCycle(bumpMaxStepsPerCycle);

    int calibrationStepsPerIteration = pConfig->Profile.GetInt(prefix + "/CalibrationStepsPerIteration", DefaultCalibrationStepsPerIteration);
    SetCalibrationStepsPerIteration(calibrationStepsPerIteration);

    int xGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/XGuideAlgorithm", DefaultGuideAlgorithm);
    SetXGuideAlgorithm(xGuideAlgorithm);

    int yGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/YGuideAlgorithm", DefaultGuideAlgorithm);
    SetYGuideAlgorithm(yGuideAlgorithm);
}

StepGuider::~StepGuider(void)
{
}

wxArrayString StepGuider::List(void)
{
    wxArrayString AoList;

    AoList.Add(_T("None"));
#ifdef STEPGUIDER_SXAO
    AoList.Add(_T("sxAO"));
#endif
#ifdef STEPGUIDER_SIMULATOR
    AoList.Add(_T("Simulator"));
#endif

    return AoList;
}

StepGuider *StepGuider::Factory(const wxString& choice)
{
    StepGuider *pReturn = NULL;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("StepGuiderFactory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("StepGuiderFactory(%s)", choice));

        if (choice.Find(_T("None")) + 1) {
        }
#ifdef STEPGUIDER_SXAO
        else if (choice.Find(_T("sxAO")) + 1) {
            pReturn = new StepGuiderSxAO();
        }
#endif
#ifdef STEPGUIDER_SIMULATOR
        else if (choice.Find(_T("Simulator")) + 1) {
            pReturn = new StepGuiderSimulator();
        }
#endif
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

bool StepGuider::Connect(void)
{
    bool bError = false;

    try
    {
        if (Mount::Connect())
        {
            throw ERROR_INFO("Mount::Connect() failed");
        }

        pFrame->pStepGuiderGraph->SetLimits(MaxPosition(LEFT), MaxPosition(UP), BumpPosition(LEFT), BumpPosition(UP));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuider::Disconnect(void)
{
    bool bError = false;

    try
    {
        pFrame->pStepGuiderGraph->SetLimits(0, 0, 0, 0);

        if (Mount::Disconnect())
        {
            throw ERROR_INFO("Mount::Disconnect() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

int StepGuider::IntegerPercent(int percentage, int number)
{
    long numerator =  (long)percentage*(long)number;
    long value =  numerator/100L;
    return (int)value;
}

int StepGuider::BumpPosition(GUIDE_DIRECTION direction)
{
    return IntegerPercent(m_bumpPercentage, MaxPosition(direction));
}

int StepGuider::GetSamplesToAverage(void)
{
    return m_samplesToAverage;
}

bool StepGuider::SetSamplesToAverage(int samplesToAverage)
{
    bool bError = false;

    try
    {
        if (samplesToAverage <= 0)
        {
            throw ERROR_INFO("invalid samplesToAverage");
        }

        m_samplesToAverage = samplesToAverage;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_samplesToAverage = DefaultSamplesToAverage;
    }

    pConfig->Profile.SetInt("/stepguider/SamplesToAverage", m_samplesToAverage);

    return bError;
}

int StepGuider::GetBumpPercentage(void)
{
    return m_bumpPercentage;
}

bool StepGuider::SetBumpPercentage(int bumpPercentage, bool updateGraph)
{
    bool bError = false;

    try
    {
        if (bumpPercentage <= 0)
        {
            throw ERROR_INFO("invalid bumpPercentage");
        }

        m_bumpPercentage = bumpPercentage;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_bumpPercentage = DefaultBumpPercentage;
    }

    pConfig->Profile.SetInt("/stepguider/BumpPercentage", m_bumpPercentage);

    if (updateGraph)
    {
        pFrame->pStepGuiderGraph->SetLimits(MaxPosition(LEFT), MaxPosition(UP), BumpPosition(LEFT), BumpPosition(UP));
    }

    return bError;
}

double StepGuider::GetBumpMaxStepsPerCycle(void)
{
    return m_bumpMaxStepsPerCycle;
}

bool StepGuider::SetBumpMaxStepsPerCycle(double bumpStepsPerCycle)
{
    bool bError = false;

    try
    {
        if (bumpStepsPerCycle <= 0.0)
        {
            throw ERROR_INFO("invalid bumpStepsPerCycle");
        }

        m_bumpMaxStepsPerCycle = bumpStepsPerCycle;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_bumpMaxStepsPerCycle = DefaultBumpMaxStepsPerCycle;
    }

    pConfig->Profile.SetDouble("/stepguider/BumpMaxStepsPerCycle", m_bumpMaxStepsPerCycle);

    return bError;
}

int StepGuider::GetCalibrationStepsPerIteration(void)
{
    return m_calibrationStepsPerIteration;
}

bool StepGuider::SetCalibrationStepsPerIteration(int calibrationStepsPerIteration)
{
    bool bError = false;

    try
    {
        if (calibrationStepsPerIteration <= 0.0)
        {
            throw ERROR_INFO("invalid calibrationStepsPerIteration");
        }

        m_calibrationStepsPerIteration = calibrationStepsPerIteration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_calibrationStepsPerIteration = DefaultCalibrationStepsPerIteration;
    }

    pConfig->Profile.SetInt("/stepguider/CalibrationStepsPerIteration", m_calibrationStepsPerIteration);

    return bError;
}

void StepGuider::ZeroCurrentPosition()
{
    m_xOffset = 0;
    m_yOffset = 0;
}

bool StepGuider::MoveToCenter()
{
    bool bError = false;

    try
    {
        int positionUpDown = CurrentPosition(UP);

        if (positionUpDown > 0)
        {
            if (Move(DOWN, positionUpDown) != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step DOWN");
            }
        }
        else if (positionUpDown < 0)
        {
            positionUpDown = -positionUpDown;

            if (Move(UP, positionUpDown) != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step UP");
            }
        }

        int positionLeftRight = CurrentPosition(LEFT);

        if (positionLeftRight > 0)
        {
            if (Move(RIGHT, positionLeftRight) != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step RIGHT");
            }
        }
        else if (positionLeftRight < 0)
        {
            positionLeftRight = -positionLeftRight;

            if (Move(LEFT, positionLeftRight) != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step LEFT");
            }
        }

        assert(m_xOffset == 0);
        assert(m_yOffset == 0);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

int StepGuider::CurrentPosition(GUIDE_DIRECTION direction)
{
    int ret=0;

    switch(direction)
    {
        case UP:
            ret =  m_yOffset;
            break;
        case DOWN:
            ret = -m_yOffset;
            break;
        case RIGHT:
            ret =  m_xOffset;
            break;
        case LEFT:
            ret = -m_xOffset;
            break;
    }

    return ret;
}

void StepGuider::ClearCalibration(void)
{
    Mount::ClearCalibration();

    m_calibrationState = CALIBRATION_STATE_CLEARED;
}

bool StepGuider::BeginCalibration(const PHD_Point& currentLocation)
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
            throw ERROR_INFO("Must have a valid start position");
        }

        ClearCalibration();
        m_calibrationState = CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER;
        m_calibrationStartingLocation = currentLocation;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void StepGuider::SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination, PierSide pierSide)
{
    m_calibrationXAngle = xAngle;
    m_calibrationXRate = xRate;
    m_calibrationYAngle = yAngle;
    m_calibrationYRate = yRate;
    Mount::SetCalibration(xAngle, yAngle, xRate, yRate, declination, pierSide);
}

/*
 * The Stepguider calibration sequence is a state machine:
 *
 *  - it is assumed that the stepguider starts out centered, so
 *  - The initial state moves the stepguider into the lower right corner. Then,
 *  - the guider moves left for the full travel of the guider to compute the
 *    x calibration values, then
 *  - the guider moves up for the full travel of guider to compute the
 *    y calibration values, then
 *  - the guider returns to the center of its travel and calibration is complete
 */

bool StepGuider::UpdateCalibrationState(const PHD_Point &currentLocation)
{
    bool bError = false;

    try
    {
        wxString status0, status1;
        int stepsRemainingUp = MaxPosition(UP) - CurrentPosition(UP);
        int stepsRemainingDown = MaxPosition(DOWN) - CurrentPosition(DOWN);
        int stepsRemainingRight  = MaxPosition(RIGHT)  - CurrentPosition(RIGHT);
        int stepsRemainingLeft  = MaxPosition(LEFT)  - CurrentPosition(LEFT);

        stepsRemainingUp /= m_calibrationStepsPerIteration;
        stepsRemainingDown /= m_calibrationStepsPerIteration;
        stepsRemainingRight  /= m_calibrationStepsPerIteration;
        stepsRemainingLeft  /= m_calibrationStepsPerIteration;

        int stepsRemainingDownAndRight = wxMax(stepsRemainingDown, stepsRemainingRight);

        assert(stepsRemainingUp >= 0);
        assert(stepsRemainingDown >= 0);
        assert(stepsRemainingRight  >= 0);
        assert(stepsRemainingLeft  >= 0);
        assert(stepsRemainingDownAndRight    >= 0);


        bool moveUp = false;
        bool moveDown = false;
        bool moveRight  = false;
        bool moveLeft  = false;

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER:
                if (stepsRemainingDownAndRight > 0)
                {
                    status0.Printf(_("Init Calibration: %3d"), stepsRemainingDownAndRight);
                    moveDown = stepsRemainingDown > 0;
                    moveRight  = stepsRemainingRight > 0;
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_STARTING_LOCATION, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_STARTING_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_STARTING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage -m_calibrationAverageSamples+1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                Debug.AddLine(wxString::Format("Falling through to state GO_LEFT, startinglocation=(%.2f, %.2f)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                m_calibrationState = CALIBRATION_STATE_GO_LEFT;
                // fall through
            case CALIBRATION_STATE_GO_LEFT:
                if (stepsRemainingLeft > 0)
                {
                    status0.Printf(_("Left Calibration: %3d"), stepsRemainingLeft);
                    m_calibrationIterations++;
                    moveLeft  = true;
                    GuideLog.CalibrationStep(this, "Left", stepsRemainingLeft,
                        m_calibrationStartingLocation.dX(currentLocation),  m_calibrationStartingLocation.dY(currentLocation),
                        currentLocation, m_calibrationStartingLocation.Distance(currentLocation));
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_CENTER_LOCATION, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_CENTER_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_CENTER_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage -m_calibrationAverageSamples+1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibrationXAngle = m_calibrationStartingLocation.Angle(m_calibrationAveragedLocation);
                m_calibrationXRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                                     (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.2f rate=%.2f"), m_calibrationXAngle, m_calibrationXRate);
                GuideLog.CalibrationDirectComplete(this, "Left", m_calibrationXAngle, m_calibrationXRate);
                Debug.AddLine(wxString::Format("LEFT calibration completes with angle=%.2f rate=%.2f", m_calibrationXAngle, m_calibrationXRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                m_calibrationState = CALIBRATION_STATE_GO_UP;
                Debug.AddLine(wxString::Format("Falling through to state GO_UP, startinglocation=(%.2f, %.2f)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                // fall through
            case CALIBRATION_STATE_GO_UP:
                if (stepsRemainingUp > 0)
                {
                    status0.Printf(_("up Calibration: %3d"), stepsRemainingUp);
                    m_calibrationIterations++;
                    moveUp = true;
                    GuideLog.CalibrationStep(this, "Up", stepsRemainingLeft,
                        m_calibrationStartingLocation.dX(currentLocation),  m_calibrationStartingLocation.dY(currentLocation),
                        currentLocation, m_calibrationStartingLocation.Distance(currentLocation));
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_ENDING_LOCATION, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_ENDING_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_ENDING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage -m_calibrationAverageSamples+1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibrationYAngle = m_calibrationAveragedLocation.Angle(m_calibrationStartingLocation);
                m_calibrationYRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                                     (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.2f rate=%.2f"), m_calibrationYAngle, m_calibrationYRate);
                GuideLog.CalibrationDirectComplete(this, "Left", m_calibrationYAngle, m_calibrationYRate);
                Debug.AddLine(wxString::Format("UP calibration completes with angle=%.2f rate=%.2f", m_calibrationYAngle, m_calibrationYRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationState = CALIBRATION_STATE_RECENTER;
                Debug.AddLine(wxString::Format("Falling through to state RECENTER, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_RECENTER:
                status0.Printf(_("Finish Calibration: %3d"), stepsRemainingDownAndRight/2);
                moveRight = (CurrentPosition(LEFT) >= m_calibrationStepsPerIteration);
                moveDown = (CurrentPosition(UP) >= m_calibrationStepsPerIteration);
                if (moveRight || moveDown)
                {
                    Debug.AddLine(wxString::Format("CurrentPosition(LEFT)=%d CurrentPosition(UP)=%d", CurrentPosition(LEFT), CurrentPosition(UP)));
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_COMPLETE;
                Debug.AddLine(wxString::Format("Falling through to state COMPLETE, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_COMPLETE:
                SetCalibration(m_calibrationXAngle, m_calibrationYAngle,
                               m_calibrationXRate,  m_calibrationYRate,
                               0.0, PIER_SIDE_UNKNOWN);
                status1 = _T("calibration complete");
                GuideLog.CalibrationComplete(this);
                Debug.AddLine("Calibration Complete");
                break;
            default:
                assert(false);
                break;
        }

        if (moveUp)
        {
            assert(!moveDown);
            pFrame->ScheduleCalibrationMove(this, UP);
        }

        if (moveDown)
        {
            assert(!moveUp);
            pFrame->ScheduleCalibrationMove(this, DOWN);
        }

        if (moveRight)
        {
            assert(!moveLeft);
            pFrame->ScheduleCalibrationMove(this, RIGHT);
        }

        if (moveLeft)
        {
            assert(!moveRight);
            pFrame->ScheduleCalibrationMove(this, LEFT);
        }

        if (m_calibrationState != CALIBRATION_STATE_COMPLETE)
        {
            if (status1.IsEmpty())
            {
                double dX = m_calibrationStartingLocation.dX(currentLocation);
                double dY = m_calibrationStartingLocation.dY(currentLocation);
                double dist = m_calibrationStartingLocation.Distance(currentLocation);
                status1.Printf(_T("dx=%4.1f dy=%4.1f dist=%4.1f"), dX, dY, dist);
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
        bError = true;

        ClearCalibration();
    }

    return bError;
}

bool StepGuider::GuidingCeases(void)
{
    bool bError = false;

    // We have stopped guiding.  Reset bump state and recenter the stepguider

    m_avgOffset.Invalidate();
    m_bumpStepWeight = 1.0;
    m_bumpRemaining.Invalidate();
    // clear bump display in stepguider graph
    pFrame->pStepGuiderGraph->ShowBump(PHD_Point(), m_bumpRemaining);

    try
    {
        if (MoveToCenter())
        {
            throw ERROR_INFO("MoveToCenter() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuider::CalibrationMove(GUIDE_DIRECTION direction)
{
    bool bError = false;

    Debug.AddLine(wxString::Format("stepguider calibration move dir= %d steps= %d", direction, m_calibrationStepsPerIteration));

    try
    {
        double stepsTaken = Move(direction, m_calibrationStepsPerIteration, false);

        if (stepsTaken != m_calibrationStepsPerIteration)
        {
            throw THROW_INFO("stepsTaken != m_calibrationStepsPerIteration");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

double StepGuider::Move(GUIDE_DIRECTION direction, double amount, bool normalMove)
{
    int steps = 0;

    try
    {
        Debug.AddLine(wxString::Format("Move(%d, %.2f, %d)", direction, amount, normalMove));

        // Compute the required guide steps
        if (m_guidingEnabled)
        {
            // Acutally do the guide
            steps = (int)(amount + 0.5);
            assert(steps >= 0);

            if (steps > 0)
            {
                int yDirection = 0;
                int xDirection = 0;

                switch (direction)
                {
                    case UP:
                        yDirection = 1;
                        break;
                    case DOWN:
                        yDirection = -1;
                        break;
                    case RIGHT:
                        xDirection = 1;
                        break;
                    case LEFT:
                        xDirection = -1;
                        break;
                    default:
                        throw ERROR_INFO("StepGuider::Move(): invalid direction");
                        break;
                }

                assert(yDirection == 0 || xDirection == 0);
                assert(yDirection != 0 || xDirection != 0);

                Debug.AddLine(wxString::Format("stepping direction=%d steps=%d xDirection=%d yDirection=%d", direction, steps, xDirection, yDirection));

                if (WouldHitLimit(direction, steps))
                {
                    int new_steps = MaxPosition(direction) - 1 - CurrentPosition(direction);
                    Debug.AddLine(wxString::Format("StepGuider step would hit limit: truncate move direction=%d steps=%d => %d", direction, steps, new_steps));
                    steps = new_steps;
                }

                if (steps > 0)
                {
                    if (Step(direction, steps))
                    {
                        throw ERROR_INFO("step failed");
                    }

                    m_xOffset += xDirection * steps;
                    m_yOffset += yDirection * steps;

                    Debug.AddLine(wxString::Format("stepped: xOffset=%d yOffset=%d", m_xOffset, m_yOffset));
                }
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = -1;
    }

    return (double)steps;
}

bool StepGuider::Move(const PHD_Point& cameraVectorEndpoint, bool normalMove)
{
    bool bError = false;

    try
    {
        bool moveFailed = Mount::Move(cameraVectorEndpoint, normalMove);

        // keep a moving average of the AO position
        if (m_avgOffset.IsValid())
        {
            static double const alpha = .25; // moderately high weighting for latest sample
            m_avgOffset.X += alpha * (m_xOffset - m_avgOffset.X);
            m_avgOffset.Y += alpha * (m_yOffset - m_avgOffset.Y);
        }
        else
        {
            m_avgOffset.SetXY((double) m_xOffset, (double) m_yOffset);
        }

        pFrame->pStepGuiderGraph->AppendData(m_xOffset, m_yOffset, m_avgOffset);

        // consider bumping the secondary mount if this is a normal move
        if (normalMove && pSecondaryMount)
        {
            bool isOutside = abs(CurrentPosition(RIGHT)) > BumpPosition(RIGHT) ||
                abs(CurrentPosition(UP)) > BumpPosition(UP);

            // if the current bump has not brought us in, increase the bump size
            if (isOutside && m_bumpRemaining.IsValid())
            {
                Debug.AddLine("outside bump range, increase bump weight %.2f => %.2f", m_bumpStepWeight, m_bumpStepWeight + 1.0);
                m_bumpStepWeight += 1.0;
            }

            // if we are back inside, decrease the bump weight
            if (!isOutside && m_bumpStepWeight > 1.0)
            {
                double prior = m_bumpStepWeight;
                m_bumpStepWeight *= 0.5;
                if (m_bumpStepWeight < 1.0)
                    m_bumpStepWeight = 1.0;
                Debug.AddLine("back inside bump range: decrease bump weight %.2f => %.2f", prior, m_bumpStepWeight);
            }

            if (isOutside)
            {
                // start a new bump based on average position
                PHD_Point vectorEndpoint(xRate() * -m_avgOffset.X,
                                         yRate() * -m_avgOffset.Y);

                // we have to transform our notion of where we are (which is in "AO Coordinates")
                // into "Camera Coordinates" so we can bump the secondary mount to put us closer
                // to the center of the AO

                if (TransformMountCoordinatesToCameraCoordinates(vectorEndpoint, m_bumpRemaining))
                {
                    throw ERROR_INFO("MountToCamera failed");
                }

                Debug.AddLine("starting a new bump (%.3f, %.3f) isValid = %d", m_bumpRemaining.X, m_bumpRemaining.Y, m_bumpRemaining.IsValid());

                assert(m_bumpRemaining.IsValid());
#ifdef BRET_AO_DEBUG
            m_bumpRemaining += cameraVectorEndpoint;
#endif
            }
        }

        if (m_bumpRemaining.IsValid() && pSecondaryMount->IsBusy())
            Debug.AddLine("secondary mount is busy, cannot bump");

        // if we have a bump in progress and the secondary mount is not moving,
        // schedule another move
        if (m_bumpRemaining.IsValid() && !pSecondaryMount->IsBusy())
        {
            double xBumpSize = 0.0, yBumpSize = 0.0;
            bool willBump = false;

            // for bumping, since we don't know which way we are moving (in mount coordinates),
            // we just use the average calibration rate.  That should be close enough for our purposes

            Debug.AddLine("bumping secondary for remaining (%.3f, %.3f)", m_bumpRemaining.X, m_bumpRemaining.Y);

            // we are close enough to done -- avoiding direct comparison
            // to 0.0 because with floating point we may never get 0.0
            if (fabs(m_bumpRemaining.X) >= 0.01)
            {
                double maxBumpPixels = m_bumpMaxStepsPerCycle * m_calibrationXRate * m_bumpStepWeight;
                xBumpSize = m_bumpRemaining.X;

                if (fabs(xBumpSize) > maxBumpPixels)
                {
                    Debug.AddLine("clamp x bump to %.3f (%.3f * %.3f * %.3f)", maxBumpPixels,
                        m_bumpMaxStepsPerCycle, m_calibrationXRate, m_bumpStepWeight);

                    xBumpSize = maxBumpPixels;

                    if (m_bumpRemaining.X < 0.0)
                    {
                        xBumpSize *= -1.0;
                    }
                }

                willBump = true;
            }

            if (fabs(m_bumpRemaining.Y) >= 0.01)
            {
                double maxBumpPixels = m_bumpMaxStepsPerCycle * m_calibrationYRate * m_bumpStepWeight;
                yBumpSize = m_bumpRemaining.Y;

                if (fabs(yBumpSize) > maxBumpPixels)
                {
                    Debug.AddLine("clamp y bump to %.3f (%.3f * %.3f * .3f)", maxBumpPixels,
                        m_bumpMaxStepsPerCycle, m_calibrationYRate, m_bumpStepWeight);

                    yBumpSize = maxBumpPixels;

                    if (m_bumpRemaining.Y < 0.0)
                    {
                        yBumpSize *= -1.0;
                    }
                }

                willBump = true;
            }

            if (!willBump)
            {
                Debug.AddLine("Mount Bump finished -- invalidating m_bumpRemaining");

                m_bumpRemaining.Invalidate();

                pFrame->pStepGuiderGraph->ShowBump(PHD_Point(), m_bumpRemaining);
            }
            else
            {
                PHD_Point thisBump(xBumpSize, yBumpSize);

                // display the current and remaining bump vectors on the stepguider graph
                {
                    PHD_Point tcur, trem;
                    TransformCameraCoordinatesToMountCoordinates(thisBump, tcur);
                    tcur.X /= xRate();
                    tcur.Y /= yRate();
                    TransformCameraCoordinatesToMountCoordinates(m_bumpRemaining, trem);
                    trem.X /= xRate();
                    trem.Y /= yRate();
                    pFrame->pStepGuiderGraph->ShowBump(tcur, trem);
                }

                Debug.AddLine("Scheduling Mount bump of (%.3f, %.3f)", thisBump.X, thisBump.Y);

                pFrame->ScheduleSecondaryMove(pSecondaryMount, thisBump, false);

                m_bumpRemaining -= thisBump;

                Debug.AddLine("after bump, remaining is (%.3f, %.3f) valid=%d",
                    m_bumpRemaining.X, m_bumpRemaining.Y, m_bumpRemaining.IsValid());
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

bool StepGuider::IsAtLimit(GUIDE_DIRECTION direction, bool& atLimit)
{
    atLimit = CurrentPosition(direction) == MaxPosition(direction) - 1;
    return false;
}

bool StepGuider::WouldHitLimit(GUIDE_DIRECTION direction, int steps)
{
    bool bReturn = false;

    assert(steps >= 0);

    if (CurrentPosition(direction) + steps >= MaxPosition(direction))
    {
        bReturn = true;
    }

    Debug.AddLine(wxString::Format("WouldHitLimit=%d current=%d, steps=%d, max=%d", bReturn, CurrentPosition(direction), steps, MaxPosition(direction)));

    return bReturn;
}

wxString StepGuider::GetSettingsSummary() {
    // return a loggable summary of current mount settings
    return Mount::GetSettingsSummary() +
        wxString::Format("Calibration steps = %d, Samples to average = %d, Bump percentage = %d, Bump step = %.2f\n",
            GetCalibrationStepsPerIteration(),
            GetSamplesToAverage(),
            GetBumpPercentage(),
            GetBumpMaxStepsPerCycle()
        );
}

wxString StepGuider::GetMountClassName() const
{
    return wxString("stepguider");
}

bool StepGuider::IsStepGuider(void) const
{
    return true;
}

const char *StepGuider::DirectionStr(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "None";
    case UP:    return "Up";
    case DOWN:  return "Down";
    case RIGHT: return "Right";
    case LEFT:  return "Left";
    default:    return "?";
    }
}

const char *StepGuider::DirectionChar(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "-";
    case UP:    return "U";
    case DOWN:  return "D";
    case RIGHT: return "R";
    case LEFT:  return "L";
    default:    return "?";
    }
}

ConfigDialogPane *StepGuider::GetConfigDialogPane(wxWindow *pParent)
{
    return new StepGuiderConfigDialogPane(pParent, this);
}

StepGuider::StepGuiderConfigDialogPane::StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : MountConfigDialogPane(pParent, _("AO"), pStepGuider)
{
    int width;

    m_pStepGuider = pStepGuider;

    width = StringWidth(_T("000"));
    m_pCalibrationStepsPerIteration = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10, 3,_T("Cal_Steps"));

    DoAdd(_("Calibration Steps"), m_pCalibrationStepsPerIteration,
        wxString::Format(_("How many steps should be issued per calibration cycle. Default = %d, increase for short f/l scopes and decrease for longer f/l scopes"), DefaultCalibrationStepsPerIteration));

    width = StringWidth(_T("000"));
    m_pSamplesToAverage = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 9, 0, _T("Samples_To_Average"));

    DoAdd(_("Samples to Average"), m_pSamplesToAverage,
        wxString::Format(_("When calibrating, how many samples should be averaged. Default = %d, increase for worse seeing and small imaging scales"), DefaultSamplesToAverage));

    width = StringWidth(_T("000"));
    m_pBumpPercentage = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 99, 0, _T("Bump_Percentage"));

    DoAdd(_("Bump Percentage"), m_pBumpPercentage,
        wxString::Format(_("What percentage of the AO travel can be used before bumping the mount. Default = %d"), DefaultBumpPercentage));

    width = StringWidth(_T("00.00"));
    m_pBumpMaxStepsPerCycle = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.01, 99.99, 0.0, 0.25, _T("Bump_steps"));

    DoAdd(_("Bump Step"), m_pBumpMaxStepsPerCycle,
        wxString::Format(_("How far should a mount bump move the mount between images (in AO steps). Default = %.2f, decrease if mount bumps cause spikes on the graph"), DefaultBumpMaxStepsPerCycle));
}

StepGuider::StepGuiderConfigDialogPane::~StepGuiderConfigDialogPane(void)
{
}

void StepGuider::StepGuiderConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationStepsPerIteration->SetValue(m_pStepGuider->GetCalibrationStepsPerIteration());
    m_pSamplesToAverage->SetValue(m_pStepGuider->GetSamplesToAverage());
    m_pBumpPercentage->SetValue(m_pStepGuider->GetBumpPercentage());
    m_pBumpMaxStepsPerCycle->SetValue(m_pStepGuider->GetBumpMaxStepsPerCycle());
}

void StepGuider::StepGuiderConfigDialogPane::UnloadValues(void)
{
    m_pStepGuider->SetCalibrationStepsPerIteration(m_pCalibrationStepsPerIteration->GetValue());
    m_pStepGuider->SetSamplesToAverage(m_pSamplesToAverage->GetValue());
    m_pStepGuider->SetBumpPercentage(m_pBumpPercentage->GetValue(), true);
    m_pStepGuider->SetBumpMaxStepsPerCycle(m_pBumpMaxStepsPerCycle->GetValue());

    MountConfigDialogPane::UnloadValues();
}
