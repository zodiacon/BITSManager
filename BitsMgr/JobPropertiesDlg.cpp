#include "pch.h"
#include "JobPropertiesDlg.h"

CJobPropertiesDlg::CJobPropertiesDlg(IBackgroundCopyJob* pJob) : m_pJob(pJob) {
}

void CJobPropertiesDlg::InitJob() {
    PWSTR name;
    m_pJob->GetDisplayName(&name);
    SetDlgItemText(IDC_DISPLAYNAME, name);
    ::CoTaskMemFree(name);

    m_pJob->GetDescription(&name);
    SetDlgItemText(IDC_DESC, name);
    ::CoTaskMemFree(name);

    GUID id;
    m_pJob->GetId(&id);
    WCHAR text[64];
    ::StringFromGUID2(id, text, _countof(text));
    SetDlgItemText(IDC_GUID, text);

    CComPtr<IEnumBackgroundCopyFiles> spFiles;
    m_pJob->EnumFiles(&spFiles);
    if (spFiles) {
        ULONG count;
        spFiles->GetCount(&count);
        for (ULONG i = 0; i < count; i++) {
            CComPtr<IBackgroundCopyFile> spFile;
            spFiles->Next(1, &spFile, nullptr);
            if (spFile) {
                spFile->GetRemoteName(&name);
                int n = m_List.InsertItem(m_List.GetItemCount(), name);
                ::CoTaskMemFree(name);
                spFile->GetLocalName(&name);
                m_List.SetItemText(n, 1, name);
                ::CoTaskMemFree(name);
                BG_JOB_STATE state;
                m_pJob->GetState(&state);
                if (state != BG_JOB_STATE_ERROR) {
                    BG_FILE_PROGRESS progress;
                    spFile->GetProgress(&progress);
                    CString text;
                    text.Format(L"%u K / %u K", progress.BytesTransferred >> 10, progress.BytesTotal >> 10);
                    m_List.SetItemText(n, 2, text);
                }
            }
        }
    }
}

LRESULT CJobPropertiesDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    m_List.Attach(GetDlgItem(IDC_FILES));
    m_List.InsertColumn(0, L"Remote URL", LVCFMT_LEFT, 250);
    m_List.InsertColumn(1, L"Local Path", LVCFMT_LEFT, 250);
    m_List.InsertColumn(2, L"Progress", LVCFMT_RIGHT, 100);

    InitJob();

    return 0;
}

LRESULT CJobPropertiesDlg::OnCloseCmd(WORD, WORD id, HWND, BOOL&) {
    if (id == IDOK) {
        // update job
        CString text;
        GetDlgItemText(IDC_DISPLAYNAME, text);
        m_pJob->SetDisplayName(text);
        GetDlgItemText(IDC_DESC, text);
        m_pJob->SetDescription(text);        
    }
    EndDialog(id);
    return 0;
}
