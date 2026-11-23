#pragma once

#include "resource.h"
#include <DialogHelper.h>

class CJobPropertiesDlg : 
	public CDialogImpl<CJobPropertiesDlg>,
	public CDialogHelper<CJobPropertiesDlg>,
	public CDynamicDialogLayout<CJobPropertiesDlg> {
public:
	enum { IDD = IDD_JOBPROPS };

	explicit CJobPropertiesDlg(IBackgroundCopyJob* pJob);

	BEGIN_MSG_MAP(CJobPropertiesDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnRightClick)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		COMMAND_ID_HANDLER(ID_EDIT_COPYURL, OnEditCopyURL)
		COMMAND_ID_HANDLER(IDC_ADDFILE, OnAddFile)
		COMMAND_ID_HANDLER(IDC_SETTINGS, OnSettings)
		CHAIN_MSG_MAP(CDialogHelper<CJobPropertiesDlg>)
		CHAIN_MSG_MAP(CDynamicDialogLayout<CJobPropertiesDlg>)
	ALT_MSG_MAP(1)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	void InitJob();
	// state, progress, error and the files' progress, every second
	void UpdateLive();
	void SetTextIfChanged(UINT id, PCWSTR text);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	// sets what was changed; false (after showing the error) if BITS refused something
	bool SaveChanges();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CListViewCtrl m_List;
	CComboBox m_Priority;
	IBackgroundCopyJob* m_pJob;
	// the values as shown when the dialog opened
	CString m_Name, m_Description, m_Program, m_Params;
	BG_JOB_PRIORITY m_OrgPriority{ BG_JOB_PRIORITY_NORMAL };
};

