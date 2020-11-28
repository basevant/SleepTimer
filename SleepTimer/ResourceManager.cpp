#include "stdafx.h"
#include "ResourceManager.h"

const CString CResourceManager::LoadStringFromResource(
    const UINT stringId
) noexcept
{
    WCHAR* pBuf = nullptr;

    auto const len = LoadStringW(
        nullptr,
        stringId,
        reinterpret_cast<LPWSTR>(&pBuf),
        0
    );

    return len > 0 ? CString(pBuf, len) : CString();
}
