#pragma once

#include "resource.h"
#include <DialogHelper.h>

//
// a job's settings: retry delay, no-progress timeout, notification flags, proxy, HTTP headers and security
// flags, and credentials. OK sets only what changed; credentials can't be read, so they're set only if a user
// name is typed, and Remove removes the ones for the chosen target and scheme at once
//
class CJobSettingsDlg :
	public CDialogImpl<CJobSettingsDlg>,
	public CDialogHelper<CJobSettingsDlg> {
public:
	enum { IDD = IDD_JOBSETTINGS };

	explicit CJobSettingsDlg(IBackgroundCopyJob* pJob) : m_pJob(pJob) {}

	BEGIN_MSG_MAP(CJobSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_CRED_REMOVE, OnRemoveCredentials)
		COMMAND_HANDLER(IDC_PROXYUSAGE, CBN_SELCHANGE, OnProxyUsageChanged)
		CHAIN_MSG_MAP(CDialogHelper<CJobSettingsDlg>)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveCredentials(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProxyUsageChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void LoadSettings();
	// false (after showing the error) if BITS refused a change
	bool SaveSettings();
	bool Check(HRESULT hr, PCWSTR what);
	void EnableProxyLists();
	ULONG GetNotifyFlagsFromUI() const;
	DWORD GetSecurityFlagsFromUI() const;
	static CString GetCombo(CComboBox const& cb);

	IBackgroundCopyJob* m_pJob;
	CComQIPtr<IBackgroundCopyJob2> m_spJob2;
	CComQIPtr<IBackgroundCopyJobHttpOptions> m_spHttp;
	CComboBox m_ProxyUsage, m_Redirect, m_CredTarget, m_CredScheme;

	// as loaded, to set only what changed
	ULONG m_RetryDelay{ 0 }, m_NoProgress{ 0 }, m_NotifyFlags{ 0 };
	BG_JOB_PROXY_USAGE m_Usage{ BG_JOB_PROXY_USAGE_PRECONFIG };
	CString m_ProxyList, m_ProxyBypass, m_Headers;
	ULONG m_SecurityFlags{ 0 };
};
