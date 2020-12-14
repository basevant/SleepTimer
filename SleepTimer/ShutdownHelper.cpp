#include "stdafx.h"
#include "ShutdownHelper.h"

#include "SystemHelper.h"

BOOL CShutdownHelper::AdjustShutDownPrivileges() noexcept
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

BOOLEAN CShutdownHelper::Hibernate()
{
    return SetSuspendState(
        TRUE,
        FALSE,
        FALSE
    );
}

BOOL CShutdownHelper::Shutdown()
{
    UINT shutdownFlags = EWX_SHUTDOWN;
    if (CSystemHelper::IsOperatingSystemIsWindows8OrGreater())
    {
        //	#define EWX_HYBRID_SHUTDOWN         0x00400000
        //	requires Win8 headers, using raw hex to compile using WinXP toolset
        shutdownFlags |= 0x00400000;
    }

    return ExitWindowsEx(
        shutdownFlags,
        SHTDN_REASON_FLAG_PLANNED
    );
}

BOOLEAN CShutdownHelper::Sleep()
{
    return SetSuspendState(
        FALSE,
        FALSE,
        FALSE
    );
}
