// SleepTimer.cpp : main source file for SleepTimer.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;
bool AdjustShutDownPrivileges(void) throw();

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	if (!AdjustShutDownPrivileges())
	{
		MessageBox(
			NULL,
			CResourceManager::LoadStringFromResource(IDS_ERROR_UNABLE_TO_ADJUST_SHUTDOWN_PRIVILEGES),
			CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
			MB_OK | MB_ICONEXCLAMATION
			);

		return 0;
	}

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

bool AdjustShutDownPrivileges(void) throw()
{
	HANDLE hToken = NULL;

	TOKEN_PRIVILEGES tokenPrivileges;
	ZeroMemory(&tokenPrivileges, sizeof(TOKEN_PRIVILEGES));

	BOOL result = OpenProcessToken(
		GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken
		);

	if (FALSE == result)
	{
		return false;
	}

	result = LookupPrivilegeValueW(
		NULL,
		SE_SHUTDOWN_NAME,
		&tokenPrivileges.Privileges[0].Luid
		);

	if (FALSE == result)
	{
		if (NULL != hToken)
		{
			CloseHandle(hToken);
			hToken = NULL;
		}

		return false;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	result = AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tokenPrivileges,
		0,
		(PTOKEN_PRIVILEGES)NULL,
		0
		);

	if (
		(FALSE != result)
		&& (GetLastError() == ERROR_SUCCESS)
		)
	{
		if (NULL != hToken)
		{
			CloseHandle(hToken);
			hToken = NULL;
		}

		return true;
	}

	if (NULL != hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}

	return false;
}

