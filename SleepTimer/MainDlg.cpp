// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

#include "SettingsRegistryStorage.h"
#include "ShutdownHelper.h"

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
    m_isHibernateAllowed = IsPwrHibernateAllowed();
    m_isSuspendAllowed = IsPwrSuspendAllowed();
    m_isShutdownAllowed = IsPwrShutdownAllowed();

    const auto canMakeAnyPowerOffAction = (
        m_isHibernateAllowed
        || m_isSuspendAllowed
        || m_isShutdownAllowed
        );

    if (!canMakeAnyPowerOffAction)
    {
        MessageBox(
            CResourceManager::LoadStringFromResource(IDS_ERROR_UNABLE_TO_ADJUST_SHUTDOWN_PRIVILEGES),
            CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
            MB_OK | MB_ICONEXCLAMATION
        );

        CloseDialog(0);

        return TRUE;
    }

    POINTS topLeftWindowPointFromRegistry;
    ZeroMemory(&topLeftWindowPointFromRegistry, sizeof(POINTS));

    auto const windowHasBeenMoved = (
        (TRUE == CSettingsRegistryStorage().ReadWindowPosition(topLeftWindowPointFromRegistry))
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

    SetPreferablePowerOffTypeMode(PowerOffTypeHibernate);

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
    CSettingsRegistryStorage().WriteWindowPosition(MAKEPOINTS(lParam));

    return 0;
}

LRESULT CMainDlg::OnBtnTimerClick(
    const WORD,
    const WORD,
    const HWND,
    const BOOL&
    )
{
    m_shutDownNow = false;

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

            m_shutDownNow = (
                (0 == powerOffInHours)
                && (0 == powerOffInMinutes)
                );

            if (!m_shutDownNow)
            {
                m_shutDownInSecondsCountdown = powerOffInHours * 3600 + powerOffInMinutes * 60;
                m_shutDownAt = CTime::GetCurrentTime() + CTimeSpan(0, powerOffInHours, powerOffInMinutes, 0);
            }
        }
        else
        {
            m_shutDownAtHours = GetComboBoxSelectedItemData(IDC_CMB_AT_HRS);
            m_shutDownAtMinutes = GetComboBoxSelectedItemData(IDC_CMB_AT_MINS);

            const auto currentTime = CTime::GetCurrentTime();
            const auto currentHour = currentTime.GetHour();
            const auto currentMinute = currentTime.GetMinute();

            m_shutDownNow = (
                (m_shutDownAtHours == currentHour)
                && (m_shutDownAtMinutes == currentMinute)
                );

            if (!m_shutDownNow)
            {
                const auto shutDownIsInPast = (
                    (m_shutDownAtHours < currentHour)
                    || (m_shutDownAtHours == currentHour
                        && m_shutDownAtMinutes < currentMinute
                        )
                    );

                if (shutDownIsInPast)
                {
                    //  now: 11:06
                    //  power-off at: 11:00 (tomorrow)

                    const auto midNight = CTime(
                        currentTime.GetYear(),
                        currentTime.GetMonth(),
                        currentTime.GetDay(),
                        23,
                        59,
                        59
                    );

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
        }

        if (m_shutDownNow)
        {
            ProcessShutdownNowCase();

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

void CMainDlg::SetPreferablePowerOffTypeMode(
    const PowerOffType& powerOffType
    ) noexcept
{
    EnableOrDisableControl(IDC_RAD_PWR_HIBERNATE, m_isHibernateAllowed);
    EnableOrDisableControl(IDC_RAD_PWR_SLEEP, m_isSuspendAllowed);
    EnableOrDisableControl(IDC_RAD_PWR_OFF, m_isShutdownAllowed);

    auto activePowerButtonId = -1;

    switch (powerOffType)
    {
        case PowerOffTypeHibernate:
        {
            if (m_isHibernateAllowed)
            {
                activePowerButtonId = IDC_RAD_PWR_HIBERNATE;
            }
        }
        break;

        case PowerOffTypeSuspend:
        {
            if (m_isSuspendAllowed)
            {
                activePowerButtonId = IDC_RAD_PWR_SLEEP;
            }
        }
        break;

        case PowerOffTypeShutdown:
        {
            if (m_isShutdownAllowed)
            {
                activePowerButtonId = IDC_RAD_PWR_OFF;
            }
        }
        break;
    }

    if (activePowerButtonId == -1)
    {
        if (m_isHibernateAllowed)
        {
            activePowerButtonId = IDC_RAD_PWR_HIBERNATE;
        }
        else if (m_isSuspendAllowed)
        {
            activePowerButtonId = IDC_RAD_PWR_SLEEP;
        }
        else if (m_isShutdownAllowed)
        {
            activePowerButtonId = IDC_RAD_PWR_OFF;
        }
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

void CMainDlg::ShowCurrentTime() const noexcept
{
    GetDlgItem(IDC_LBL_TIME).SetWindowTextW(
        CTime::GetCurrentTime().Format(TIMER_MASK)
        );
}

void CMainDlg::ProcessCountDown() noexcept
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

void CMainDlg::ProcessShutdownOption()
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

        //  minimize possible active window (like media player in full screen mode)
        //

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
            SendDlgItemMessage(
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

void CMainDlg::ShowCountDown() const noexcept
{
    const auto currentTime = CTime::GetCurrentTime();
    const auto countDownValue = m_shutDownAt - currentTime;

    auto lblCountDown = GetDlgItem(IDC_LBL_OFF_ELPSD);

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

BOOL CMainDlg::MoveWindowPositionToSavedPosition(
    POINTS topLeftWindowPointFromRegistry
) noexcept
{
    return SetWindowPos(
        nullptr,
        topLeftWindowPointFromRegistry.x,
        topLeftWindowPointFromRegistry.y,
        -1,
        -1,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
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
            CShutdownHelper::Hibernate();
        }
        break;

        case PowerOffTypeShutdown:
        {
            CShutdownHelper::Shutdown();
        }
        break;

        case PowerOffTypeSuspend:
        {
            CShutdownHelper::Sleep();
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

void CMainDlg::ProcessShutdownNowCase() noexcept
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

    m_shutDownNow = false;

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
