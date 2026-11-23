// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "View.h"
#include <sddl.h>
#include <algorithm>
#include "SecurityHelper.h"
#include "SortHelper.h"
#include "JobPropertiesDlg.h"
#include "NewJobDlg.h"
#include "ClipboardHelper.h"
#include "ListViewHelper.h"

void CView::SetUpdateUI(CUpdateUIBase* ui) {
	m_pUI = ui;
}

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

CString CView::GetColumnText(HWND, int row, int col) const {
	CString text;
	auto const item = m_Jobs[row].get();

	switch (col) {
		case 0: return item->DisplayName;
		case 1: return item->StringId;
		case 2: return item->Owner;
		case 3: return JobTypeToString(item->Type);
		case 4: return JobStateToString(item->State);
		case 5: return CTime(item->Times.CreationTime).Format(L"%x %X");
		case 6: return CTime(item->Times.ModificationTime).Format(L"%x %X");
		case 7:
		{
			auto time = item->Times.TransferCompletionTime;
			if (time.dwHighDateTime == 0 && time.dwLowDateTime == 0)
				return L"";
			return CTime(time).Format(L"%x %X");
		}
		case 8: text.Format(L"%u / %u", item->Progress.FilesTransferred, item->Progress.FilesTotal); break;
		case 9: return JobPriorityToString(item->Priority);
		case 10: return JobProgressToString(item->Progress);
	}

	return text;
}

int CView::GetRowImage(HWND, int row, int) const {
	auto& item = m_Jobs[row];
	switch (item->State) {
		case BG_JOB_STATE_TRANSFERRED: return 4;
		case BG_JOB_STATE_ACKNOWLEDGED: return 5;
		case BG_JOB_STATE_SUSPENDED: return 0;
		case BG_JOB_STATE_ERROR: return 6;
		case BG_JOB_STATE_TRANSIENT_ERROR: return 6;
		case BG_JOB_STATE_CANCELLED: return 7;
		case BG_JOB_STATE_TRANSFERRING: return item->Type == BG_JOB_TYPE_DOWNLOAD ? 2 : 3;
	}

	return 1;
}

void CView::DoSort(const SortInfo* si) {
	auto compare = [&](auto& j1, auto& j2) -> bool {
		switch (si->SortColumn) {
			case 0: return SortHelper::Sort(j1->DisplayName, j2->DisplayName, si->SortAscending);
			case 1: return SortHelper::Sort(j1->StringId, j2->StringId, si->SortAscending);
			case 2: return SortHelper::Sort(j1->Owner, j2->Owner, si->SortAscending);
			case 3: return SortHelper::Sort(j1->Type, j2->Type, si->SortAscending);
			case 4: return SortHelper::Sort(j1->State, j2->State, si->SortAscending);
			case 5: return SortHelper::Sort(*(LONGLONG*)&j1->Times.CreationTime, *(LONGLONG*)&j2->Times.CreationTime, si->SortAscending);
			case 6: return SortHelper::Sort(*(LONGLONG*)&j1->Times.ModificationTime, *(LONGLONG*)&j2->Times.ModificationTime, si->SortAscending);
			case 7: return SortHelper::Sort(*(LONGLONG*)&j1->Times.TransferCompletionTime, *(LONGLONG*)&j2->Times.TransferCompletionTime, si->SortAscending);
			case 8: return SortHelper::Sort(j1->Progress.FilesTotal, j2->Progress.FilesTotal, si->SortAscending);
			case 9: return SortHelper::Sort(j1->Priority, j2->Priority, si->SortAscending);
			// unknown sizes sort as -1, keeping the ordering consistent for std::sort
			case 10: return SortHelper::Sort(JobProgressPercent(j1->Progress), JobProgressPercent(j2->Progress), si->SortAscending);
		}
		return false;
	};

	std::sort(m_Jobs.begin(), m_Jobs.end(), compare);
}

bool CView::OnRightClickList(HWND, int row, int col, POINT const& pt) {
	if (row < 0)
		return 0;

	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	auto popup = menu.GetSubMenu(0);
	EnableMenuItems(popup);
	Frame()->TrackPopupMenu(popup);

	return true;
}

bool CView::OnDoubleClickList(HWND, int row, int col, POINT const& pt) {
	if (row < 0)
		return false;

	ShowProperties(row);
	return true;
}

PCWSTR CView::JobTypeToString(BG_JOB_TYPE type) {
	switch (type) {
		case BG_JOB_TYPE_DOWNLOAD: return L"Download";
		case BG_JOB_TYPE_UPLOAD: return L"Upload";
		case BG_JOB_TYPE_UPLOAD_REPLY: return L"Upload Reply";
	}
	return L"Unknown";
}

PCWSTR CView::JobStateToString(BG_JOB_STATE state) {
	switch (state) {
		case BG_JOB_STATE_CONNECTING: return L"Connecting";
		case BG_JOB_STATE_QUEUED: return L"Queued";
		case BG_JOB_STATE_CANCELLED: return L"Canceled";
		case BG_JOB_STATE_ERROR: return L"Error";
		case BG_JOB_STATE_TRANSFERRED: return L"Transferred";
		case BG_JOB_STATE_TRANSFERRING: return L"Transferring";
		case BG_JOB_STATE_TRANSIENT_ERROR: return L"Transient Error";
		case BG_JOB_STATE_ACKNOWLEDGED: return L"Acknowledged";
		case BG_JOB_STATE_SUSPENDED: return L"Suspended";
	}

	return L"Unknown";
}

PCWSTR CView::JobPriorityToString(BG_JOB_PRIORITY priority) {
	switch (priority) {
		case BG_JOB_PRIORITY_HIGH: return L"High (1)";
		case BG_JOB_PRIORITY_NORMAL: return L"Normal (2)";
		case BG_JOB_PRIORITY_LOW: return L"Low (3)";
		case BG_JOB_PRIORITY_FOREGROUND: return L"Foreground (0)";
	}
	return L"";
}

int CView::JobProgressPercent(const BG_JOB_PROGRESS& progress) {
	if (progress.BytesTotal == 0 || progress.BytesTotal == BG_SIZE_UNKNOWN)
		return -1;
	return (int)(progress.BytesTransferred * 100 / progress.BytesTotal);
}

CString CView::JobProgressToString(const BG_JOB_PROGRESS& progress) {
	CString text;
	if (progress.FilesTotal == 0)
		return text;

	if (auto percent = JobProgressPercent(progress); percent >= 0)
		text.Format(L"%d %% (%llu / %llu)", percent, progress.BytesTransferred, progress.BytesTotal);
	else if (progress.BytesTotal == 0)
		text = L"(empty)";
	else
		text.Format(L"%llu / ?", progress.BytesTransferred);
	return text;
}

CString CView::ErrorText(HRESULT hr) {
	CString text;
	PWSTR desc = nullptr;
	CComPtr<IBackgroundCopyManager> spMgr;
	spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (spMgr && SUCCEEDED(spMgr->GetErrorDescription(hr, LANGIDFROMLCID(::GetThreadLocale()), &desc)) && desc) {
		text = desc;
		::CoTaskMemFree(desc);
	}
	else {
		PWSTR msg = nullptr;
		if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr, 0, (PWSTR)&msg, 0, nullptr)) {
			text = msg;
			::LocalFree(msg);
		}
	}
	text.Trim();
	CString code;
	code.Format(L" (0x%08X)", hr);
	return text + code;
}

void CView::ShowError(PCWSTR what, HRESULT hr) {
	AtlMessageBox(*this, (PCWSTR)(CString(what) + L"\n\n" + ErrorText(hr)), IDR_MAINFRAME, MB_ICONERROR);
}

bool CView::IsFinalState(BG_JOB_STATE state) {
	return state == BG_JOB_STATE_ACKNOWLEDGED || state == BG_JOB_STATE_CANCELLED;
}

bool CView::CanApply(UINT id, BG_JOB_STATE state) {
	switch (id) {
		case ID_JOB_CANCEL: return !IsFinalState(state);
		case ID_JOB_PAUSE:
			return state == BG_JOB_STATE_QUEUED || state == BG_JOB_STATE_CONNECTING ||
				state == BG_JOB_STATE_TRANSFERRING || state == BG_JOB_STATE_TRANSIENT_ERROR;
		// Resume also retries a job in the error state
		case ID_JOB_RESUME: return state == BG_JOB_STATE_SUSPENDED || state == BG_JOB_STATE_ERROR;
		case ID_JOB_COMPLETE: return state == BG_JOB_STATE_TRANSFERRED;
	}
	return true;
}

bool CView::CanApplyToSelection(UINT id) const {
	for (int i = -1; (i = m_List.GetNextItem(i, LVIS_SELECTED)) >= 0; )
		if (CanApply(id, m_Jobs[i]->State))
			return true;
	return false;
}

void CView::UpdateUI() {
	for (auto id : { ID_JOB_CANCEL, ID_JOB_PAUSE, ID_JOB_RESUME, ID_JOB_COMPLETE })
		m_pUI->UIEnable(id, CanApplyToSelection(id));
	auto count = m_List.GetSelectedCount();
	m_pUI->UIEnable(ID_JOB_PROPERTIES, count == 1);
	m_pUI->UIEnable(ID_EDIT_COPYURL, count == 1);
}

void CView::EnableMenuItems(HMENU hMenu) const {
	CMenuHandle menu(hMenu);
	for (auto id : { ID_JOB_CANCEL, ID_JOB_PAUSE, ID_JOB_RESUME, ID_JOB_COMPLETE })
		menu.EnableMenuItem(id, MF_BYCOMMAND | (CanApplyToSelection(id) ? MF_ENABLED : MF_GRAYED));
	menu.EnableMenuItem(ID_JOB_PROPERTIES, MF_BYCOMMAND | (m_List.GetSelectedCount() == 1 ? MF_ENABLED : MF_GRAYED));
}

HRESULT CView::ApplyToSelection(UINT id, PCWSTR verb, std::function<HRESULT(IBackgroundCopyJob*)> const& action) {
	HRESULT result = S_OK;
	for (int i = -1; (i = m_List.GetNextItem(i, LVIS_SELECTED)) >= 0; ) {
		auto& item = m_Jobs[i];
		if (!CanApply(id, item->State))
			continue;
		auto hr = action(item->spJob);
		if (FAILED(hr)) {
			ShowError(CString(L"Failed to ") + verb + L" job \"" + item->DisplayName + L"\".", hr);
			result = hr;
			break;
		}
	}
	Refresh();
	UpdateUI();
	return result;
}

std::vector<Guid> CView::GetSelectedIds() const {
	std::vector<Guid> ids;
	for (int i = -1; (i = m_List.GetNextItem(i, LVIS_SELECTED)) >= 0; )
		ids.push_back(m_Jobs[i]->Id);
	return ids;
}

void CView::SelectIds(std::vector<Guid> const& ids) {
	m_List.SetItemState(-1, 0, LVIS_SELECTED);
	for (size_t i = 0; i < m_Jobs.size(); i++)
		if (std::find(ids.begin(), ids.end(), m_Jobs[i]->Id) != ids.end())
			m_List.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
}

void CView::ShowProperties(int index) {
	CJobPropertiesDlg dlg(m_Jobs[index]->spJob);
	dlg.DoModal();
}

void CView::Refresh() {
	// acknowledged and canceled jobs are gone from BITS and can't change: they stay listed
	// (until Delete Completed Jobs) without being queried
	for (auto& job : m_Jobs)
		if (!IsFinalState(job->State))
			UpdateBitsJob(job.get());

	if (m_spMgr) {
		bool added = false;
		CComPtr<IEnumBackgroundCopyJobs> spJobs;
		m_spMgr->EnumJobsW(m_Elevated ? BG_JOB_ENUM_ALL_USERS : 0, &spJobs);
		if (spJobs) {
			ULONG count;
			spJobs->GetCount(&count);
			for (ULONG i = 0; i < count; i++) {
				CComPtr<IBackgroundCopyJob> spJob;
				spJobs->Next(1, &spJob, nullptr);
				if (spJob) {
					Guid id;
					spJob->GetId(&id);
					if (auto it = m_JobsMap.find(id); it == m_JobsMap.end()) {
						AddBitsJob(spJob);
						added = true;
					}
				}
			}
		}
		m_List.SetItemCountEx((int)m_Jobs.size(), LVSICF_NOSCROLL);
		if (added) {
			// keep the current sort order; the selection is by index, so carry it over by job
			auto selected = GetSelectedIds();
			Sort(m_List);
			SelectIds(selected);
		}
	}
	UpdateUI();
}

bool CView::AddBitsJob(IBackgroundCopyJob* pJob) {
	auto info = std::make_shared<JobInfo>();
	info->spJob = pJob;

	pJob->GetId(&info->Id);
	::StringFromGUID2(info->Id, info->StringId.GetBufferSetLength(64), 64);

	PWSTR text;
	if (SUCCEEDED(pJob->GetDescription(&text))) {
		info->Description = text;
		::CoTaskMemFree(text);
	}

	if (SUCCEEDED(pJob->GetOwner(&text))) {
		// the SID itself if it can't be resolved (e.g. a deleted account)
		info->Owner = text;
		PSID sid;
		if (::ConvertStringSidToSid(text, &sid)) {
			DWORD lname = 0, ldomain = 0;
			SID_NAME_USE use;
			::LookupAccountSid(nullptr, sid, nullptr, &lname, nullptr, &ldomain, &use);
			if (lname > 0) {
				CString name, domain;
				if (::LookupAccountSid(nullptr, sid, name.GetBufferSetLength(lname), &lname,
					domain.GetBufferSetLength(ldomain), &ldomain, &use)) {
					name.ReleaseBuffer();
					domain.ReleaseBuffer();
					info->Owner = domain.IsEmpty() ? name : domain + L"\\" + name;
				}
			}
			::LocalFree(sid);
		}
		::CoTaskMemFree(text);
	}
	pJob->GetType(&info->Type);

	UpdateBitsJob(info.get());

	m_Jobs.push_back(info);
	m_JobsMap.insert({ info->Id, info });

	return true;
}

bool CView::UpdateBitsJob(JobInfo* info) {
	auto pJob = info->spJob;
	PWSTR text;
	if (SUCCEEDED(pJob->GetDisplayName(&text))) {
		info->DisplayName = text;
		::CoTaskMemFree(text);
	}
	pJob->GetState(&info->State);

	pJob->GetTimes(&info->Times);
	pJob->GetPriority(&info->Priority);
	pJob->GetProgress(&info->Progress);

	return true;
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	SetStatic();
	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_REPORT | LVS_OWNERDATA);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	auto hr = m_spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (FAILED(hr)) {
		ShowError(L"Failed to connect to the BITS service.", hr);
		return -1;
	}
	m_Elevated = SecurityHelper::IsRunningElevated();

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"Display Name", LVCFMT_LEFT, 350);
	cm->AddColumn(L"GUID", LVCFMT_LEFT, 260, ColumnFlags::Numeric | ColumnFlags::Const);
	cm->AddColumn(L"Owner", LVCFMT_LEFT, 200);
	cm->AddColumn(L"Type", LVCFMT_LEFT, 70);
	cm->AddColumn(L"State", LVCFMT_LEFT, 100);
	cm->AddColumn(L"Created", LVCFMT_LEFT, 120);
	cm->AddColumn(L"Modified", LVCFMT_LEFT, 120);
	cm->AddColumn(L"Completed", LVCFMT_LEFT, 120);
	cm->AddColumn(L"Files", LVCFMT_RIGHT, 50);
	cm->AddColumn(L"Priority", LVCFMT_LEFT, 110);
	cm->AddColumn(L"Progress", LVCFMT_LEFT, 130);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32, 8, 4);
	UINT icons[] = {
		IDI_SUSPENDED, IDI_GEAR, IDI_DOWNLOAD, IDI_UPLOAD, IDI_OK, IDI_OK2, IDI_STOP2,
		IDI_CANCEL,
	};
	for (auto icon : icons)
		images.AddIcon(AtlLoadIconImage(icon, 0, 16, 16));
	m_List.SetImageList(images, LVSIL_SMALL);

	Refresh();

	SetTimer(1, 1000, nullptr);
	UpdateUI();

	return 0;
}

LRESULT CView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1)
		Refresh();
	return 0;
}

LRESULT CView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());

	return 0;
}

LRESULT CView::OnCancelJob(WORD, WORD, HWND, BOOL&) {
	int count = 0;
	for (int i = -1; (i = m_List.GetNextItem(i, LVIS_SELECTED)) >= 0; )
		if (CanApply(ID_JOB_CANCEL, m_Jobs[i]->State))
			count++;
	if (count == 0)
		return 0;

	CString text;
	text.Format(L"Cancel %d job(s)?", count);
	if (IDNO == AtlMessageBox(*this, (PCWSTR)text, IDR_MAINFRAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
		return 0;

	ApplyToSelection(ID_JOB_CANCEL, L"cancel", [](auto job) { return job->Cancel(); });
	return 0;
}

LRESULT CView::OnSuspendJob(WORD, WORD, HWND, BOOL&) {
	ApplyToSelection(ID_JOB_PAUSE, L"pause", [](auto job) { return job->Suspend(); });
	return 0;
}

LRESULT CView::OnResumeJob(WORD, WORD, HWND, BOOL&) {
	ApplyToSelection(ID_JOB_RESUME, L"resume", [](auto job) { return job->Resume(); });
	return 0;
}

LRESULT CView::OnItemChanged(int, LPNMHDR, BOOL&) {
	UpdateUI();
	return 0;
}

LRESULT CView::OnJobProps(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_List.GetSelectedCount() == 1);
	auto selected = m_List.GetNextItem(-1, LVIS_SELECTED);
	ShowProperties(selected);

	return 0;
}

LRESULT CView::OnEditCopy(WORD, WORD, HWND, BOOL&) const {
	auto count = m_List.GetSelectedCount();
	CString text;
	if (count == 0 || count == m_List.GetItemCount()) {
		for (int i = 0; i < m_List.GetItemCount(); i++)
			text += ListViewHelper::GetRowAsString(m_List, i);
	}
	else {
		for(int i = -1;;) {
			i = m_List.GetNextItem(i, LVIS_SELECTED);
			if (i < 0)
				break;
			text += ListViewHelper::GetRowAsString(m_List, i);
		}
	}
	ClipboardHelper::CopyText(*this, text);

	return 0;
}

LRESULT CView::OnCompleteJob(WORD, WORD, HWND, BOOL&) {
	// Complete() makes the transferred files available under their final names and removes the job from BITS
	for (int i = -1; (i = m_List.GetNextItem(i, LVIS_SELECTED)) >= 0; ) {
		auto& item = m_Jobs[i];
		if (!CanApply(ID_JOB_COMPLETE, item->State))
			continue;
		auto hr = item->spJob->Complete();
		if (FAILED(hr)) {
			ShowError(L"Failed to complete job \"" + item->DisplayName + L"\".", hr);
			break;
		}
		if (hr == BG_S_PARTIAL_COMPLETE || hr == BG_S_UNABLE_TO_DELETE_FILES)
			AtlMessageBox(*this, (PCWSTR)(L"Job \"" + item->DisplayName + L"\" completed with a warning.\n\n" + ErrorText(hr)),
				IDR_MAINFRAME, MB_ICONWARNING);
	}
	Refresh();
	UpdateUI();
	return 0;
}

LRESULT CView::OnDeleteCompletedJobs(WORD, WORD, HWND, BOOL&) {
	// acknowledged and canceled jobs are gone from BITS; the list only keeps them until removed here
	auto done = [](auto const& job) { return IsFinalState(job->State); };
	for (auto& job : m_Jobs)
		if (done(job))
			m_JobsMap.erase(job->Id);
	std::erase_if(m_Jobs, done);

	// selection is by index, which no longer matches
	m_List.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_List.SetItemCountEx((int)m_Jobs.size(), LVSICF_NOSCROLL);
	UpdateUI();
	return 0;
}

LRESULT CView::OnNewJob(WORD, WORD, HWND, BOOL&) {
	if (!m_spMgr)
		return 0;

	CNewJobDlg dlg;
	if (dlg.DoModal() != IDOK)
		return 0;

	GUID id;
	CComPtr<IBackgroundCopyJob> spJob;
	auto hr = m_spMgr->CreateJob(dlg.GetDisplayName(), dlg.GetType(), &id, &spJob);
	if (FAILED(hr)) {
		ShowError(L"Failed to create job.", hr);
		return 0;
	}

	hr = spJob->AddFile(dlg.GetUrl(), dlg.GetLocalPath());
	if (FAILED(hr)) {
		// don't leave an empty job behind
		spJob->Cancel();
		ShowError(L"Failed to add the file to the job.", hr);
		return 0;
	}
	spJob->SetPriority(dlg.GetPriority());

	if (dlg.StartNow()) {
		hr = spJob->Resume();
		if (FAILED(hr))
			ShowError(L"The job was created but failed to start.", hr);
	}

	if (m_JobsMap.find(static_cast<Guid const&>(id)) == m_JobsMap.end())
		AddBitsJob(spJob);
	m_List.SetItemCountEx((int)m_Jobs.size(), LVSICF_NOSCROLL);

	// select the new job
	m_List.SetItemState(-1, 0, LVIS_SELECTED);
	auto it = std::find_if(m_Jobs.begin(), m_Jobs.end(), [&](auto const& j) { return ::IsEqualGUID(j->Id, id); });
	if (it != m_Jobs.end()) {
		int index = (int)(it - m_Jobs.begin());
		m_List.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_List.EnsureVisible(index, FALSE);
	}
	return 0;
}

LRESULT CView::OnSaveAs(WORD, WORD, HWND, BOOL&) {
	auto path = ListViewHelper::PromptForCsvFile(*this, L"BitsJobs.csv");
	if (path.IsEmpty())
		return 0;

	if (!ListViewHelper::SaveAsCsv(m_List, path))
		ShowError(L"Failed to save the job list.", HRESULT_FROM_WIN32(::GetLastError()));
	return 0;
}

