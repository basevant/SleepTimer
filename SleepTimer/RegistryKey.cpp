#include "stdafx.h"
#include "RegistryKey.h"

CRegistryKey::~CRegistryKey()
{
    CloseKey();
}

LSTATUS CRegistryKey::OpenCurrentUserKeyForReading(const LPCWSTR lpSubKey)
{
    CloseKey();

    auto const lStatus = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        lpSubKey,
        0,
        KEY_READ,
        &m_hKey
    );

    return lStatus;
}

LSTATUS CRegistryKey::OpenCurrentUserKeyForWriting(
    const LPCWSTR lpSubKey,
    const BOOL createIfMissing
)
{
    CloseKey();

    auto lStatus = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        lpSubKey,
        0,
        KEY_READ | KEY_WRITE,
        &m_hKey
    );

    if (lStatus != ERROR_SUCCESS
        && (
            (lStatus == ERROR_FILE_NOT_FOUND)
            && createIfMissing
            )
        )
    {
        lStatus = RegCreateKey(
            HKEY_CURRENT_USER,
            lpSubKey,
            &m_hKey
        );
    }

    ATLASSERT(lStatus == ERROR_SUCCESS);

    return lStatus;
}

LSTATUS CRegistryKey::CloseKey()
{
    if (m_hKey == nullptr)
    {
        return ERROR_SUCCESS;
    }

    const auto status = RegCloseKey(m_hKey);
    ATLASSERT(status == ERROR_SUCCESS);

    m_hKey = nullptr;

    return status;
}

LSTATUS CRegistryKey::ReadDwordValue(
    const LPCWSTR lpValueName,
    DWORD & dwValue
) const
{
    ATLASSERT(m_hKey != nullptr);

    if (m_hKey == nullptr)
    {
        return ERROR_INVALID_HANDLE;
    }

    DWORD dwDataSize = 0;

    auto lStatus = RegQueryValueExW(
        m_hKey,
        lpValueName,
        nullptr,
        nullptr,
        nullptr,
        &dwDataSize
    );

    ATLASSERT(lStatus == ERROR_SUCCESS);

    if (lStatus != ERROR_SUCCESS)
    {
        return lStatus;
    }

    DWORD regValue = 0;

    lStatus = RegQueryValueExW(
        m_hKey,
        lpValueName,
        nullptr,
        nullptr,
        reinterpret_cast<LPBYTE>(&regValue),
        &dwDataSize
    );

    ATLASSERT(lStatus == ERROR_SUCCESS);

    if (lStatus != ERROR_SUCCESS)
    {
        return lStatus;
    }

    dwValue = regValue;

    return lStatus;
}

LSTATUS CRegistryKey::WriteDwordValue(
    const LPCWSTR lpValueName,
    const DWORD dwValue
) const noexcept
{
    ATLASSERT(m_hKey != nullptr);
    if (m_hKey == nullptr)
    {
        return ERROR_INVALID_HANDLE;
    }

    auto const lStatus = RegSetValueExW(
        m_hKey,
        lpValueName,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&dwValue),
        sizeof(dwValue)
    );

    ATLASSERT(lStatus == ERROR_SUCCESS);

    return lStatus;
}
