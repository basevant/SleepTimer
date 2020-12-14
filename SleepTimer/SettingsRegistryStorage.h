#pragma once
#include "RegistryKey.h"

class CSettingsRegistryStorage
{
private:
    const static LPCWSTR SLEEP_TIMER_REG_KEY;
    const static LPCWSTR SLEEP_TIMER_REG_PARAM_WND_X_POS;
    const static LPCWSTR SLEEP_TIMER_REG_PARAM_WND_Y_POS;

    CRegistryKey m_regKey;

public:
    BOOL ReadWindowPosition(POINTS& topLeftPoint) noexcept;

    BOOL WriteWindowPosition(const POINTS currentWindowPosition) noexcept;

private:
    BOOL OpenRegistryForReading();
    BOOL OpenRegistryForWriting();

    static BOOL StatusToBool(const LSTATUS status) noexcept;

    BOOL ReadDwordValue(
        const LPCWSTR lpValueName,
        DWORD& refDwValue
    ) const noexcept;

    BOOL WriteDwordValue(
        const LPCWSTR lpValueName,
        const DWORD dwValue
    ) const noexcept;
};
