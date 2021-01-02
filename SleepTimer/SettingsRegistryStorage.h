#pragma once
#include "RegistryKey.h"
#include "Settings.h"

class CSettingsRegistryStorage
{
private:
    const LPCWSTR SLEEP_TIMER_REG_KEY = L"Software\\SleepTimer";

    const LPCWSTR SLEEP_TIMER_REG_PARAM_WND_X_POS = L"X";
    const LPCWSTR SLEEP_TIMER_REG_PARAM_WND_Y_POS = L"Y";

    const LPCWSTR SLEEP_TIMER_REG_PARAM_TIMER_TYPE = L"TimerType";

    const LPCWSTR SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_HOURS = L"InHours";
    const LPCWSTR SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_MINUTES = L"InMinutes";

    const LPCWSTR SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_HOURS = L"AtHours";
    const LPCWSTR SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_MINUTES = L"AtMinutes";

    const LPCWSTR SLEEP_TIMER_REG_PARAM_POWEROFF_TYPE = L"PowerOffType";

    CRegistryKey m_regKey;
public:
    void ReadUiSettings(CSettings& settings) noexcept;

    void WriteUiSettings(const CSettings& settings) noexcept;

private:
    BOOL OpenRegistryForReading();
    BOOL OpenRegistryForWriting();

    static BOOL StatusToBool(const LSTATUS status) noexcept;

    BOOL TryReadDwordValue(
        const LPCWSTR lpValueName,
        DWORD& refDwValue
    ) const noexcept;

    BOOL WriteDwordValue(
        const LPCWSTR lpValueName,
        const DWORD dwValue
    ) const noexcept;

    BOOL TryReadTopLeftPosition(POINTS& topLeftPoint) const noexcept;

    BOOL TryReadTimerType(TimerType& timerType) const noexcept;

    BOOL TryReadTimerTypeInHours(unsigned short& hours) const noexcept;

    BOOL TryReadTimerTypeInMinutes(unsigned short& minutes) const noexcept;

    BOOL TryReadTimerTypeAtHours(unsigned short& hours) const noexcept;

    BOOL TryReadTimerTypeAtMinutes(unsigned short& minutes) const noexcept;

    BOOL TryReadPowerOffType(PowerOffType& powerOffType) const noexcept;

    BOOL TryWriteTopLeftPosition(const POINTS topLeftPoint) const noexcept;

    BOOL TryWriteTimerType(const TimerType timerType) const noexcept;

    BOOL TryWriteTimerTypeInHours(const unsigned short hours) const noexcept;

    BOOL TryWriteTimerTypeInMinutes(const unsigned short minutes) const noexcept;

    BOOL TryWriteTimerTypeAtHours(const unsigned short hours) const noexcept;

    BOOL TryWriteTimerTypeAtMinutes(const unsigned short minutes) const noexcept;

    BOOL TryWritePowerOffType(const PowerOffType powerOffType) const noexcept;
};
