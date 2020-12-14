// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

CMainDlg::CMainDlg():
    m_timerType()
{
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
    return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
    UIUpdateChildWindows();
    return FALSE;
}

LRESULT CMainDlg::OnInitDialog(
    const UINT,
    const WPARAM,
    const LPARAM,
    const BOOL&
    )
{
    POINTS topLeftWindowPointFromRegistry;
    ZeroMemory(&topLeftWindowPointFromRegistry, sizeof(POINTS));

    auto const windowHasBeenMoved = (
        (TRUE == GetWindowTopLeftPositionFromRegistry(topLeftWindowPointFromRegistry))
        && (TRUE == MoveWindowPositionToSavedPosition(topLeftWindowPointFromRegistry))
        );

    if (!windowHasBeenMoved)
    {
        // center the dialog on the screen
        CenterWindow();
    }

    // set icons
    auto *const hIcon = AtlLoadIconImage(
        IDR_MAINFRAME,
        LR_DEFAULTCOLOR,
        ::GetSystemMetrics(SM_CXICON),
        ::GetSystemMetrics(SM_CYICON)
    );

    SetIcon(hIcon, TRUE);

    auto *const hIconSmall = AtlLoadIconImage(
        IDR_MAINFRAME,
        LR_DEFAULTCOLOR,
        ::GetSystemMetrics(SM_CXSMICON),
        ::GetSystemMetrics(SM_CYSMICON)
    );

    SetIcon(hIconSmall, FALSE);

    // register object for message filtering and idle updates
    auto *const pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);

    UIAddChildWindowContainer(m_hWnd);

    //
    //	Custom UI initialization
    //

    FillHoursCombo(IDC_CMB_AT_HRS, 0);
    FillMinutesCombo(IDC_CMB_AT_MINS);

    FillHoursCombo(IDC_CMB_IN_HRS, 1);
    FillMinutesCombo(IDC_CMB_IN_MINS);

    SendDlgItemMessage(IDC_RAD_OFF_IN, BM_CLICK);

    SetPowerOffTypeMode(PowerOffTypeHibernate);

    StartShutdownTimer();
    StartCurrentTimeTimer();

    ShowCurrentTime();

    return TRUE;
}

LRESULT CMainDlg::OnDestroy(
    const UINT,
    const WPARAM,
    const LPARAM,
    const BOOL&
    )
{
    StopShutdownTimer();
    StopCurrentTimeTimer();

    // unregister message filtering and idle updates
    auto * pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->RemoveMessageFilter(this);

    return 0;
}

LRESULT CMainDlg::OnCancel(
    const WORD,
    const WORD wId,
    const HWND,
    const BOOL&
    ) noexcept
{
    if (m_isTicking)
    {
        const auto msgBoxResult = MessageBox(
            CResourceManager::LoadStringFromResource(IDS_CAUTION_CONFIRM_EXIT),
            CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
            MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL
            );

        if (IDNO == msgBoxResult)
        {
            return 0;
        }
    }

    CloseDialog(wId);
    return 0;
}

LRESULT CMainDlg::OnTimerModeAtClick(
    const WORD,
    const WORD,
    const HWND,
    const BOOL&
    ) noexcept
{
    SetTimerTypeMode(TimerTypeAt);

    return 0;
}

LRESULT CMainDlg::OnTimerModeInClick(
    const WORD,
    const WORD,
    const HWND,
    const BOOL&
    ) noexcept
{
    SetTimerTypeMode(TimerTypeIn);

    return 0;
}

LRESULT CMainDlg::OnTimer(
    const UINT,
    const WPARAM wParam,
    const LPARAM,
    const BOOL&
)
{
    if (wParam == SHUTDOWN_TIMER_ID)
    {
        ProcessShutdownOption();
        ProcessCountDown();
    }

    if (wParam == CURRENT_TIME_TIMER_ID)
    {
        ShowCurrentTime();
    }

    return 0;
}

LRESULT CMainDlg::OnMove(
    const UINT,
    const WPARAM,
    const LPARAM lParam,
    const BOOL&
) noexcept
{
    SaveWindowPosition(MAKEPOINTS(lParam));

    return 0;
}

LRESULT CMainDlg::OnBtnTimerClick(
    const WORD,
    const WORD,
    const HWND,
    const BOOL&
    )
{
    m_shutDownByZeros = false;

    if (m_isTicking)
    {
        EnableUiAndStopCountdown();
    }
    else
    {
        if (TRUE == RadioIsChecked(IDC_RAD_OFF_IN))
        {
            const unsigned short powerOffInHours = GetComboBoxSelectedItemData(IDC_CMB_IN_HRS);
            const unsigned short powerOffInMinutes = GetComboBoxSelectedItemData(IDC_CMB_IN_MINS);

            m_shutDownByZeros = (
                (0 == powerOffInHours)
                && (0 == powerOffInMinutes)
                );

            m_shutDownInSecondsCountdown = powerOffInHours * 3600 + powerOffInMinutes * 60;
            m_shutDownAt = CTime::GetCurrentTime() + CTimeSpan(0, powerOffInHours, powerOffInMinutes, 0);
        }
        else
        {
            m_shutDownAtHours = GetComboBoxSelectedItemData(IDC_CMB_AT_HRS);
            m_shutDownAtMinutes = GetComboBoxSelectedItemData(IDC_CMB_AT_MINS);

            m_shutDownByZeros = (
                (0 == m_shutDownAtHours)
                && (0 == m_shutDownAtMinutes)
                );

            const auto currentTime = CTime::GetCurrentTime();

            if (m_shutDownAtHours < currentTime.GetHour())
            {
                const auto midNight = CTime(currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay(), 23, 59, 59);
                const auto tomorrow = midNight + CTimeSpan(0, 0, 0, 1);
                m_shutDownAt = tomorrow + CTimeSpan(0, m_shutDownAtHours, m_shutDownAtMinutes, 0);
            }
            else
            {
                m_shutDownAt = CTime(
                    currentTime.GetYear(),
                    currentTime.GetMonth(),
                    currentTime.GetDay(),
                    m_shutDownAtHours,
                    m_shutDownAtMinutes,
                    0
                    );
            }
        }

        if (m_shutDownByZeros)
        {
            ProcessShutdownByZerosCase();

            return 0;
        }

        DisableUiAndStartCountdown();

        m_isCautionMessageAlreadyShown = false;
    }

    m_isTicking = !m_isTicking;

    return 0;
}

void CMainDlg::CloseDialog(
    const int nVal
    ) noexcept
{
    DestroyWindow();
    ::PostQuitMessage(nVal);
}

void CMainDlg::SetTimerTypeMode(
    const TimerType& timerType
    ) noexcept
{
    const auto shouldSetTimerTypeIn = (TimerTypeIn == timerType ? TRUE : FALSE);

    EnableOrDisableShutdownAtControls(!shouldSetTimerTypeIn);

    EnableOrDisableShutdownInControls(shouldSetTimerTypeIn);

    m_timerType = timerType;
}

void CMainDlg::SetPowerOffTypeMode(
    const PowerOffType& powerOffType
    ) noexcept
{
    const auto isHibernateAllowed = IsPwrHibernateAllowed();
    const auto isSuspendAllowed = IsPwrSuspendAllowed();

    EnableOrDisableControl(IDC_RAD_PWR_HIBERNATE, isHibernateAllowed);
    EnableOrDisableControl(IDC_RAD_PWR_SLEEP, isSuspendAllowed);

    auto activePowerButtonId = 0;

    switch (powerOffType)
    {
    case PowerOffTypeHibernate:
    {
        if (isHibernateAllowed)
        {
            activePowerButtonId = IDC_RAD_PWR_HIBERNATE;
        }
    }
        break;

    case PowerOffTypeSuspend:
    {
        if (isSuspendAllowed)
        {
            activePowerButtonId = IDC_RAD_PWR_SLEEP;
        }
    }
        break;
    }

    if (0 == activePowerButtonId)
    {
        activePowerButtonId = IDC_RAD_PWR_OFF;
    }

    CheckRadioButton(
        IDC_RAD_PWR_OFF,
        IDC_RAD_PWR_SLEEP,
        activePowerButtonId
        );
}

void CMainDlg::FillCombo(
    const int comboId,
    const std::vector<unsigned short>& comboValues,
    const unsigned short selectedItemIdx
    ) const
{
    auto someCombo = static_cast<CComboBox>(GetDlgItem(comboId));

    for (const auto& p : comboValues)
    {
        CString comboText;
        comboText.Format(L"%s%d", p < 10 ? L"0" : L"", p);

        const auto newStringIdx = someCombo.AddString(comboText);
        someCombo.SetItemData(newStringIdx, p);
    }

    if (comboValues.size() > selectedItemIdx)
    {
        someCombo.SetCurSel(selectedItemIdx);
    }
}

void CMainDlg::FillHoursCombo(
    const int comboId,
    const unsigned short selectedIndex
    ) const
{
    std::vector<unsigned short> comboHoursValues;

    for (unsigned short i = 0; i <= 23; i++)
    {
        comboHoursValues.push_back(i);
    }

    FillCombo(comboId, comboHoursValues, selectedIndex);
}

void CMainDlg::FillMinutesCombo(const int comboId) const
{
    std::vector<unsigned short> comboMinutesValues;

    for (unsigned short i = 0; i < 12; i++)
    {
        comboMinutesValues.push_back(i * 5);
    }

    FillCombo(comboId, comboMinutesValues, 0);
}

void CMainDlg::ShowCurrentTime(void) const noexcept
{
    GetDlgItem(IDC_LBL_TIME).SetWindowTextW(
        CTime::GetCurrentTime().Format(TIMER_MASK)
        );
}

void CMainDlg::ProcessCountDown(void) noexcept
{
    if (!m_isTicking)
    {
        return;
    }

    if (m_timerType == TimerTypeIn)
    {
        m_shutDownInSecondsCountdown--;
    }

    ShowCountDown();
}

void CMainDlg::ProcessShutdownOption(void) noexcept
{
    if (!m_isTicking)
    {
        return;
    }

    const auto currentPowerOffType = static_cast<PowerOffType>(GetPowerOffType());

    bool currentTimeIsCautionTime;
    bool currentTimeIsShutdownTime;

    const auto currentTime = CTime::GetCurrentTime();

    if (m_timerType == TimerTypeAt)
    {
        const auto currentTimePlusOneMinute = currentTime + CTimeSpan(0, 0, 1, 0);

        currentTimeIsCautionTime = (
            currentTimePlusOneMinute.GetHour() == m_shutDownAtHours
            && currentTimePlusOneMinute.GetMinute() == m_shutDownAtMinutes
            );

        currentTimeIsShutdownTime = (
            m_shutDownAt <= CTime::GetCurrentTime()
            );
    }
    else
    {
        currentTimeIsCautionTime = m_shutDownInSecondsCountdown <= 60;
        currentTimeIsShutdownTime = m_shutDownInSecondsCountdown <= 0;
    }

    if (currentTimeIsCautionTime && !m_isCautionMessageAlreadyShown)
    {
        m_isCautionMessageAlreadyShown = true;

        auto* const hWndForegroundWindow = GetForegroundWindow();
        if (
            (nullptr != hWndForegroundWindow)
            && (m_hWnd != hWndForegroundWindow)
            )
        {
            ::ShowWindow(
                hWndForegroundWindow,
                SW_MINIMIZE
            );
        }

        ShowWindow(SW_SHOWDEFAULT);

        const auto powerOffTypeMessage = GetPowerOffTypeDescription(currentPowerOffType);

        CStringW stopShutdownCautionMessage;
        stopShutdownCautionMessage.Format(
            CResourceManager::LoadStringFromResource(IDS_CAUTION_MESSAGE_BEFORE_SHUTDOWN_TEMPLATE),
            static_cast<LPCWSTR>(powerOffTypeMessage)
        );

        const auto stopShutdownCautionMessageBoxResult = MessageBox(
            stopShutdownCautionMessage,
            CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
            MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND
        );

        if (IDYES == stopShutdownCautionMessageBoxResult)
        {
            ::SendDlgItemMessage(
                m_hWnd,
                IDC_BTN_START,
                BM_CLICK,
                0,
                0
            );
        }
    }

    if (currentTimeIsShutdownTime)
    {
        PowerOffAndExit(currentPowerOffType);
    }
}

bool CMainDlg::RadioIsChecked(
    const UINT_PTR radioButtonId
    ) const noexcept
{
    return (
        GetDlgItem(radioButtonId).SendMessageW(BM_GETCHECK, 0, 0L) != 0
        );
}

void CMainDlg::EnableOrDisableShutdownAtControls(
    const BOOL ctrlsAreEnabled
    ) const noexcept
{
    EnableOrDisableControl(IDC_CMB_AT_HRS, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_LBL_AT, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_CMB_AT_MINS, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_LBL_AT_MINS, ctrlsAreEnabled);
}

void CMainDlg::EnableOrDisableShutdownInControls(
    const BOOL ctrlsAreEnabled
    ) const noexcept
{
    EnableOrDisableControl(IDC_CMB_IN_HRS, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_LBL_IN_HRS, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_CMB_IN_MINS, ctrlsAreEnabled);
    EnableOrDisableControl(IDC_LBL_IN_MINS, ctrlsAreEnabled);
}

void CMainDlg::ShowCountDown(void) const noexcept
{
    const CTime currentTime = CTime::GetCurrentTime();
    const CTimeSpan countDownValue = m_shutDownAt - currentTime;

    CWindow lblCountDown = GetDlgItem(IDC_LBL_OFF_ELPSD);

    if (countDownValue.GetTotalSeconds() > 0)
    {
        lblCountDown.SetWindowTextW(
            countDownValue.Format(TIMER_MASK)
            );
    }
    else
    {
        lblCountDown.SetWindowTextW(
            L"00:00:00"
            );
    }
}

void CMainDlg::EnableOrDisableControl(
    const int controlId,
    const BOOL ctrIsEnabled
    ) const noexcept
{
    GetDlgItem(controlId).EnableWindow(ctrIsEnabled);
}

void CMainDlg::DisableControl(
    const int controlId
    ) const noexcept
{
    EnableOrDisableControl(controlId, FALSE);
}

void CMainDlg::EnableControl(
    const int controlId
    ) const noexcept
{
    EnableOrDisableControl(controlId);
}

BOOL CMainDlg::IsWindows8(void) noexcept
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

BOOL CMainDlg::GetWindowTopLeftPositionFromRegistry(POINTS& topLeftPoint) noexcept
{
    DWORD dwX;
    if (TRUE != ReadDwordRegValue(SLEEP_TIMER_REG_PARAM_WND_X_POS, dwX))
    {
        return FALSE;
    }

    DWORD dwY;
    if (TRUE != ReadDwordRegValue(SLEEP_TIMER_REG_PARAM_WND_Y_POS, dwY))
    {
        return FALSE;
    }

    SecureZeroMemory(&topLeftPoint, sizeof(POINTS));

    topLeftPoint.x = (SHORT)dwX;
    topLeftPoint.y = (SHORT)dwY;

    return TRUE;
}

BOOL CMainDlg::ReadDwordRegValue(
    LPCWSTR lpValueName,
    DWORD& refValue
) noexcept
{
    HKEY hKey;

    LSTATUS lStatus = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        SLEEP_TIMER_REG_KEY,
        0,
        KEY_READ,
        &hKey
    );

    if (lStatus != ERROR_SUCCESS)
    {
        return FALSE;
    }

    DWORD dwDataSize = 0;

    lStatus = RegQueryValueExW(
        hKey,
        lpValueName,
        NULL,
        NULL,
        NULL,
        &dwDataSize
    );

    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        hKey = NULL;

        return FALSE;
    }

    DWORD regValue;

    lStatus = RegQueryValueExW(
        hKey,
        lpValueName,
        NULL,
        NULL,
        (LPBYTE)&regValue,
        &dwDataSize
    );

    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        hKey = NULL;

        return FALSE;
    }

    refValue = regValue;

    RegCloseKey(hKey);
    hKey = NULL;

    return TRUE;
}

BOOL CMainDlg::MoveWindowPositionToSavedPosition(
    POINTS topLeftWindowPointFromRegistry
) noexcept
{
    return SetWindowPos(
        NULL,
        topLeftWindowPointFromRegistry.x,
        topLeftWindowPointFromRegistry.y,
        -1,
        -1,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

BOOL CMainDlg::SaveWindowPosition(
    POINTS currentWindowPosition
) noexcept
{
    HKEY hKey;

    LSTATUS lStatus = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        SLEEP_TIMER_REG_KEY,
        0,
        KEY_WRITE,
        &hKey
    );

    if (lStatus != ERROR_SUCCESS)
    {
        if (lStatus == ERROR_FILE_NOT_FOUND)
        {
            lStatus = RegCreateKey(
                HKEY_CURRENT_USER,
                SLEEP_TIMER_REG_KEY,
                &hKey
            );

            if (lStatus != ERROR_SUCCESS)
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    const DWORD windowCaptionAreaHeight = static_cast<DWORD>(GetSystemMetrics(SM_CYCAPTION));
    const DWORD verticalBorderWidth = static_cast<DWORD>(GetSystemMetrics(SM_CYFIXEDFRAME));
    const DWORD horizontalBorderHeight = static_cast<DWORD>(GetSystemMetrics(SM_CXFIXEDFRAME));

    const DWORD dwPosX = (
        static_cast<DWORD>(currentWindowPosition.x)
        - verticalBorderWidth
        );

    const DWORD dwPosY = (
        static_cast<DWORD>(currentWindowPosition.y)
        - windowCaptionAreaHeight
        - horizontalBorderHeight
        );

    lStatus = RegSetValueExW(
        hKey,
        SLEEP_TIMER_REG_PARAM_WND_X_POS,
        0,
        REG_DWORD,
        (const BYTE*)&dwPosX,
        sizeof(dwPosX)
    );

    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        hKey = NULL;

        return FALSE;
    }

    lStatus = RegSetValueExW(
        hKey,
        SLEEP_TIMER_REG_PARAM_WND_Y_POS,
        0,
        REG_DWORD,
        (const BYTE*)&dwPosY,
        sizeof(dwPosY)
    );

    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        hKey = NULL;

        return FALSE;
    }

    RegCloseKey(hKey);
    hKey = NULL;

    return TRUE;
}

byte CMainDlg::GetComboBoxSelectedItemData(const int comboBoxId) const
{
    const auto someComboBox = static_cast<CComboBox>(GetDlgItem(comboBoxId));
    return static_cast<byte>(someComboBox.GetItemData(someComboBox.GetCurSel()));
}

void CMainDlg::PowerOffAndExit(const PowerOffType powerOffType)
{
    switch (powerOffType)
    {
        case PowerOffTypeHibernate:
        {
            SetSuspendState(
                TRUE,
                FALSE,
                FALSE
            );
        }
        break;

        case PowerOffTypeShutdown:
        {
            UINT shutdownFlags = EWX_SHUTDOWN;
            if (IsWindows8())
            {
                //	#define EWX_HYBRID_SHUTDOWN         0x00400000
                //	requires Win8 headers, using raw hex to compile for WinXP
                shutdownFlags |= 0x00400000;
            }

            ExitWindowsEx(
                shutdownFlags,
                SHTDN_REASON_FLAG_PLANNED
            );
        }
        break;

        case PowerOffTypeSuspend:
        {
            SetSuspendState(
                FALSE,
                FALSE,
                FALSE
            );
        }
        break;
    }

    CloseDialog(0);
}

int CMainDlg::GetPowerOffType() const
{
    PowerOffType retVal;

    if (RadioIsChecked(IDC_RAD_PWR_HIBERNATE))
    {
        retVal = PowerOffTypeHibernate;
    }
    else if (RadioIsChecked(IDC_RAD_PWR_OFF))
    {
        retVal = PowerOffTypeShutdown;
    }
    else
    {
        retVal = PowerOffTypeSuspend;
    }

    return retVal;
}

CString CMainDlg::GetPowerOffTypeDescription(const PowerOffType powerOffType)
{
    //	http://msdn.microsoft.com/en-us/library/windows/desktop/ms645505%28v=vs.85%29.aspx
    //
    CString retVal;

    switch (powerOffType)
    {
        case PowerOffTypeHibernate:
        {
            retVal = CResourceManager::LoadStringFromResource(IDS_PWR_HIBERNATE_CAUTION);
        }
        break;

        case PowerOffTypeShutdown:
        {
            retVal = CResourceManager::LoadStringFromResource(IDS_PWR_SHUTDOWN_CAUTION);
        }
        break;

        case PowerOffTypeSuspend:
        {
            retVal = CResourceManager::LoadStringFromResource(IDS_PWR_SLEEP_CAUTION);
        }
        break;
    }

    return retVal;
}

void CMainDlg::EnableUiAndStopCountdown() const
{
    EnableOrDisableShutdownAtControls(TRUE);
    EnableControl(IDC_RAD_OFF_AT);

    EnableOrDisableShutdownInControls(TRUE);
    EnableControl(IDC_RAD_OFF_IN);

    EnableControl(IDC_RAD_PWR_HIBERNATE);
    EnableControl(IDC_RAD_PWR_OFF);
    EnableControl(IDC_RAD_PWR_SLEEP);

    GetDlgItem(IDC_LBL_OFF_AT).SetWindowTextW(
        CResourceManager::LoadStringFromResource(IDS_EMPTY_TIME)
    );

    GetDlgItem(IDC_LBL_OFF_ELPSD).SetWindowTextW(
        CResourceManager::LoadStringFromResource(IDS_EMPTY_TIME)
    );

    GetDlgItem(IDC_BTN_START).SetWindowTextW(
        CResourceManager::LoadStringFromResource(IDS_TIMER_IS_STOPPED_SHOULD_I_START_IT)
    );
}

void CMainDlg::DisableUiAndStartCountdown() const
{
    EnableOrDisableShutdownAtControls(FALSE);
    DisableControl(IDC_RAD_OFF_AT);

    EnableOrDisableShutdownInControls(FALSE);
    DisableControl(IDC_RAD_OFF_IN);

    DisableControl(IDC_RAD_PWR_HIBERNATE);
    DisableControl(IDC_RAD_PWR_OFF);
    DisableControl(IDC_RAD_PWR_SLEEP);

    GetDlgItem(IDC_LBL_OFF_AT).SetWindowTextW(
        m_shutDownAt.Format(TIMER_MASK)
    );

    ShowCountDown();

    GetDlgItem(IDC_BTN_START).SetWindowTextW(
        CResourceManager::LoadStringFromResource(IDS_TIMER_IS_TICKING_SHOULD_I_STOP_IT)
    );
}

void CMainDlg::StartShutdownTimer() noexcept
{
    SetTimer(SHUTDOWN_TIMER_ID, 1000);
}

void CMainDlg::StopShutdownTimer() noexcept
{
    KillTimer(SHUTDOWN_TIMER_ID);
}

void CMainDlg::ProcessShutdownByZerosCase() noexcept
{
    StopShutdownTimer();

    const auto confirmShutdownNowMessage = CResourceManager::LoadStringFromResource(
        IDS_CAUTION_MESSAGE_SHUTDOWN_BY_ZEROS
    );

    const auto powerOffNowMessageBoxResult = MessageBox(
        confirmShutdownNowMessage,
        CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
        MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND
    );

    if (IDYES == powerOffNowMessageBoxResult)
    {
        PowerOffAndExit(
            static_cast<PowerOffType>(GetPowerOffType())
        );

        return;
    }

    m_shutDownByZeros = false;

    StartShutdownTimer();
}

void CMainDlg::StartCurrentTimeTimer() noexcept
{
    SetTimer(CURRENT_TIME_TIMER_ID, 1000);
}

void CMainDlg::StopCurrentTimeTimer() noexcept
{
    KillTimer(CURRENT_TIME_TIMER_ID);
}
