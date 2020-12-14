#include "stdafx.h"
#include "SettingsRegistryStorage.h"

const LPCWSTR CSettingsRegistryStorage::SLEEP_TIMER_REG_KEY = L"Software\\SleepTimer";
const LPCWSTR CSettingsRegistryStorage::SLEEP_TIMER_REG_PARAM_WND_X_POS = L"X";
const LPCWSTR CSettingsRegistryStorage::SLEEP_TIMER_REG_PARAM_WND_Y_POS = L"Y";

BOOL CSettingsRegistryStorage::ReadWindowPosition(POINTS& topLeftPoint) noexcept
{
    auto result = OpenRegistryForReading();
    if (result != TRUE)
    {
        return FALSE;
    }

    DWORD dwX;

    result = ReadDwordValue(SLEEP_TIMER_REG_PARAM_WND_X_POS, dwX);

    if (result != TRUE)
    {
        return FALSE;
    }

    DWORD dwY;

    result = ReadDwordValue(SLEEP_TIMER_REG_PARAM_WND_Y_POS, dwY);

    if (result != TRUE)
    {
        return FALSE;
    }

    SecureZeroMemory(&topLeftPoint, sizeof(POINTS));

    topLeftPoint.x = static_cast<SHORT>(dwX);
    topLeftPoint.y = static_cast<SHORT>(dwY);

    return TRUE;
}

BOOL CSettingsRegistryStorage::WriteWindowPosition(
    const POINTS currentWindowPosition
) noexcept
{
    const auto windowCaptionAreaHeight = static_cast<DWORD>(GetSystemMetrics(SM_CYCAPTION));
    const auto verticalBorderWidth = static_cast<DWORD>(GetSystemMetrics(SM_CYFIXEDFRAME));
    const auto horizontalBorderHeight = static_cast<DWORD>(GetSystemMetrics(SM_CXFIXEDFRAME));

    const auto dwPosX = (
        static_cast<DWORD>(currentWindowPosition.x)
        - verticalBorderWidth
        );

    const auto dwPosY = (
        static_cast<DWORD>(currentWindowPosition.y)
        - windowCaptionAreaHeight
        - horizontalBorderHeight
        );

    OpenRegistryForWriting();

    auto const result = WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_WND_X_POS,
        dwPosX
    );

    if (result != TRUE)
    {
        return result;
    }

    return WriteDwordValue(
        SLEEP_TIMER_REG_PARAM_WND_Y_POS,
        dwPosY
    );
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

BOOL CSettingsRegistryStorage::ReadDwordValue(
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
