#include "stdafx.h"
#include "ResourceManager.h"

const CString CResourceManager::LoadStringFromResource(
	const UINT stringID
	) throw ()
{
	WCHAR * pBuf = NULL;

	const int len = LoadStringW(
		NULL,
		stringID,
		reinterpret_cast<LPWSTR>(&pBuf),
		0
		);

	return len > 0 ? CString(pBuf, len) : CString();
}
