#pragma once

#include "resource.h"

class CJobPropertiesDlg : 
	public CDialogImpl<CJobPropertiesDlg>,
	public CDynamicDialogLayout<CJobPropertiesDlg> {
public:
	enum { IDD = IDD_JOBPROPS };

	explicit CJobPropertiesDlg(IBackgroundCopyJob* pJob);

	BEGIN_MSG_MAP(CJobPropertiesDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnRightClick)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		CHAIN_MSG_MAP(CDynamicDialogLayout<CJobPropertiesDlg>)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	void InitJob();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CListViewCtrl m_List;
	CComboBox m_Priority;
	IBackgroundCopyJob* m_pJob;
};

