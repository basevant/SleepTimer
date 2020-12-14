#pragma once

class CSystemHelper
{
public:
    static BOOL AdjustShutDownPrivileges() noexcept;
    static BOOL IsOperatingSystemIsWindows8OrGreater() noexcept;
};
