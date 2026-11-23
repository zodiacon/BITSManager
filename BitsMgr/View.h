// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <VirtualListView.h>
#include "IMainFrame.h"
#include <FrameView.h>
#include <functional>

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

	void DoSort(const SortInfo* si);
	bool OnRightClickList(HWND, int row, int col, POINT const& pt);
	bool OnDoubleClickList(HWND, int row, int col, POINT const& pt);

	static PCWSTR JobTypeToString(BG_JOB_TYPE type);
	static PCWSTR JobStateToString(BG_JOB_STATE state);
	static PCWSTR JobPriorityToString(BG_JOB_PRIORITY priority);
	static CString JobProgressToString(const BG_JOB_PROGRESS& progress);
	// percent transferred, or -1 if the total size is unknown or zero
	static int JobProgressPercent(const BG_JOB_PROGRESS& progress);
	// BITS's own description of an error (it knows the BG_E_* codes), else the system's
	static CString ErrorText(HRESULT hr);

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
		COMMAND_ID_HANDLER(ID_JOB_COMPLETE, OnCompleteJob)
		COMMAND_ID_HANDLER(ID_JOB_DELETECOMPLETEDJOBS, OnDeleteCompletedJobs)
		COMMAND_ID_HANDLER(ID_FILE_NEWJOB, OnNewJob)
		COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnSaveAs)
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

	// acknowledged or canceled: no longer in BITS
	static bool IsFinalState(BG_JOB_STATE state);
	// whether a job command applies to a job in this state
	static bool CanApply(UINT id, BG_JOB_STATE state);
	bool CanApplyToSelection(UINT id) const;
	void EnableMenuItems(HMENU hMenu) const;
	// runs the action on each selected job the command applies to; stops at (and reports) the first failure
	HRESULT ApplyToSelection(UINT id, PCWSTR verb, std::function<HRESULT(IBackgroundCopyJob*)> const& action);
	std::vector<Guid> GetSelectedIds() const;
	void SelectIds(std::vector<Guid> const& ids);

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
	LRESULT OnCompleteJob(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDeleteCompletedJobs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewJob(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void ShowError(PCWSTR what, HRESULT hr);

	CComPtr<IBackgroundCopyManager> m_spMgr;
	std::vector<std::shared_ptr<JobInfo>> m_Jobs;
	std::unordered_map<Guid, std::shared_ptr<JobInfo>> m_JobsMap;
	CUpdateUIBase* m_pUI{ nullptr };
	bool m_Elevated{ false };
	IMainFrame* m_pFrame;
	CListViewCtrl m_List;
};
