#pragma once

class CResourceManager
{
public:
    static const CString LoadStringFromResource(
        const UINT stringId
    ) noexcept;
};
