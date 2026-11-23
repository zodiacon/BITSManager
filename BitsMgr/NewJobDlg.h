#pragma once

#include "resource.h"
#include <DialogHelper.h>

//
// collects what's needed to create a BITS job with one file; the caller creates the job
//
class CNewJobDlg :
	public CDialogImpl<CNewJobDlg>,
	public CDialogHelper<CNewJobDlg> {
public:
	enum { IDD = IDD_NEWJOB };

	CString const& GetDisplayName() const { return m_Name; }
	CString const& GetUrl() const { return m_Url; }
	CString const& GetLocalPath() const { return m_LocalPath; }
	BG_JOB_TYPE GetType() const { return m_Type; }
	BG_JOB_PRIORITY GetPriority() const { return m_Priority; }
	bool StartNow() const { return m_Start; }

	BEGIN_MSG_MAP(CNewJobDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		CHAIN_MSG_MAP(CDialogHelper<CNewJobDlg>)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	BG_JOB_TYPE GetSelectedType() const;

	CComboBox m_TypeCombo, m_PriorityCombo;
	CString m_Name, m_Url, m_LocalPath;
	BG_JOB_TYPE m_Type{ BG_JOB_TYPE_DOWNLOAD };
	BG_JOB_PRIORITY m_Priority{ BG_JOB_PRIORITY_NORMAL };
	bool m_Start{ true };
};
