#pragma once

#include "resource.h"
#include <DialogHelper.h>

//
// a remote URL and a local file to add to an existing job; the caller adds them
//
class CAddFileDlg :
	public CDialogImpl<CAddFileDlg>,
	public CDialogHelper<CAddFileDlg> {
public:
	enum { IDD = IDD_ADDFILE };

	// the job's type decides whether browsing picks a file to write (download) or to read (upload)
	explicit CAddFileDlg(BG_JOB_TYPE type) : m_Type(type) {}

	CString const& GetUrl() const { return m_Url; }
	CString const& GetLocalPath() const { return m_LocalPath; }

	BEGIN_MSG_MAP(CAddFileDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		CHAIN_MSG_MAP(CDialogHelper<CAddFileDlg>)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	BG_JOB_TYPE m_Type;
	CString m_Url, m_LocalPath;
};
