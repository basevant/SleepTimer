// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

CMainDlg::CMainDlg()
{
	//	fill the Hours list
	//

	for (unsigned short i = 0; i <= 24; i++)
	{
		m_comboIdxToHour.insert({ i, i });
	}

	//	fill the Minutes list
	//

	for (unsigned short i = 0; i < 12; i++)
	{
		m_comboIdxToMinutes.insert({ i, i * 5 });
	}

	m_isTicking = false;
	m_shutDownByZeros = false;
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
	) throw()
{
    POINTS topLeftWindowPointFromRegistry;
    ZeroMemory(&topLeftWindowPointFromRegistry, sizeof(POINTS));

    const bool windowHasBeenMoved = (
        (TRUE == GetWindowTopLeftPositionFromRegistry(topLeftWindowPointFromRegistry))
        && (TRUE == MoveWindowPositionToSavedPosition(topLeftWindowPointFromRegistry))
        );

    if (!windowHasBeenMoved)
    {
        // center the dialog on the screen
        CenterWindow();
    }

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);

	UIAddChildWindowContainer(m_hWnd);

	//
	//	Custom UI initialization
	//

	FillCombo(IDC_CMB_AT_HRS, m_comboIdxToHour, 1);
	FillCombo(IDC_CMB_AT_MINS, m_comboIdxToMinutes, 0);

	FillCombo(IDC_CMB_IN_HRS, m_comboIdxToHour, 1);
	FillCombo(IDC_CMB_IN_MINS, m_comboIdxToMinutes, 0);

	SendDlgItemMessage(IDC_RAD_OFF_IN, BM_CLICK);

	SetPowerOffTypeMode(PowerOffTypeHibernate);

	SetTimer(CURRENT_TIME_TIMER_ID, 1000);

	ShowCurrentTime();

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(
	const UINT,
	const WPARAM,
	const LPARAM,
	const BOOL&
	) throw()
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);

	return 0;
}

LRESULT CMainDlg::OnCancel(
	const WORD,
	const WORD wID,
	const HWND,
	const BOOL&
	) throw()
{
	if (m_isTicking)
	{
		const int msgBoxResult = MessageBox(
			CResourceManager::LoadStringFromResource(IDS_CAUTION_CONFIRM_EXIT),
			CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
			MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL
			);

		if (IDNO == msgBoxResult)
		{
			return 0;
		}
	}

	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnTimerModeAtClick(
	const WORD,
	const WORD,
	const HWND,
	const BOOL&
	) throw()
{
	SetTimerTypeMode(TimerTypeAt);

	return 0;
}

LRESULT CMainDlg::OnTimerModeInClick(
	const WORD,
	const WORD,
	const HWND,
	const BOOL&
	) throw()
{
	SetTimerTypeMode(TimerTypeIn);

	return 0;
}

LRESULT CMainDlg::OnTimer(
	const UINT,
	const WPARAM wParam,
	const LPARAM,
	const BOOL&
	) throw()
{
	const UINT_PTR timerId = static_cast<UINT_PTR>(wParam);

	switch (timerId)
	{
		case CURRENT_TIME_TIMER_ID:
		{
			ProcessShutdownOption();
			ShowCurrentTime();
			ProcessCountDown();
		}
		break;
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
	) throw()
{
	m_shutDownByZeros = false;

	if (m_isTicking)
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
	else
	{
		if (TRUE == RadioIsChecked(IDC_RAD_OFF_IN))
		{
			const CComboBox hoursCombo = GetDlgItem(IDC_CMB_IN_HRS);
			const unsigned short powerOffHours = m_comboIdxToHour[static_cast<unsigned short>(hoursCombo.GetCurSel())];

			const CComboBox minutesCombo = GetDlgItem(IDC_CMB_IN_MINS);
			const unsigned short powerOffMinutes = m_comboIdxToMinutes[static_cast<unsigned short>(minutesCombo.GetCurSel())];

			m_shutDownByZeros = (
				(0 == powerOffHours)
				&& (0 == powerOffMinutes)
				);

			m_shutDownAt = CTime::GetCurrentTime() + CTimeSpan(0, powerOffHours, powerOffMinutes, 0);
		}
		else
		{
			const CComboBox hoursCombo = GetDlgItem(IDC_CMB_AT_HRS);
			const unsigned short powerOffHours = m_comboIdxToHour[static_cast<unsigned short>(hoursCombo.GetCurSel())];

			const CComboBox minutesCombo = GetDlgItem(IDC_CMB_AT_MINS);
			const unsigned short powerOffMinutes = m_comboIdxToMinutes[static_cast<unsigned short>(minutesCombo.GetCurSel())];

			m_shutDownByZeros = (
				(0 == powerOffHours)
				&& (0 == powerOffMinutes)
				);

			const CTime currentTime = CTime::GetCurrentTime();

			if (powerOffHours < currentTime.GetHour())
			{
				const CTime midNight = CTime(currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay(), 23, 59, 59);
				const CTime tomorrow = midNight + CTimeSpan(0, 0, 0, 1);
				m_shutDownAt = tomorrow + CTimeSpan(0, powerOffHours, powerOffMinutes, 0);
			}
			else
			{
				m_shutDownAt = CTime(
					currentTime.GetYear(),
					currentTime.GetMonth(),
					currentTime.GetDay(),
					powerOffHours,
					powerOffMinutes,
					0
					);
			}
		}

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

	m_isTicking = !m_isTicking;

	return 0;
}

void CMainDlg::CloseDialog(
	const int nVal
	) throw()
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

void CMainDlg::SetTimerTypeMode(
	const TimerType& timerType
	) const throw()
{
	const BOOL shouldSetTimerTypeIn = (TimerTypeIn == timerType ? TRUE : FALSE);

	EnableOrDisableShutdownAtControls(!shouldSetTimerTypeIn);

	EnableOrDisableShutdownInControls(shouldSetTimerTypeIn);
}

void CMainDlg::SetPowerOffTypeMode(
	const PowerOffType& powerOffType
	) throw()
{
	const BOOL isHibernateAllowed = IsPwrHibernateAllowed();
	const BOOL isSuspendAllowed = IsPwrSuspendAllowed();

	EnableOrDisableControl(IDC_RAD_PWR_HIBERNATE, isHibernateAllowed);
	EnableOrDisableControl(IDC_RAD_PWR_SLEEP, isSuspendAllowed);

	int activePowerButtonId = 0;

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
	const std::map<unsigned short, unsigned short>& data,
	const unsigned short selectedItemIdx
	) const throw()
{
	CComboBox someCombo = GetDlgItem(comboId);

	for (const auto& p : data)
	{
		CString comboText;
		comboText.Format(L"%s%d", p.second < 10 ? L"0" : L"", p.second);

		someCombo.AddString(comboText);
	}

	if (data.size() > selectedItemIdx)
	{
		someCombo.SetCurSel(selectedItemIdx);
	}
}

void CMainDlg::ShowCurrentTime(void) const throw()
{
	GetDlgItem(IDC_LBL_TIME).SetWindowTextW(
		CTime::GetCurrentTime().Format(TIMER_MASK)
		);
}

void CMainDlg::ProcessCountDown(void) const throw()
{
	if (!m_isTicking)
	{
		return;
	}

	ShowCountDown();
}

void CMainDlg::ProcessShutdownOption(void) throw()
{
	if (!m_isTicking)
	{
		return;
	}

	PowerOffType currentPowerOffType;

	if (RadioIsChecked(IDC_RAD_PWR_HIBERNATE))
	{
		currentPowerOffType = PowerOffTypeHibernate;
	}
	else if (RadioIsChecked(IDC_RAD_PWR_OFF))
	{
		currentPowerOffType = PowerOffTypeShutdown;
	}
	else if (RadioIsChecked(IDC_RAD_PWR_SLEEP))
	{
		currentPowerOffType = PowerOffTypeSuspend;
	}
	else
	{
		return;
	}

	const bool currentTimeIsCautionTime = (
		(m_shutDownAt - CTime::GetCurrentTime()) == CTimeSpan(0, 0, 1, 0)
		);

	if (currentTimeIsCautionTime)
	{
		//	http://msdn.microsoft.com/en-us/library/windows/desktop/ms645505%28v=vs.85%29.aspx
		//
		CString powerOffTypeMessage;

		switch (currentPowerOffType)
		{
		case PowerOffTypeHibernate:
		{
			powerOffTypeMessage = CResourceManager::LoadStringFromResource(IDS_PWR_HIBERNATE);
		}
			break;

		case PowerOffTypeShutdown:
		{
			powerOffTypeMessage = CResourceManager::LoadStringFromResource(IDS_PWR_SHUTDOWN);
		}
			break;

		case PowerOffTypeSuspend:
		{
			powerOffTypeMessage = CResourceManager::LoadStringFromResource(IDS_PWR_SLEEP);
		}
			break;
		}

		const HWND hWndForegroundWindow = GetForegroundWindow();
		if (
			(NULL != hWndForegroundWindow)
			&& (m_hWnd != hWndForegroundWindow)
			)
		{
			::ShowWindow(
				hWndForegroundWindow,
				SW_MINIMIZE
				);
		}

		ShowWindow(SW_SHOWDEFAULT);

		CStringW cautionShutdownMessage;
		cautionShutdownMessage.Format(
			CResourceManager::LoadStringFromResource(IDS_CAUTION_MESSAGE_BEFORE_SHUTDOWN),
			static_cast<LPCWSTR>(powerOffTypeMessage)
			);

		const int messageBoxResult = MessageBox(
			cautionShutdownMessage,
			CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
			MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND
			);

		if (IDYES == messageBoxResult)
		{
			::SendDlgItemMessage(
				m_hWnd,
				IDC_BTN_START,
				BM_CLICK,
				0,
				0
				);
		}

		return;		//	if (currentTimeIsCautionTime)
	}

	const bool currentTimeIsShutdownTime = (
		m_shutDownAt <= CTime::GetCurrentTime()
		);

	if (
		!currentTimeIsShutdownTime
		&& !m_shutDownByZeros
		)
	{
		return;
	}

	KillTimer(CURRENT_TIME_TIMER_ID);

	if (m_shutDownByZeros)
	{
		const CString confirmShutdownNowMessage = CResourceManager::LoadStringFromResource(
			IDS_CAUTION_MESSAGE_SHUTDOWN_BY_ZEROS
			);

		const int messageBoxResult = MessageBox(
			confirmShutdownNowMessage,
			CResourceManager::LoadStringFromResource(IDR_MAINFRAME),
			MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND
			);

		if (IDNO == messageBoxResult)
		{
			::SendDlgItemMessage(
				m_hWnd,
				IDC_BTN_START,
				BM_CLICK,
				0,
				0
				);

			m_shutDownByZeros = false;
			SetTimer(CURRENT_TIME_TIMER_ID, 1000);

			return;
		}
	}

	switch (currentPowerOffType)
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

	CloseDialog(1);
}

bool CMainDlg::RadioIsChecked(
	const UINT_PTR radioButtonId
	) const throw()
{
	return (
		GetDlgItem(radioButtonId).SendMessageW(BM_GETCHECK, 0, 0L) != 0
		);
}

void CMainDlg::EnableOrDisableShutdownAtControls(
	const BOOL ctrlsAreEnabled
	) const throw()
{
	EnableOrDisableControl(IDC_CMB_AT_HRS, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_LBL_AT, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_CMB_AT_MINS, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_LBL_AT_MINS, ctrlsAreEnabled);
}

void CMainDlg::EnableOrDisableShutdownInControls(
	const BOOL ctrlsAreEnabled
	) const throw()
{
	EnableOrDisableControl(IDC_CMB_IN_HRS, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_LBL_IN_HRS, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_CMB_IN_MINS, ctrlsAreEnabled);
	EnableOrDisableControl(IDC_LBL_IN_MINS, ctrlsAreEnabled);
}

void CMainDlg::ShowCountDown(void) const throw()
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
	) const throw()
{
	GetDlgItem(controlId).EnableWindow(ctrIsEnabled);
}

void CMainDlg::DisableControl(
	const int controlId
	) const throw()
{
	EnableOrDisableControl(controlId, FALSE);
}

void CMainDlg::EnableControl(
	const int controlId
	) const throw()
{
	EnableOrDisableControl(controlId);
}

BOOL CMainDlg::IsWindows8(void) throw()
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

    const DWORD dwPosX = (DWORD)currentWindowPosition.x;
    const DWORD dwPosY = (DWORD)currentWindowPosition.y;

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
