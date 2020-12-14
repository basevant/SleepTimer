#pragma once

class CRegistryKey
{
private:
    HKEY m_hKey = nullptr;
public:
    ~CRegistryKey();

    LSTATUS OpenCurrentUserKeyForReading(const LPCWSTR lpSubKey);

    LSTATUS OpenCurrentUserKeyForWriting(
        const LPCWSTR lpSubKey,
        const BOOL createIfMissing
    );

    LSTATUS CloseKey();

    LSTATUS ReadDwordValue(
        const LPCWSTR lpValueName,
        DWORD& dwValue
    ) const;

    LSTATUS WriteDwordValue(
        const LPCWSTR lpValueName,
        const DWORD dwValue
    ) const noexcept;
};
