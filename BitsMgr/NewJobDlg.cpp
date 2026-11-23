#include "pch.h"
#include "NewJobDlg.h"
#include "View.h"

LRESULT CNewJobDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	AdjustOKCancelButtons(IDI_OK2, IDI_CANCEL);
	SetDialogIcon(IDI_NEW_GEAR);

	m_TypeCombo.Attach(GetDlgItem(IDC_TYPE));
	// upload-reply jobs also need a reply file name; not supported here
	for (auto type : { BG_JOB_TYPE_DOWNLOAD, BG_JOB_TYPE_UPLOAD })
		m_TypeCombo.SetItemData(m_TypeCombo.AddString(CView::JobTypeToString(type)), type);
	m_TypeCombo.SetCurSel(0);

	m_PriorityCombo.Attach(GetDlgItem(IDC_PRIORITY));
	for (auto p : { BG_JOB_PRIORITY_FOREGROUND, BG_JOB_PRIORITY_HIGH, BG_JOB_PRIORITY_NORMAL, BG_JOB_PRIORITY_LOW }) {
		int n = m_PriorityCombo.AddString(CView::JobPriorityToString(p));
		m_PriorityCombo.SetItemData(n, p);
		if (p == BG_JOB_PRIORITY_NORMAL)
			m_PriorityCombo.SetCurSel(n);
	}

	CheckDlgButton(IDC_START, BST_CHECKED);
	CenterWindow(GetParent());

	return TRUE;
}

BG_JOB_TYPE CNewJobDlg::GetSelectedType() const {
	return (BG_JOB_TYPE)m_TypeCombo.GetItemData(m_TypeCombo.GetCurSel());
}

LRESULT CNewJobDlg::OnOK(WORD, WORD, HWND, BOOL&) {
	GetDlgItemText(IDC_DISPLAYNAME, m_Name);
	GetDlgItemText(IDC_URL, m_Url);
	GetDlgItemText(IDC_LOCALPATH, m_LocalPath);
	m_Name.Trim();
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

	if (m_Name.IsEmpty())
		m_Name = m_Url.Mid(m_Url.ReverseFind(L'/') + 1);
	if (m_Name.IsEmpty())
		m_Name = m_Url;

	m_Type = GetSelectedType();
	m_Priority = (BG_JOB_PRIORITY)m_PriorityCombo.GetItemData(m_PriorityCombo.GetCurSel());
	m_Start = IsDlgButtonChecked(IDC_START) == BST_CHECKED;

	EndDialog(IDOK);
	return 0;
}

LRESULT CNewJobDlg::OnCancel(WORD, WORD, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT CNewJobDlg::OnBrowse(WORD, WORD, HWND, BOOL&) {
	// a download writes the local file, an upload reads it
	bool download = GetSelectedType() == BG_JOB_TYPE_DOWNLOAD;
	CString current;
	GetDlgItemText(IDC_LOCALPATH, current);
	CSimpleFileDialog dlg(!download, nullptr, current.IsEmpty() ? nullptr : (PCWSTR)current,
		OFN_EXPLORER | OFN_ENABLESIZING | (download ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST),
		L"All Files (*.*)\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK)
		SetDlgItemText(IDC_LOCALPATH, dlg.m_szFileName);
	return 0;
}
