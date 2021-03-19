#include "stdafx.h"
#include "SettingsRegistryStorage.h"

void CSettingsRegistryStorage::ReadUiSettings(CSettings& settings) noexcept
{
    settings = CSettings();

    if (OpenRegistryForReading() != TRUE)
    {
        return;
    }

    POINTS topLeftPoint;
    if (TryReadTopLeftPosition(topLeftPoint))
    {
        settings.SetTopLeftWindowPositionCoordinates(topLeftPoint);
    }

    TimerType timerType;
    if (TryReadTimerType(timerType))
    {
        settings.SetTimerType(timerType);
    }

    unsigned short shortValue;
    if (TryReadTimerTypeInHours(shortValue))
    {
        settings.SetTimerTypeInHours(shortValue);
    }

    if (TryReadTimerTypeInMinutes(shortValue))
    {
        settings.SetTimerTypeInMinutes(shortValue);
    }

    if (TryReadTimerTypeAtHours(shortValue))
    {
        settings.SetTimerTypeAtHours(shortValue);
    }

    if (TryReadTimerTypeAtMinutes(shortValue))
    {
        settings.SetTimerTypeAtMinutes(shortValue);
    }

    PowerOffType powerOffType;
    if (TryReadPowerOffType(powerOffType))
    {
        settings.SetPowerOffType(powerOffType);
    }
}

void CSettingsRegistryStorage::WriteUiSettings(const CSettings& settings) noexcept
{
    auto result = OpenRegistryForWriting();
    ATLASSERT(result == TRUE);
    if (result != TRUE)
    {
        return;
    }

    result = TryWriteTopLeftPosition(settings.GetTopLeftWindowPosition());
    ATLASSERT(result == TRUE);

    result = TryWriteTimerType(settings.GetTimerType());
    ATLASSERT(result == TRUE);

    result = TryWriteTimerTypeInHours(settings.GetTimerTypeInHours());
    ATLASSERT(result == TRUE);

    result = TryWriteTimerTypeInMinutes(settings.GetTimerTypeInMinutes());
    ATLASSERT(result == TRUE);

    result = TryWriteTimerTypeAtHours(settings.GetTimerTypeAtHours());
    ATLASSERT(result == TRUE);

    result = TryWriteTimerTypeAtMinutes(settings.GetTimerTypeAtMinutes());
    ATLASSERT(result == TRUE);

    result = TryWritePowerOffType(settings.GetPowerOffType());
    ATLASSERT(result == TRUE);
}

BOOL CSettingsRegistryStorage::OpenRegistryForReading()
{
    const auto status = m_regKey.OpenCurrentUserKeyForReading(SLEEP_TIMER_REG_KEY);

    auto const retVal = StatusToBool(status);

    return retVal;
}

BOOL CSettingsRegistryStorage::OpenRegistryForWriting()
{
    const auto status = m_regKey.OpenCurrentUserKeyForWriting(
        SLEEP_TIMER_REG_KEY,
        TRUE
    );

    auto const retVal = StatusToBool(status);

    ATLASSERT(retVal == TRUE);

    return retVal;
}

BOOL CSettingsRegistryStorage::StatusToBool(const LSTATUS status) noexcept
{
    return (
        status == ERROR_SUCCESS
        ? TRUE
        : FALSE
        );
}

BOOL CSettingsRegistryStorage::TryReadDwordValue(
    const LPCWSTR lpValueName,
    DWORD& refDwValue
) const noexcept
{
    auto const status = m_regKey.ReadDwordValue(lpValueName, refDwValue);
    auto const retVal = StatusToBool(status);

    ATLASSERT(retVal == TRUE);

    return retVal;
}

BOOL CSettingsRegistryStorage::WriteDwordValue(
    const LPCWSTR lpValueName,
    const DWORD dwValue
) const noexcept
{
    auto const status = m_regKey.WriteDwordValue(lpValueName, dwValue);
    auto const retVal = StatusToBool(status);

    ATLASSERT(retVal == TRUE);

    return retVal;
}

BOOL CSettingsRegistryStorage::TryReadTopLeftPosition(
    POINTS& topLeftPoint
) const noexcept
{
    DWORD dwX;

    auto result = TryReadDwordValue(
        SLEEP_TIMER_REG_PARAM_WND_X_POS,
        dwX
    );

    if (result != TRUE)
    {
        return FALSE;
    }

    DWORD dwY;

    result = TryReadDwordValue(
        SLEEP_TIMER_REG_PARAM_WND_Y_POS,
        dwY
    );

    if (result != TRUE)
    {
        return FALSE;
    }

    topLeftPoint.x = static_cast<SHORT>(dwX);
    topLeftPoint.y = static_cast<SHORT>(dwY);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadTimerType(
    TimerType& timerType
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_TIMER_TYPE,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    timerType = static_cast<TimerType>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadTimerTypeInHours(
    unsigned short& hours
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_HOURS,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    hours = static_cast<unsigned short>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadTimerTypeInMinutes(
    unsigned short& minutes
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_MINUTES,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    minutes = static_cast<unsigned short>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadTimerTypeAtHours(
    unsigned short& hours
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_HOURS,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    hours = static_cast<unsigned short>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadTimerTypeAtMinutes(
    unsigned short& minutes
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_MINUTES,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    minutes = static_cast<unsigned short>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryReadPowerOffType(
    PowerOffType& powerOffType
) const noexcept
{
    DWORD registryValue;

    if (
        TryReadDwordValue(
            SLEEP_TIMER_REG_PARAM_POWEROFF_TYPE,
            registryValue
        ) != TRUE
        )
    {
        return FALSE;
    }

    powerOffType = static_cast<PowerOffType>(registryValue);

    return TRUE;
}

BOOL CSettingsRegistryStorage::TryWriteTopLeftPosition(const POINTS topLeftPoint) const noexcept
{
    const auto verticalBorderWidth = GetSystemMetrics(SM_CYFIXEDFRAME);
    const auto posX = topLeftPoint.x - verticalBorderWidth;

    auto result = FALSE;

    if (posX >= 0)
    {
        result = WriteDwordValue(
            SLEEP_TIMER_REG_PARAM_WND_X_POS,
            static_cast<DWORD>(posX)
        );

        if (result != TRUE)
        {
            return result;
        }
    }

    if (topLeftPoint.y >= 0)
    {
        result = WriteDwordValue(
            SLEEP_TIMER_REG_PARAM_WND_Y_POS,
            static_cast<DWORD>(topLeftPoint.y)
        );
    }

    return result;
}

BOOL CSettingsRegistryStorage::TryWriteTimerType(const TimerType timerType) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_TIMER_TYPE,
        static_cast<DWORD>(timerType)
    );
}

BOOL CSettingsRegistryStorage::TryWriteTimerTypeInHours(
    const unsigned short hours
) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_HOURS,
        hours
    );
}

BOOL CSettingsRegistryStorage::TryWriteTimerTypeInMinutes(
    const unsigned short minutes
) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_TIMER_TYPE_IN_MINUTES,
        minutes
    );
}

BOOL CSettingsRegistryStorage::TryWriteTimerTypeAtHours(
    const unsigned short hours
) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_HOURS,
        hours
    );
}

BOOL CSettingsRegistryStorage::TryWriteTimerTypeAtMinutes(
    const unsigned short minutes
) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_TIMER_TYPE_AT_MINUTES,
        minutes
    );
}

BOOL CSettingsRegistryStorage::TryWritePowerOffType(const PowerOffType powerOffType) const noexcept
{
    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_POWEROFF_TYPE,
        static_cast<DWORD>(powerOffType)
    );
}
