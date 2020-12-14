#pragma once

class CShutdownHelper
{
public:
    static BOOL AdjustShutDownPrivileges() noexcept;
    static BOOLEAN Hibernate();
    static BOOL Shutdown();
    static BOOLEAN Sleep();
};
