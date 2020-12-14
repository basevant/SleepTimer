#include "stdafx.h"
#include "SystemHelper.h"

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
