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
#include "ClipboardHelper.h"

CView::CView(IMainFrame* frame) : m_pFrame(frame) {
}

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
			auto time = item->Times.TransferCompletionTime;
			if (time.dwHighDateTime == 0 && time.dwLowDateTime == 0)
				return L"";
			return CTime(time).Format(L"%x %X");
		case 8: text.Format(L"%u / %u", item->Progress.FilesTransferred, item->Progress.FilesTotal); break;
		case 9: return JobPriorityToString(item->Priority);
		case 10: return JobProgressToString(item->Progress);
	}

	return text;
}

int CView::GetRowImage(int row) const {
	auto& item = m_Jobs[row];
	switch (item->State) {
		case BG_JOB_STATE_TRANSFERRED: return 4;
		case BG_JOB_STATE_ACKNOWLEDGED: return 5;
		case BG_JOB_STATE_SUSPENDED: return 0;
		case BG_JOB_STATE_ERROR: return 6;
		case BG_JOB_STATE_CANCELLED: return 7;
		case BG_JOB_STATE_TRANSFERRING: return item->Type == BG_JOB_TYPE_DOWNLOAD ? 2 : 3;
	}

	return 1;
}

void CView::DoSort(const SortInfo* si) {
	auto compare = [&](auto& j1, auto& j2) -> bool {
		switch (si->SortColumn) {
			case 0: return SortHelper::SortStrings(j1->DisplayName, j2->DisplayName, si->SortAscending);
			case 1: return SortHelper::SortStrings(j1->StringId, j2->StringId, si->SortAscending);
			case 2: return SortHelper::SortStrings(j1->Owner, j2->Owner, si->SortAscending);
			case 3: return SortHelper::SortNumbers(j1->Type, j2->Type, si->SortAscending);
			case 4: return SortHelper::SortNumbers(j1->State, j2->State, si->SortAscending);
			case 5: return SortHelper::SortNumbers(*(LONGLONG*)&j1->Times.CreationTime, *(LONGLONG*)&j2->Times.CreationTime, si->SortAscending);
			case 6: return SortHelper::SortNumbers(*(LONGLONG*)&j1->Times.ModificationTime, *(LONGLONG*)&j2->Times.ModificationTime, si->SortAscending);
			case 7: return SortHelper::SortNumbers(*(LONGLONG*)&j1->Times.TransferCompletionTime, *(LONGLONG*)&j2->Times.TransferCompletionTime, si->SortAscending);
			case 8: return SortHelper::SortNumbers(j1->Progress.FilesTotal, j2->Progress.FilesTotal, si->SortAscending);
			case 9: return SortHelper::SortNumbers(j1->Priority, j2->Priority, si->SortAscending);
			case 10: 
				auto& progress1 = j1->Progress;
				auto& progress2 = j2->Progress;
				if (progress1.BytesTotal == 0 || progress2.BytesTotal == 0)
					return false;
				return SortHelper::SortNumbers(progress1.BytesTransferred * 100 / progress1.BytesTotal, progress2.BytesTransferred * 100 / progress2.BytesTotal, si->SortAscending);
		}
		return false;
	};

	std::sort(m_Jobs.begin(), m_Jobs.end(), compare);
}

bool CView::OnRightClickList(int row, int col, POINT& pt) {
	if (row < 0)
		return 0;

	auto& item = m_Jobs[row];
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	m_pFrame->TrackPopupMenu(menu.GetSubMenu(0));

	return true;
}

bool CView::OnDoubleClickList(int row, int col, POINT& pt) {
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

CString CView::JobProgressToString(const BG_JOB_PROGRESS& progress) {
	CString text;
	if(progress.FilesTotal > 0 && progress.BytesTotal != (UINT64)-1)
		text.Format(L"%u %% (%llu / %llu)", 
			progress.BytesTransferred * 100 / progress.BytesTotal, progress.BytesTransferred, progress.BytesTotal);
	return text;
}

void CView::UpdateUI() {
	auto count = GetSelectedCount();
	JobInfo* info{ nullptr };
	if (count > 0) {
		info = m_Jobs[GetNextItem(-1, LVIS_SELECTED)].get();
	}
	m_pUI->UIEnable(ID_JOB_CANCEL, count > 0);
	m_pUI->UIEnable(ID_JOB_PAUSE, count > 0 && info->State == BG_JOB_STATE_TRANSFERRING);
	m_pUI->UIEnable(ID_JOB_RESUME, count > 0 && info->State == BG_JOB_STATE_SUSPENDED);
	m_pUI->UIEnable(ID_JOB_PROPERTIES, count == 1);
}

void CView::ShowProperties(int index) {
	CJobPropertiesDlg dlg(m_Jobs[index]->spJob);
	dlg.DoModal();
}

void CView::Refresh() {
	for (auto& job : m_Jobs)
		UpdateBitsJob(job.get());

	if (m_spMgr) {
		CComPtr<IEnumBackgroundCopyJobs> spJobs;
		m_spMgr->EnumJobsW(SecurityHelper::IsRunningElevated() ? BG_JOB_ENUM_ALL_USERS : 0, &spJobs);
		if (spJobs) {
			ULONG count;
			spJobs->GetCount(&count);
			for (ULONG i = 0; i < count; i++) {
				CComPtr<IBackgroundCopyJob> spJob;
				spJobs->Next(1, &spJob, nullptr);
				if (spJob) {
					Guid id;
					spJob->GetId(&id);
					if (auto it = m_JobsMap.find(id); it == m_JobsMap.end())
						AddBitsJob(spJob);
				}
			}
		}
		SetItemCountEx((int)m_Jobs.size(), LVSICF_NOSCROLL);
	}
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
		PSID sid;
		if (::ConvertStringSidToSid(text, &sid)) {
			WCHAR name[32], domain[32];
			DWORD lname = _countof(name), ldomain = _countof(domain);
			SID_NAME_USE use;
			if (::LookupAccountSid(nullptr, sid, name, &lname, domain, &ldomain, &use)) {
				info->Owner = CString(domain) + L"\\" + name;
			}
			::LocalFree(sid);
		}
		else {
			info->Owner = text;
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
	ATLVERIFY(SUCCEEDED(pJob->GetState(&info->State)));

	pJob->GetTimes(&info->Times);
	pJob->GetPriority(&info->Priority);
	pJob->GetProgress(&info->Progress);

	return true;
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	DefWindowProc();
	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	auto hr = m_spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (FAILED(hr))
		return -1;

	auto cm = GetColumnManager(*this);
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

	cm->UpdateColumns();

	CImageList images;
	images.Create(16, 16, ILC_COLOR32, 8, 4);
	UINT icons[] = {
		IDI_SUSPENDED, IDI_GEAR, IDI_DOWNLOAD, IDI_UPLOAD, IDI_OK, IDI_OK2, IDI_STOP2,
		IDI_CANCEL,
	};
	for (auto icon : icons)
		images.AddIcon(AtlLoadIconImage(icon, 0, 16, 16));
	SetImageList(images, LVSIL_SMALL);

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
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());

	return 0;
}

LRESULT CView::OnCancelJob(WORD, WORD, HWND, BOOL&) {
	auto count = GetSelectedCount();
	if (count) {
		CString text;
		text.Format(L"Cancel %d job(s)?", count);
		if (IDNO == AtlMessageBox(*this, (PCWSTR)text, IDR_MAINFRAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
			return 0;

		int i = -1;
		while ((i = GetNextItem(i, LVIS_SELECTED)) >= 0) {
			auto& item = m_Jobs[i];
			item->spJob->Cancel();
		}
		Refresh();
	}
	return 0;
}

LRESULT CView::OnSuspendJob(WORD, WORD, HWND, BOOL&) {
	auto count = GetSelectedCount();
	if (count) {
		int i = -1;
		while ((i = GetNextItem(i, LVIS_SELECTED)) >= 0) {
			auto& item = m_Jobs[i];
			item->spJob->Suspend();
		}
	}
	Refresh();
	return 0;
}

LRESULT CView::OnResumeJob(WORD, WORD, HWND, BOOL&) {
	auto count = GetSelectedCount();
	if (count) {
		int i = -1;
		while ((i = GetNextItem(i, LVIS_SELECTED)) >= 0) {
			auto& item = m_Jobs[i];
			item->spJob->Resume();
		}
	}
	Refresh();

	return 0;
}

LRESULT CView::OnItemChanged(int, LPNMHDR, BOOL&) {
	UpdateUI();
	return 0;
}

LRESULT CView::OnJobProps(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(GetSelectedCount() == 1);
	auto selected = GetNextItem(-1, LVIS_SELECTED);
	ShowProperties(selected);

	return 0;
}

LRESULT CView::OnEditCopy(WORD, WORD, HWND, BOOL&) {
	auto count = GetSelectedCount();
	CString text;
	if (count == 0 || count == GetItemCount()) {
		for (int i = 0; i < GetItemCount(); i++)
			text += GetLineText(*this, i);
	}
	else {
		for(int i = -1;;) {
			i = GetNextItem(i, LVIS_SELECTED);
			if (i < 0)
				break;
			text += GetLineText(*this, i);
		}
	}
	ClipboardHelper::CopyText(*this, text);

	return 0;
}
