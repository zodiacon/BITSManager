// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <VirtualListView.h>
#include "IMainFrame.h"
#include <FrameView.h>

struct Guid : GUID {
	bool operator==(Guid const& other) const {
		return ::IsEqualGUID(*this, other);
	}
};

template<>
struct std::hash<Guid> {
	size_t operator()(Guid const& guid) const {
		return guid.Data1 ^ guid.Data2 ^ (guid.Data3 << 16LL);
	}
};

class CView : 
	public CFrameView<CView, IMainFrame>,
	public CVirtualListView<CView> {
public:
	DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

	using CFrameView::CFrameView;

	void SetUpdateUI(CUpdateUIBase* ui);
	BOOL PreTranslateMessage(MSG* pMsg);

	CString GetColumnText(HWND, int row, int col) const;
	int GetRowImage(HWND, int row, int) const;

	void Sort(const SortInfo* si);
	bool OnRightClickList(HWND, int row, int col, POINT& pt);
	bool OnDoubleClickList(HWND, int row, int col, POINT& pt);

	static PCWSTR JobTypeToString(BG_JOB_TYPE type);
	static PCWSTR JobStateToString(BG_JOB_STATE state);
	static PCWSTR JobPriorityToString(BG_JOB_PRIORITY priority);
	static CString JobProgressToString(const BG_JOB_PROGRESS& progress);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		CHAIN_MSG_MAP(CVirtualListView<CView>)
		CHAIN_MSG_MAP(BaseFrame)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnRefresh)
		COMMAND_ID_HANDLER(ID_JOB_CANCEL, OnCancelJob)
		COMMAND_ID_HANDLER(ID_JOB_PAUSE, OnSuspendJob)
		COMMAND_ID_HANDLER(ID_JOB_RESUME, OnResumeJob)
		COMMAND_ID_HANDLER(ID_JOB_PROPERTIES, OnJobProps)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	struct JobInfo {
		Guid Id;
		CString StringId;
		CString DisplayName;
		CString Description;
		CString Owner;
		BG_JOB_STATE State;
		BG_JOB_TYPE Type;
		BG_JOB_TIMES Times;
		BG_JOB_PRIORITY Priority;
		BG_JOB_PROGRESS Progress;
		CComPtr<IBackgroundCopyJob> spJob;
	};

	void UpdateUI();
	void ShowProperties(int index);
	void Refresh();
	bool AddBitsJob(IBackgroundCopyJob* pJob);
	bool UpdateBitsJob(JobInfo* info);

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancelJob(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSuspendJob(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnResumeJob(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnJobProps(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) const;

	CComPtr<IBackgroundCopyManager> m_spMgr;
	std::vector<std::shared_ptr<JobInfo>> m_Jobs;
	std::unordered_map<Guid, std::shared_ptr<JobInfo>> m_JobsMap;
	CUpdateUIBase* m_pUI{ nullptr };
	IMainFrame* m_pFrame;
	CListViewCtrl m_List;
};
