#pragma once

class CResourceManager
{
public:
    static CString LoadStringFromResource(
        const UINT stringId
    ) noexcept;
};
