// SleepTimer.cpp : main source file for SleepTimer.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"
#include "SystemHelper.h"

CAppModule _Module;

int Run(LPCTSTR /*lpctstrCmdLine*/ = nullptr, const int nCmdShow = SW_SHOWDEFAULT)
{
    if (CSystemHelper::AdjustShutDownPrivileges() == FALSE)
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

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lptstrCmdLine, const int nShowCmd)
{
    auto hRes = ::CoInitialize(nullptr);
    ATLASSERT(SUCCEEDED(hRes));

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(nullptr, 0, 0, 0L);

    AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

    hRes = _Module.Init(nullptr, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    auto const nRet = Run(lptstrCmdLine, nShowCmd);

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}
