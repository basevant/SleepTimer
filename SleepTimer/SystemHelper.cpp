#include "stdafx.h"
#include "SystemHelper.h"

BOOL CSystemHelper::AdjustShutDownPrivileges() noexcept
{
    HANDLE hToken = nullptr;

    TOKEN_PRIVILEGES tokenPrivileges;
    ZeroMemory(&tokenPrivileges, sizeof(TOKEN_PRIVILEGES));

    auto result = OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
    );

    if (result == FALSE)
    {
        return result;
    }

    result = LookupPrivilegeValueW(
        nullptr,
        SE_SHUTDOWN_NAME,
        &tokenPrivileges.Privileges[0].Luid
    );

    if (result == FALSE)
    {
        if (nullptr != hToken)
        {
            CloseHandle(hToken);
            hToken = nullptr;
        }

        return result;
    }

    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    result = AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tokenPrivileges,
        0,
        static_cast<PTOKEN_PRIVILEGES>(nullptr),
        nullptr
    );

    if (hToken != nullptr)
    {
        CloseHandle(hToken);
        hToken = nullptr;
    }

    return result;
}

BOOL CSystemHelper::IsOperatingSystemIsWindows8OrGreater() noexcept
{
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (FALSE == GetVersionEx(&osvi))
    {
        return FALSE;
    }

    return (
        (osvi.dwMajorVersion >= 6)
        && (osvi.dwMinorVersion >= 2)
        );
}
