// SleepTimer.cpp : main source file for SleepTimer.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;
bool AdjustShutDownPrivileges() noexcept;

int Run(LPCTSTR /*lpctstrCmdLine*/ = nullptr, const int nCmdShow = SW_SHOWDEFAULT)
{
    if (!AdjustShutDownPrivileges())
    {
        MessageBox(
            nullptr,
            CResourceManager::LoadStringFromResource(IDS_ERROR_UNABLE_TO_ADJUST_SHUTDOWN_PRIVILEGES),
            CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
            MB_OK | MB_ICONEXCLAMATION
            );

        return 0;
    }

    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    CMainDlg dlgMain;

    if(dlgMain.Create(nullptr) == nullptr)
    {
        ATLTRACE(_T("Main dialog creation failed!\n"));
        return 0;
    }

    dlgMain.ShowWindow(nCmdShow);

    auto const nRet = theLoop.Run();

    _Module.RemoveMessageLoop();
    return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lptstrCmdLine, const int nCmdShow)
{
    auto hRes = ::CoInitialize(nullptr);
    ATLASSERT(SUCCEEDED(hRes));

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(nullptr, 0, 0, 0L);

    AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

    hRes = _Module.Init(nullptr, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    auto const nRet = Run(lptstrCmdLine, nCmdShow);

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}

bool AdjustShutDownPrivileges() noexcept
{
    HANDLE hToken = nullptr;

    TOKEN_PRIVILEGES tokenPrivileges;
    ZeroMemory(&tokenPrivileges, sizeof(TOKEN_PRIVILEGES));

    auto result = OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    if (FALSE == result)
    {
        return false;
    }

    result = LookupPrivilegeValueW(
        nullptr,
        SE_SHUTDOWN_NAME,
        &tokenPrivileges.Privileges[0].Luid
        );

    if (FALSE == result)
    {
        if (nullptr != hToken)
        {
            CloseHandle(hToken);
            hToken = nullptr;
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
        static_cast<PTOKEN_PRIVILEGES>(nullptr),
        nullptr
        );

    if (
        (FALSE != result)
        && (GetLastError() == ERROR_SUCCESS)
        )
    {
        if (nullptr != hToken)
        {
            CloseHandle(hToken);
            hToken = nullptr;
        }

        return true;
    }

    if (nullptr != hToken)
    {
        CloseHandle(hToken);
        hToken = nullptr;
    }

    return false;
}
