#include "stdafx.h"
#include "Settings.h"

void CSettings::SetTopLeftWindowPositionCoordinates(
    const POINTS topLeftWindowPositionCoordinates
) noexcept
{
    m_topLeftWindowPositionCoordinates = topLeftWindowPositionCoordinates;
}

POINTS CSettings::GetTopLeftWindowPosition() const noexcept
{
    return m_topLeftWindowPositionCoordinates;
}

void CSettings::SetTimerType(const TimerType timerType) noexcept
{
    m_timerType = timerType;
}

TimerType CSettings::GetTimerType() const noexcept
{
    return m_timerType;
}

void CSettings::SetTimerTypeInHours(const unsigned short hours)
{
    m_timerTypeInHours = (
        ValidateHours(hours)
        ? hours
        : static_cast<unsigned short>(1)
        );
}

unsigned short CSettings::GetTimerTypeInHours() const noexcept
{
    return m_timerTypeInHours;
}

void CSettings::SetTimerTypeInMinutes(const unsigned short minutes)
{
    m_timerTypeInMinutes = (
        ValidateMinutes(minutes)
        ? minutes
        : static_cast<unsigned short>(0)
        );
}

unsigned short CSettings::GetTimerTypeInMinutes() const noexcept
{
    return m_timerTypeInMinutes;
}

void CSettings::SetTimerTypeAtHours(const unsigned short hours)
{
    m_timerTypeAtHours = (
        ValidateHours(hours)
        ? hours
        : static_cast<unsigned short>(23)
        );
}

unsigned short CSettings::GetTimerTypeAtHours() const noexcept
{
    return m_timerTypeAtHours;
}

void CSettings::SetTimerTypeAtMinutes(const unsigned short minutes)
{
    m_timerTypeAtMinutes = (
        ValidateMinutes(minutes)
        ? minutes
        : static_cast<unsigned short>(0)
        );
}

unsigned short CSettings::GetTimerTypeAtMinutes() const noexcept
{
    return m_timerTypeAtMinutes;
}

void CSettings::SetPowerOffType(const PowerOffType powerOffType) noexcept
{
    m_powerOffType = powerOffType;
}

PowerOffType CSettings::GetPowerOffType() const noexcept
{
    return m_powerOffType;
}

bool CSettings::ValidateHours(const unsigned short hours) noexcept
{
    return (hours <= 23);
}

bool CSettings::ValidateMinutes(const unsigned short minutes) noexcept
{
    return (minutes <= 59);
}
