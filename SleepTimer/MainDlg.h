// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ResourceManager.h"

constexpr auto SLEEP_TIMER_REG_KEY = L"Software\\SleepTimer";
constexpr auto SLEEP_TIMER_REG_PARAM_WND_X_POS = L"X";
constexpr auto SLEEP_TIMER_REG_PARAM_WND_Y_POS = L"Y";

class CMainDlg :
	public CDialogImpl<CMainDlg>,
	public CUpdateUI<CMainDlg>,
	public CMessageFilter
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_RAD_OFF_AT, OnTimerModeAtClick)
		COMMAND_ID_HANDLER(IDC_RAD_OFF_IN, OnTimerModeInClick)
		COMMAND_ID_HANDLER(IDC_BTN_START, OnBtnTimerClick)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)


	//////////////////////////////////////
	/*	MESSAGE HANDLERS BLOCK HEADER	*/
	/*									*/
	/*									*/

	LRESULT OnInitDialog(
		const UINT,
		const WPARAM,
		const LPARAM,
		const BOOL&
		) throw();

	LRESULT OnDestroy(
		const UINT,
		const WPARAM,
		const LPARAM,
		const BOOL&
		) throw();

	LRESULT OnTimer(
		const UINT,
		const WPARAM wParam,
		const LPARAM,
		const BOOL&
		) throw();

    LRESULT OnMove(
        const UINT,
        const WPARAM,
        const LPARAM lParam,
        const BOOL&
    ) throw();

	/*									*/
	/*									*/
	/*	MESSAGE HANDLERS BLOCK FOOTER	*/
	//////////////////////////////////////


	//////////////////////////////////////////
	/*	COMMAND-ID HANDLERS BLOCK HEADER	*/
	/*										*/
	/*										*/

	LRESULT OnCancel(
		const WORD,
		const WORD wID,
		const HWND,
		const BOOL&
		) throw();

	LRESULT OnTimerModeAtClick(
		const WORD,
		const WORD,
		const HWND,
		const BOOL&
		) throw();

	LRESULT OnTimerModeInClick(
		const WORD,
		const WORD,
		const HWND,
		const BOOL&
		) throw();

	LRESULT OnBtnTimerClick(
		const WORD,
		const WORD,
		const HWND,
		const BOOL&
		) throw();

	/*										*/
	/*										*/
	/*	COMMAND-ID HANDLERS BLOCK FOOTER	*/
	//////////////////////////////////////////

	void CloseDialog(
		const int nVal
		) throw();

	CMainDlg();

private:

	static const UINT_PTR CURRENT_TIME_TIMER_ID = 15;

	const CString TIMER_MASK = CResourceManager::LoadStringFromResource(IDS_TIMER_MASK);

	enum TimerType
	{
		TimerTypeAt = 0,
		TimerTypeIn = 1
	};

	enum PowerOffType
	{
		PowerOffTypeShutdown = 0,
		PowerOffTypeSuspend = 1,
		PowerOffTypeHibernate = 2
	};

	std::map<unsigned short, unsigned short> m_comboIdxToHour;
	std::map<unsigned short, unsigned short> m_comboIdxToMinutes;

	bool m_isTicking;
	CTime m_shutDownAt;
	bool m_shutDownByZeros;

	void SetTimerTypeMode(
		const TimerType& timerType
		) const throw();

	void SetPowerOffTypeMode(
		const PowerOffType& powerOffType
		) throw();

	void FillCombo(
		const int comboId,
		const std::map<unsigned short, unsigned short>& data,
		const unsigned short selectedItemIdx
		) const throw();

	void ShowCurrentTime(void) const throw();

	void ProcessCountDown(void) const throw();

	void ProcessShutdownOption(void) throw();

	bool RadioIsChecked(
		const UINT_PTR radioButtonId
		) const throw();

	void ShowCountDown(void)const throw();

	void EnableOrDisableShutdownAtControls(
		const BOOL ctrlsAreEnabled
		) const throw();

	void EnableOrDisableShutdownInControls(
		const BOOL ctrlsAreEnabled
		) const throw();

	void EnableOrDisableControl(
		const int controlId,
		const BOOL ctrIsEnabled = 1
		) const throw();

	void DisableControl(
		const int controlId
		) const throw();

	void EnableControl(
		const int controlId
		) const throw();

	static BOOL IsWindows8(void) throw();

    static BOOL GetWindowTopLeftPositionFromRegistry(POINTS& refTopLeftPoint) noexcept;

    static BOOL ReadDwordRegValue(
        LPCWSTR lpValueName,
        DWORD& refValue
    ) noexcept;

    BOOL MoveWindowPositionToSavedPosition(
        POINTS topLeftWindowPointFromRegistry
    ) noexcept;

    BOOL SaveWindowPosition(
        POINTS currentWindowPosition
    ) noexcept;
};
