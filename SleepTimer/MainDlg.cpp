// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

#include "SettingsRegistryStorage.h"
#include "ShutdownHelper.h"

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

    LoadAndApplyUiSettings();

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

LRESULT CMainDlg::OnBtnTimerClick(
    const WORD,
    const WORD,
    const HWND,
    const BOOL&
    )
{
    SaveUiSettings();

    if (m_isTicking)
    {
        EnableUiAndStopCountdown();
    }
    else
    {
        bool shutDownNow;

        if (GetTimerType() == TimerTypeIn)
        {
            auto const powerOffInHours = GetComboBoxSelectedItemData(IDC_CMB_IN_HRS);
            auto const powerOffInMinutes = GetComboBoxSelectedItemData(IDC_CMB_IN_MINS);

            shutDownNow = (
                (0 == powerOffInHours)
                && (0 == powerOffInMinutes)
                );

            if (!shutDownNow)
            {
                m_shutDownCountdownInSeconds = static_cast<long long>(
                    powerOffInHours * 3600 + powerOffInMinutes * 60
                    );
            }
        }
        else
        {
            auto const shutDownAtHours = GetComboBoxSelectedItemData(IDC_CMB_AT_HRS);
            auto const shutDownAtMinutes = GetComboBoxSelectedItemData(IDC_CMB_AT_MINS);

            const auto currentTime = CTime::GetCurrentTime();
            const auto currentHour = currentTime.GetHour();
            const auto currentMinute = currentTime.GetMinute();

            shutDownNow = (
                (shutDownAtHours == currentHour)
                && (shutDownAtMinutes == currentMinute)
                );

            if (!shutDownNow)
            {
                const auto shutDownIsInPast = (
                    (shutDownAtHours < currentHour)
                    || (shutDownAtHours == currentHour
                        && shutDownAtMinutes < currentMinute
                        )
                    );

                CTime shutDownAt;

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
                    shutDownAt = tomorrow + CTimeSpan(0, shutDownAtHours, shutDownAtMinutes, 0);
                }
                else
                {
                    shutDownAt = CTime(
                        currentTime.GetYear(),
                        currentTime.GetMonth(),
                        currentTime.GetDay(),
                        shutDownAtHours,
                        shutDownAtMinutes,
                        0
                    );
                }

                m_shutDownCountdownInSeconds = (shutDownAt - currentTime).GetTotalSeconds();
            }
        }

        if (shutDownNow)
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

    CheckRadioButton(
        IDC_RAD_OFF_AT,
        IDC_RAD_OFF_IN,
        shouldSetTimerTypeIn ? IDC_RAD_OFF_IN : IDC_RAD_OFF_AT
    );
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

void CMainDlg::SetComboActiveItem(
    const int comboId,
    const unsigned short itemValue
) const
{
    auto someCombo = static_cast<CComboBox>(GetDlgItem(comboId));
    auto const someComboItemsCount = someCombo.GetCount();

    for (auto i = 0; i < someComboItemsCount; i++)
    {
        const auto itemData = static_cast<unsigned short>(
            someCombo.GetItemData(i)
            );

        if (itemData == itemValue)
        {
            someCombo.SetCurSel(i);
            return;
        }
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

    for (auto i = 0; i < 12; i++)
    {
        comboMinutesValues.push_back(
            static_cast<unsigned short>(i * 5)
            );
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

    m_shutDownCountdownInSeconds--;

    ShowCountDown();
}

void CMainDlg::ProcessShutdownOption()
{
    if (!m_isTicking)
    {
        return;
    }

    const auto currentPowerOffType = GetPowerOffType();

    auto const currentTimeIsCautionTime = m_shutDownCountdownInSeconds <= 60;
    auto const currentTimeIsShutdownTime = m_shutDownCountdownInSeconds <= 0;

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
        StopPowerOffTimerAndHandlePowerOffType(currentPowerOffType);
    }
}

bool CMainDlg::RadioIsChecked(
    const int radioButtonId
    ) const noexcept
{
    return (
        GetDlgItem(radioButtonId).SendMessageW(BM_GETCHECK) != 0
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
    auto lblCountDown = GetDlgItem(IDC_LBL_OFF_ELPSD);
    auto lblPowerOffAt = GetDlgItem(IDC_LBL_OFF_AT);

    if (m_shutDownCountdownInSeconds > 0)
    {
        const CTimeSpan countDownValue(m_shutDownCountdownInSeconds);

        lblCountDown.SetWindowTextW(
            countDownValue.Format(TIMER_MASK)
            );

        const auto currentTime = CTime::GetCurrentTime();

        lblPowerOffAt.SetWindowTextW(
            (currentTime + countDownValue).Format(TIMER_MASK)
        );
    }
    else
    {
        lblCountDown.SetWindowTextW(L"00:00:00");

        lblPowerOffAt.SetWindowTextW(L"?");
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
    const POINTS topLeftWindowPosition
) noexcept
{
    return SetWindowPos(
        nullptr,
        topLeftWindowPosition.x,
        topLeftWindowPosition.y,
        -1,
        -1,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

unsigned short CMainDlg::GetComboBoxSelectedItemData(const int comboBoxId) const
{
    const auto someComboBox = static_cast<CComboBox>(GetDlgItem(comboBoxId));
    return static_cast<unsigned short>(someComboBox.GetItemData(someComboBox.GetCurSel()));
}

void CMainDlg::StopPowerOffTimerAndHandlePowerOffType(const PowerOffType powerOffType)
{
    m_isTicking = false;

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
}

PowerOffType CMainDlg::GetPowerOffType() const
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

void CMainDlg::EnableUiAndStopCountdown()
{
    StopShutdownTimer();

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

void CMainDlg::DisableUiAndStartCountdown()
{
    StartShutdownTimer();

    EnableOrDisableShutdownAtControls(FALSE);
    DisableControl(IDC_RAD_OFF_AT);

    EnableOrDisableShutdownInControls(FALSE);
    DisableControl(IDC_RAD_OFF_IN);

    DisableControl(IDC_RAD_PWR_HIBERNATE);
    DisableControl(IDC_RAD_PWR_OFF);
    DisableControl(IDC_RAD_PWR_SLEEP);

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
        StopPowerOffTimerAndHandlePowerOffType(GetPowerOffType());

        return;
    }

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

void CMainDlg::LoadAndApplyUiSettings() noexcept
{
    CSettings uiSettings;
    CSettingsRegistryStorage().ReadUiSettings(uiSettings);

    //  Window position
    //
    auto const topLeftPosition = uiSettings.GetTopLeftWindowPosition();
    auto const topLeftPositionIsValid = (
        (topLeftPosition.x > 0)
        && (topLeftPosition.y > 0)
        );

    auto const windowHasBeenMoved = (
        topLeftPositionIsValid
        && (TRUE == MoveWindowPositionToSavedPosition(topLeftPosition))
        );

    if (!windowHasBeenMoved)
    {
        CenterWindow();
    }

    //  UI controls
    //
    SetTimerTypeMode(uiSettings.GetTimerType());

    SetComboActiveItem(
        IDC_CMB_IN_HRS,
        uiSettings.GetTimerTypeInHours()
    );

    SetComboActiveItem(
        IDC_CMB_IN_MINS,
        uiSettings.GetTimerTypeInMinutes()
    );

    SetComboActiveItem(
        IDC_CMB_AT_HRS,
        uiSettings.GetTimerTypeAtHours()
    );

    SetComboActiveItem(
        IDC_CMB_AT_MINS,
        uiSettings.GetTimerTypeAtMinutes()
    );

    SetPreferablePowerOffTypeMode(uiSettings.GetPowerOffType());
}

void CMainDlg::SaveUiSettings() const noexcept
{
    CSettings uiSettings;

    //  Window position
    //
    RECT windowRect;
    SecureZeroMemory(&windowRect, sizeof(RECT));

    if (GetWindowRect(&windowRect) != FALSE)
    {
        uiSettings.SetTopLeftWindowPositionCoordinates(
            POINTS
            {
                static_cast<SHORT>(windowRect.left),
                static_cast<SHORT>(windowRect.top)
            }
        );
    }

    //  UI controls
    //
    uiSettings.SetTimerType(
        GetTimerType()
    );

    uiSettings.SetTimerTypeInHours(
        GetComboBoxSelectedItemData(IDC_CMB_IN_HRS)
    );

    uiSettings.SetTimerTypeInMinutes(
        GetComboBoxSelectedItemData(IDC_CMB_IN_MINS)
    );

    uiSettings.SetTimerTypeAtHours(
        GetComboBoxSelectedItemData(IDC_CMB_AT_HRS)
    );

    uiSettings.SetTimerTypeAtMinutes(
        GetComboBoxSelectedItemData(IDC_CMB_AT_MINS)
    );

    uiSettings.SetPowerOffType(
        GetPowerOffType()
    );

    CSettingsRegistryStorage().WriteUiSettings(uiSettings);
}

TimerType CMainDlg::GetTimerType() const noexcept
{
    return (
        RadioIsChecked(IDC_RAD_OFF_IN)
        ? TimerTypeIn
        : TimerTypeAt
        );
}
