// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ResourceManager.h"

class CMainDlg :
    public CDialogImpl<CMainDlg>,
    public CUpdateUI<CMainDlg>,
    public CMessageFilter
{
public:
    enum { IDD = IDD_MAINDLG };

    BOOL PreTranslateMessage(MSG* pMsg) override;
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
        );

    LRESULT OnDestroy(
        const UINT,
        const WPARAM,
        const LPARAM,
        const BOOL&
        );

    LRESULT OnTimer(
        const UINT,
        const WPARAM wParam,
        const LPARAM,
        const BOOL&
        );

    LRESULT OnMove(
        const UINT,
        const WPARAM,
        const LPARAM lParam,
        const BOOL&
    ) noexcept;

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
        const WORD wId,
        HWND,
        const BOOL&
        ) noexcept;

    LRESULT OnTimerModeAtClick(
        const WORD,
        const WORD,
        const HWND,
        const BOOL&
        ) noexcept;

    LRESULT OnTimerModeInClick(
        const WORD,
        const WORD,
        const HWND,
        const BOOL&
        ) noexcept;

    LRESULT OnBtnTimerClick(
        const WORD,
        const WORD,
        const HWND,
        const BOOL&
        );

    /*										*/
    /*										*/
    /*	COMMAND-ID HANDLERS BLOCK FOOTER	*/
    //////////////////////////////////////////

    void CloseDialog(
        const int nVal
        ) noexcept;

    CMainDlg();

private:
    BOOL m_isHibernateAllowed = false;
    BOOL m_isSuspendAllowed = false;
    BOOL m_isShutdownAllowed = false;

    static const UINT_PTR SHUTDOWN_TIMER_ID = 15;
    static const UINT_PTR CURRENT_TIME_TIMER_ID = 30;

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

    bool m_isTicking = false;

    CTime m_shutDownAt;

    byte m_shutDownAtHours = 0;
    byte m_shutDownAtMinutes = 0;

    int m_shutDownInSecondsCountdown = 0;

    bool m_shutDownByZeros = false;
    TimerType m_timerType;

    bool m_isCautionMessageAlreadyShown = false;

    void SetTimerTypeMode(
        const TimerType& timerType
        ) noexcept;

    void SetPowerOffTypeMode(
        const PowerOffType& powerOffType
        ) noexcept;

    void FillCombo(
        const int comboId,
        const std::vector<unsigned short>& comboValues,
        const unsigned short selectedItemIdx
        ) const;

    void FillHoursCombo(
        const int comboId,
        const unsigned short selectedIndex
    ) const;

    void FillMinutesCombo(const int comboId) const;

    void ShowCurrentTime() const noexcept;

    void ProcessCountDown() noexcept;

    void ProcessShutdownOption();

    bool RadioIsChecked(
        const UINT_PTR radioButtonId
        ) const noexcept;

    void ShowCountDown()const noexcept;

    void EnableOrDisableShutdownAtControls(
        const BOOL ctrlsAreEnabled
        ) const noexcept;

    void EnableOrDisableShutdownInControls(
        const BOOL ctrlsAreEnabled
        ) const noexcept;

    void EnableOrDisableControl(
        const int controlId,
        const BOOL ctrIsEnabled = 1
        ) const noexcept;

    void DisableControl(
        const int controlId
        ) const noexcept;

    void EnableControl(
        const int controlId
        ) const noexcept;

    BOOL MoveWindowPositionToSavedPosition(
        POINTS topLeftWindowPointFromRegistry
    ) noexcept;

    byte GetComboBoxSelectedItemData(const int comboBoxId) const;
    void PowerOffAndExit(const PowerOffType powerOffType);
    int GetPowerOffType() const;
    static CString GetPowerOffTypeDescription(const PowerOffType powerOffType);

    void EnableUiAndStopCountdown() const;
    void DisableUiAndStartCountdown() const;

    void StartShutdownTimer() noexcept;
    void StopShutdownTimer() noexcept;

    void StartCurrentTimeTimer() noexcept;
    void StopCurrentTimeTimer() noexcept;

    void ProcessShutdownByZerosCase() noexcept;
};
