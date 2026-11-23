#include "pch.h"
#include "JobPropertiesDlg.h"
#include "DialogHelper.h"
#include <WTLHelper.h>
#include <DpiHelper.h>
#include "View.h"
#include "ListViewHelper.h"
#include "ClipboardHelper.h"
#include "AddFileDlg.h"
#include "JobSettingsDlg.h"

CJobPropertiesDlg::CJobPropertiesDlg(IBackgroundCopyJob* pJob) : m_pJob(pJob) {
}

namespace {
	//
	// BITS requires the notify parameters to start with the program itself (it becomes argv[0]).
	// The dialog shows and edits only the arguments after it.
	//
	CString StripProgram(CString params, CString const& program) {
		params.TrimLeft();
		CString first;
		int end;
		if (params.Left(1) == L"\"") {
			end = params.Find(L'"', 1);
			first = end < 0 ? params.Mid(1) : params.Mid(1, end - 1);
			end = end < 0 ? params.GetLength() : end + 1;
		}
		else {
			end = params.Find(L' ');
			if (end < 0)
				end = params.GetLength();
			first = params.Left(end);
		}
		if (first.CompareNoCase(program) == 0)
			params = params.Mid(end);
		return params.Trim();
	}

	CString FormatSize(UINT64 bytes) {
		CString text;
		if (bytes == BG_SIZE_UNKNOWN)
			text = L"?";
		else if (bytes < (1 << 10))
			text.Format(L"%llu B", bytes);
		else if (bytes < (1 << 20))
			text.Format(L"%llu KB", bytes >> 10);
		else
			text.Format(L"%llu MB", bytes >> 20);
		return text;
	}
}

void CJobPropertiesDlg::InitJob() {
    PWSTR name = nullptr;
    if (SUCCEEDED(m_pJob->GetDisplayName(&name))) {
        m_Name = name;
        ::CoTaskMemFree(name);
    }
    SetDlgItemText(IDC_DISPLAYNAME, m_Name);

    if (SUCCEEDED(m_pJob->GetDescription(&name))) {
        m_Description = name;
        ::CoTaskMemFree(name);
    }
    SetDlgItemText(IDC_DESC, m_Description);
    // read back as the (multiline) edit control has it, so an unchanged description compares equal
    GetDlgItemText(IDC_DESC, m_Description);

    GUID id;
    m_pJob->GetId(&id);
    WCHAR text[64];
    ::StringFromGUID2(id, text, _countof(text));
    SetDlgItemText(IDC_GUID, text);

    CComQIPtr<IBackgroundCopyJob2> spJob2(m_pJob);
    if (spJob2) {
        PWSTR program = nullptr, params = nullptr;
        if (SUCCEEDED(spJob2->GetNotifyCmdLine(&program, &params))) {
            m_Program = program;
            m_Program.Trim();
            if (params)
                m_Params = StripProgram(params, m_Program);
            SetDlgItemText(IDC_PROGRAM, m_Program);
            SetDlgItemText(IDC_PARAMS, m_Params);
            ::CoTaskMemFree(program);
            ::CoTaskMemFree(params);
        }
    }
    else {
        GetDlgItem(IDC_PARAMS).EnableWindow(FALSE);
        GetDlgItem(IDC_PROGRAM).EnableWindow(FALSE);
    }

    m_pJob->GetPriority(&m_OrgPriority);
    for (int i = 0; i < m_Priority.GetCount(); i++)
        if (m_Priority.GetItemData(i) == (DWORD_PTR)m_OrgPriority)
            m_Priority.SetCurSel(i);

    UpdateLive();
}

void CJobPropertiesDlg::UpdateLive() {
    // what changes while the dialog is open (the editable fields are left to the user)
    BG_JOB_STATE state = BG_JOB_STATE_ERROR;
    m_pJob->GetState(&state);
    SetTextIfChanged(IDC_STATE, CView::JobStateToString(state));

    BG_JOB_PROGRESS progress{};
    m_pJob->GetProgress(&progress);
    SetTextIfChanged(IDC_PROGRESS, CView::JobProgressToString(progress));

    CString error;
    if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSIENT_ERROR)
        error = CView::GetJobError(m_pJob);
    SetTextIfChanged(IDC_ERROR, error.IsEmpty() ? L"(none)" : (PCWSTR)error);

    CComPtr<IEnumBackgroundCopyFiles> spFiles;
    m_pJob->EnumFiles(&spFiles);
    if (!spFiles)
        return;

    ULONG count = 0;
    spFiles->GetCount(&count);
    for (ULONG i = 0; i < count; i++) {
        CComPtr<IBackgroundCopyFile> spFile;
        spFiles->Next(1, &spFile, nullptr);
        if (!spFile)
            continue;

        // files can be added to a job, never removed: new ones are appended
        if ((int)i >= m_List.GetItemCount()) {
            CString remote, local;
            PWSTR name;
            if (SUCCEEDED(spFile->GetRemoteName(&name))) {
                remote = name;
                ::CoTaskMemFree(name);
            }
            if (SUCCEEDED(spFile->GetLocalName(&name))) {
                local = name;
                ::CoTaskMemFree(name);
            }
            int n = m_List.InsertItem(m_List.GetItemCount(), remote);
            m_List.SetItemText(n, 1, local);
        }

        BG_FILE_PROGRESS fileProgress;
        CString text;
        if (SUCCEEDED(spFile->GetProgress(&fileProgress)))
            text = FormatSize(fileProgress.BytesTransferred) + L" / " + FormatSize(fileProgress.BytesTotal);
        CString current;
        m_List.GetItemText(i, 2, current);
        if (current != text)
            m_List.SetItemText(i, 2, text);
    }
}

void CJobPropertiesDlg::SetTextIfChanged(UINT id, PCWSTR text) {
    // setting the same text again would flicker, and reset the error box's scroll position and selection
    CString current;
    GetDlgItemText(id, current);
    if (current != text)
        SetDlgItemText(id, text);
}

LRESULT CJobPropertiesDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
    if (id == 1)
        UpdateLive();
    return 0;
}

LRESULT CJobPropertiesDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    InitDynamicLayout();
    AdjustOKCancelButtons(IDI_OK2, IDI_CANCEL);
    SetDialogIcon(IDI_FILE);

    m_List.Attach(GetDlgItem(IDC_FILES));
    m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

    // widths at 96 DPI, scaled to the dialog's; and again when its DPI changes
    m_List.InsertColumn(0, L"Remote URL", LVCFMT_LEFT, DpiHelper::Scale(250, m_hWnd));
    m_List.InsertColumn(1, L"Local Path", LVCFMT_LEFT, DpiHelper::Scale(200, m_hWnd));
    m_List.InsertColumn(2, L"Progress", LVCFMT_CENTER, DpiHelper::Scale(100, m_hWnd));
    ListViewHelper::EnableColumnDpiScaling(m_List);

    m_Priority.Attach(GetDlgItem(IDC_PRIORITY));
    BG_JOB_PRIORITY priorities[] = { BG_JOB_PRIORITY_FOREGROUND, BG_JOB_PRIORITY_HIGH, BG_JOB_PRIORITY_NORMAL, BG_JOB_PRIORITY_LOW };
    for(decltype(priorities[0]) p : priorities)
        m_Priority.SetItemData(m_Priority.AddString(CView::JobPriorityToString(p)), p);

    InitJob();
    SetTimer(1, 1000);

    return 0;
}

LRESULT CJobPropertiesDlg::OnCloseCmd(WORD, WORD id, HWND, BOOL&) {
    if (id == IDOK && !SaveChanges())
        return 0;   // the error was shown; keep the dialog open

    EndDialog(id);
    return 0;
}

bool CJobPropertiesDlg::SaveChanges() {
    // only what changed is set, so an unchanged dialog never fails (e.g. on a job that's no longer in BITS
    // or that belongs to another user)
    auto check = [&](HRESULT hr, PCWSTR what) {
        if (FAILED(hr))
            AtlMessageBox(m_hWnd, (PCWSTR)(CString(L"Failed to set the ") + what + L".\n\n" + CView::ErrorText(hr)),
                IDR_MAINFRAME, MB_ICONERROR);
        return SUCCEEDED(hr);
    };

    CString text;
    GetDlgItemText(IDC_DISPLAYNAME, text);
    if (text != m_Name && !check(m_pJob->SetDisplayName(text), L"display name"))
        return false;
    GetDlgItemText(IDC_DESC, text);
    if (text != m_Description && !check(m_pJob->SetDescription(text), L"description"))
        return false;
    auto priority = (BG_JOB_PRIORITY)m_Priority.GetItemData(m_Priority.GetCurSel());
    if (priority != m_OrgPriority && !check(m_pJob->SetPriority(priority), L"priority"))
        return false;

    CComQIPtr<IBackgroundCopyJob2> spJob2(m_pJob);
    if (spJob2) {
        CString program, params;
        GetDlgItemText(IDC_PROGRAM, program);
        GetDlgItemText(IDC_PARAMS, params);
        program.Trim();
        params.Trim();
        if (program != m_Program || params != m_Params) {
            HRESULT hr;
            if (program.IsEmpty()) {
                // null program removes the command line
                hr = spJob2->SetNotifyCmdLine(nullptr, nullptr);
            }
            else {
                // the parameters must begin with the program
                CString cmdline = L"\"" + program + L"\"";
                if (!params.IsEmpty())
                    cmdline += L" " + params;
                hr = spJob2->SetNotifyCmdLine(program, cmdline);
            }
            if (!check(hr, L"notify command line"))
                return false;
        }
    }
    return true;
}

LRESULT CJobPropertiesDlg::OnBrowse(WORD, WORD id, HWND, BOOL&) {
    CSimpleFileDialog dlg(TRUE, L"exe", nullptr, OFN_EXPLORER | OFN_ENABLESIZING,
        L"Executable Files (*.exe)\0*.exe\0", *this);
    if (dlg.DoModal() == IDOK) {
        SetDlgItemText(IDC_PROGRAM, dlg.m_szFileName);
    }
    return 0;
}

LRESULT CJobPropertiesDlg::OnRightClick(int, LPNMHDR hdr, BOOL&) {
    auto lv = (NMITEMACTIVATE*)hdr;
    if (lv->iItem >= 0) {
        CMenu menu;
        menu.LoadMenuW(IDR_CONTEXT);
        POINT pt;
        ::GetCursorPos(&pt);
        auto popup = menu.GetSubMenu(1);
        WTLHelper::InitMenu(popup, MenuItemData{ ID_EDIT_COPY, IDI_COPY }, DpiHelper::SmallIconSize(m_hWnd));
        // TPM_RETURNCMD: the command comes back here rather than as a posted WM_COMMAND
        auto cmd = (UINT)popup.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
        if (cmd)
            SendMessage(WM_COMMAND, cmd);
    }
    return 0;
}

LRESULT CJobPropertiesDlg::OnEditCopy(WORD, WORD, HWND, BOOL&) {
    CString text;
    for (int i = -1;;) {
        i = m_List.GetNextItem(i, LVIS_SELECTED);
        if (i < 0)
            break;
        text += ListViewHelper::GetRowAsString(m_List, i);
    }
    ClipboardHelper::CopyText(m_hWnd, text);

    return 0;
}

LRESULT CJobPropertiesDlg::OnEditCopyURL(WORD, WORD, HWND, BOOL&) {
    auto count = m_List.GetSelectedCount();
    if (count != 1)
        return 0;

    CString text;
    m_List.GetItemText(m_List.GetSelectedIndex(), 0, text);
    ClipboardHelper::CopyText(m_hWnd, text);

    return 0;
}

LRESULT CJobPropertiesDlg::OnAddFile(WORD, WORD, HWND, BOOL&) {
    BG_JOB_TYPE type = BG_JOB_TYPE_DOWNLOAD;
    m_pJob->GetType(&type);
    CAddFileDlg dlg(type);
    if (dlg.DoModal() != IDOK)
        return 0;

    // an upload job takes one file only, and no job takes files once it's canceled or acknowledged: BITS says so
    auto hr = m_pJob->AddFile(dlg.GetUrl(), dlg.GetLocalPath());
    if (FAILED(hr)) {
        AtlMessageBox(m_hWnd, (PCWSTR)(L"Failed to add the file.\n\n" + CView::ErrorText(hr)), IDR_MAINFRAME, MB_ICONERROR);
        return 0;
    }
    // the new file's row, and the job's new totals
    UpdateLive();
    return 0;
}

LRESULT CJobPropertiesDlg::OnSettings(WORD, WORD, HWND, BOOL&) {
    CJobSettingsDlg dlg(m_pJob);
    dlg.DoModal();
    return 0;
}
