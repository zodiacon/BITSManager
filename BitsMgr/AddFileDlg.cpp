#include "pch.h"
#include "AddFileDlg.h"

LRESULT CAddFileDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	AdjustOKCancelButtons(IDI_OK2, IDI_CANCEL);
	SetDialogIcon(IDI_FILE);
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAddFileDlg::OnOK(WORD, WORD, HWND, BOOL&) {
	GetDlgItemText(IDC_URL, m_Url);
	GetDlgItemText(IDC_LOCALPATH, m_LocalPath);
	m_Url.Trim();
	m_LocalPath.Trim();

	if (m_Url.IsEmpty() || m_LocalPath.IsEmpty()) {
		AtlMessageBox(m_hWnd, L"Both the remote URL and the local file are required.", IDR_MAINFRAME, MB_ICONWARNING);
		GotoDlgCtrl(GetDlgItem(m_Url.IsEmpty() ? IDC_URL : IDC_LOCALPATH));
		return 0;
	}

	// BITS requires a full local path
	WCHAR full[MAX_PATH];
	if (::GetFullPathName(m_LocalPath, _countof(full), full, nullptr))
		m_LocalPath = full;

	EndDialog(IDOK);
	return 0;
}

LRESULT CAddFileDlg::OnCancel(WORD, WORD, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT CAddFileDlg::OnBrowse(WORD, WORD, HWND, BOOL&) {
	bool download = m_Type == BG_JOB_TYPE_DOWNLOAD;
	CString current;
	GetDlgItemText(IDC_LOCALPATH, current);
	CSimpleFileDialog dlg(!download, nullptr, current.IsEmpty() ? nullptr : (PCWSTR)current,
		OFN_EXPLORER | OFN_ENABLESIZING | (download ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST),
		L"All Files (*.*)\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK)
		SetDlgItemText(IDC_LOCALPATH, dlg.m_szFileName);
	return 0;
}
