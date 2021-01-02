#pragma once
#include "PowerOffType.h"
#include "TimerType.h"

class CSettings
{
private:
    POINTS m_topLeftWindowPositionCoordinates = POINTS{ 0, 0 };

    TimerType m_timerType = TimerTypeIn;

    unsigned short m_timerTypeInHours = 1;
    unsigned short m_timerTypeInMinutes = 0;

    unsigned short m_timerTypeAtHours = 23;
    unsigned short m_timerTypeAtMinutes = 0;

    PowerOffType m_powerOffType = PowerOffTypeHibernate;

public:
    static bool ValidateHours(const unsigned short hours) noexcept;
    static bool ValidateMinutes(const unsigned short minutes) noexcept;

    void SetTopLeftWindowPositionCoordinates(
        const POINTS topLeftWindowPositionCoordinates
    ) noexcept;

    POINTS GetTopLeftWindowPosition() const noexcept;

    void SetTimerType(
        const TimerType timerType
    ) noexcept;

    TimerType GetTimerType() const noexcept;

    void SetTimerTypeInHours(
        const unsigned short hours
    );

    unsigned short GetTimerTypeInHours() const noexcept;

    void SetTimerTypeInMinutes(
        const unsigned short minutes
    );

    unsigned short GetTimerTypeInMinutes() const noexcept;

    void SetTimerTypeAtHours(
        const unsigned short hours
    );

    unsigned short GetTimerTypeAtHours() const noexcept;

    void SetTimerTypeAtMinutes(
        const unsigned short minutes
    );

    unsigned short GetTimerTypeAtMinutes() const noexcept;

    void SetPowerOffType(
        const PowerOffType powerOffType
    ) noexcept;

    PowerOffType GetPowerOffType() const noexcept;
};
