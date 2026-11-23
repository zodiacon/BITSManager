#include "pch.h"
#include "JobSettingsDlg.h"
#include "View.h"
#include <bits2_5.h>

namespace {
	struct CheckFlag {
		UINT id;
		ULONG flag;
	};

	const CheckFlag s_NotifyFlags[] = {
		{ IDC_NOTIFY_TRANSFERRED, BG_NOTIFY_JOB_TRANSFERRED },
		{ IDC_NOTIFY_ERROR, BG_NOTIFY_JOB_ERROR },
		{ IDC_NOTIFY_MODIFICATION, BG_NOTIFY_JOB_MODIFICATION },
		{ IDC_NOTIFY_FILE, BG_NOTIFY_FILE_TRANSFERRED },
		{ IDC_NOTIFY_DISABLE, BG_NOTIFY_DISABLE },
	};

	const CheckFlag s_SecurityFlags[] = {
		{ IDC_IGNORE_CN, BG_SSL_IGNORE_CERT_CN_INVALID },
		{ IDC_IGNORE_DATE, BG_SSL_IGNORE_CERT_DATE_INVALID },
		{ IDC_IGNORE_CA, BG_SSL_IGNORE_UNKNOWN_CA },
		{ IDC_IGNORE_USAGE, BG_SSL_IGNORE_CERT_WRONG_USAGE },
		{ IDC_CHECK_CRL, BG_SSL_ENABLE_CRL_CHECK },
		{ IDC_HTTPS_TO_HTTP, BG_HTTP_REDIRECT_POLICY_ALLOW_HTTPS_TO_HTTP },
	};

	// the custom headers are one string with CRLF between them; the edit box shows one per line
	CString NormalizeHeaders(CString text) {
		text.Replace(L"\r\n", L"\n");
		CString result;
		int pos = 0;
		for (auto line = text.Tokenize(L"\n", pos); pos >= 0; line = text.Tokenize(L"\n", pos)) {
			line.Trim();
			if (line.IsEmpty())
				continue;
			if (!result.IsEmpty())
				result += L"\r\n";
			result += line;
		}
		return result;
	}

	void AddItem(CComboBox& cb, PCWSTR text, DWORD_PTR data) {
		cb.SetItemData(cb.AddString(text), data);
	}

	void SelectData(CComboBox& cb, DWORD_PTR data) {
		for (int i = 0; i < cb.GetCount(); i++)
			if (cb.GetItemData(i) == data) {
				cb.SetCurSel(i);
				return;
			}
		cb.SetCurSel(0);
	}
}

LRESULT CJobSettingsDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	AdjustOKCancelButtons(IDI_OK2, IDI_CANCEL);
	SetDialogIcon(IDI_GEAR);

	m_spJob2 = m_pJob;
	m_spHttp = m_pJob;

	// CBS_SORT isn't set: the items keep this order
	m_ProxyUsage.Attach(GetDlgItem(IDC_PROXYUSAGE));
	AddItem(m_ProxyUsage, L"System default", BG_JOB_PROXY_USAGE_PRECONFIG);
	AddItem(m_ProxyUsage, L"No proxy", BG_JOB_PROXY_USAGE_NO_PROXY);
	AddItem(m_ProxyUsage, L"Use these proxies", BG_JOB_PROXY_USAGE_OVERRIDE);
	AddItem(m_ProxyUsage, L"Detect automatically", BG_JOB_PROXY_USAGE_AUTODETECT);

	m_Redirect.Attach(GetDlgItem(IDC_REDIRECT));
	AddItem(m_Redirect, L"Allow", BG_HTTP_REDIRECT_POLICY_ALLOW_SILENT);
	AddItem(m_Redirect, L"Allow and report", BG_HTTP_REDIRECT_POLICY_ALLOW_REPORT);
	AddItem(m_Redirect, L"Don't allow", BG_HTTP_REDIRECT_POLICY_DISALLOW);

	m_CredTarget.Attach(GetDlgItem(IDC_CRED_TARGET));
	AddItem(m_CredTarget, L"Server", BG_AUTH_TARGET_SERVER);
	AddItem(m_CredTarget, L"Proxy", BG_AUTH_TARGET_PROXY);
	m_CredTarget.SetCurSel(0);

	m_CredScheme.Attach(GetDlgItem(IDC_CRED_SCHEME));
	AddItem(m_CredScheme, L"Basic", BG_AUTH_SCHEME_BASIC);
	AddItem(m_CredScheme, L"Digest", BG_AUTH_SCHEME_DIGEST);
	AddItem(m_CredScheme, L"NTLM", BG_AUTH_SCHEME_NTLM);
	AddItem(m_CredScheme, L"Negotiate", BG_AUTH_SCHEME_NEGOTIATE);
	AddItem(m_CredScheme, L"Passport", BG_AUTH_SCHEME_PASSPORT);
	m_CredScheme.SetCurSel(3);

	LoadSettings();
	CenterWindow(GetParent());
	return TRUE;
}

void CJobSettingsDlg::LoadSettings() {
	m_pJob->GetMinimumRetryDelay(&m_RetryDelay);
	m_pJob->GetNoProgressTimeout(&m_NoProgress);
	SetDlgItemInt(IDC_RETRYDELAY, m_RetryDelay, FALSE);
	SetDlgItemInt(IDC_NOPROGRESS, m_NoProgress, FALSE);

	m_pJob->GetNotifyFlags(&m_NotifyFlags);
	for (auto& f : s_NotifyFlags)
		CheckDlgButton(f.id, (m_NotifyFlags & f.flag) ? BST_CHECKED : BST_UNCHECKED);

	PWSTR list = nullptr, bypass = nullptr;
	if (SUCCEEDED(m_pJob->GetProxySettings(&m_Usage, &list, &bypass))) {
		m_ProxyList = list;
		m_ProxyBypass = bypass;
		::CoTaskMemFree(list);
		::CoTaskMemFree(bypass);
	}
	SelectData(m_ProxyUsage, m_Usage);
	SetDlgItemText(IDC_PROXYLIST, m_ProxyList);
	SetDlgItemText(IDC_PROXYBYPASS, m_ProxyBypass);
	EnableProxyLists();

	if (m_spHttp) {
		PWSTR headers = nullptr;
		if (SUCCEEDED(m_spHttp->GetCustomHeaders(&headers)) && headers) {
			m_Headers = NormalizeHeaders(headers);
			::CoTaskMemFree(headers);
		}
		SetDlgItemText(IDC_HEADERS, m_Headers);
		m_spHttp->GetSecurityFlags(&m_SecurityFlags);
		for (auto& f : s_SecurityFlags)
			CheckDlgButton(f.id, (m_SecurityFlags & f.flag) ? BST_CHECKED : BST_UNCHECKED);
		SelectData(m_Redirect, m_SecurityFlags & BG_HTTP_REDIRECT_POLICY_MASK);
	}
	else {
		// BITS before 2.5 (no IBackgroundCopyJobHttpOptions)
		for (UINT id : { IDC_HEADERS, IDC_IGNORE_CN, IDC_IGNORE_DATE, IDC_IGNORE_CA, IDC_IGNORE_USAGE, IDC_CHECK_CRL, IDC_HTTPS_TO_HTTP, IDC_REDIRECT })
			GetDlgItem(id).EnableWindow(FALSE);
	}

	if (!m_spJob2) {
		for (UINT id : { IDC_CRED_TARGET, IDC_CRED_SCHEME, IDC_CRED_USER, IDC_CRED_PASSWORD, IDC_CRED_REMOVE })
			GetDlgItem(id).EnableWindow(FALSE);
	}
}

void CJobSettingsDlg::EnableProxyLists() {
	// the lists apply only to "Use these proxies"; BITS rejects them with the other usages
	bool lists = m_ProxyUsage.GetItemData(m_ProxyUsage.GetCurSel()) == BG_JOB_PROXY_USAGE_OVERRIDE;
	GetDlgItem(IDC_PROXYLIST).EnableWindow(lists);
	GetDlgItem(IDC_PROXYBYPASS).EnableWindow(lists);
}

LRESULT CJobSettingsDlg::OnProxyUsageChanged(WORD, WORD, HWND, BOOL&) {
	EnableProxyLists();
	return 0;
}

ULONG CJobSettingsDlg::GetNotifyFlagsFromUI() const {
	ULONG flags = 0;
	for (auto& f : s_NotifyFlags)
		if (IsDlgButtonChecked(f.id) == BST_CHECKED)
			flags |= f.flag;
	return flags;
}

DWORD CJobSettingsDlg::GetSecurityFlagsFromUI() const {
	DWORD flags = (DWORD)m_Redirect.GetItemData(m_Redirect.GetCurSel());
	for (auto& f : s_SecurityFlags)
		if (IsDlgButtonChecked(f.id) == BST_CHECKED)
			flags |= f.flag;
	return flags;
}

bool CJobSettingsDlg::Check(HRESULT hr, PCWSTR what) {
	if (FAILED(hr))
		AtlMessageBox(m_hWnd, (PCWSTR)(CString(L"Failed to set the ") + what + L".\n\n" + CView::ErrorText(hr)),
			IDR_MAINFRAME, MB_ICONERROR);
	return SUCCEEDED(hr);
}

bool CJobSettingsDlg::SaveSettings() {
	BOOL ok;
	auto retry = GetDlgItemInt(IDC_RETRYDELAY, &ok, FALSE);
	if (ok && retry != m_RetryDelay && !Check(m_pJob->SetMinimumRetryDelay(retry), L"minimum retry delay"))
		return false;
	auto noProgress = GetDlgItemInt(IDC_NOPROGRESS, &ok, FALSE);
	if (ok && noProgress != m_NoProgress && !Check(m_pJob->SetNoProgressTimeout(noProgress), L"no-progress timeout"))
		return false;

	auto notify = GetNotifyFlagsFromUI();
	if (notify != m_NotifyFlags && !Check(m_pJob->SetNotifyFlags(notify), L"notification flags"))
		return false;

	auto usage = (BG_JOB_PROXY_USAGE)m_ProxyUsage.GetItemData(m_ProxyUsage.GetCurSel());
	CString list, bypass;
	if (usage == BG_JOB_PROXY_USAGE_OVERRIDE) {
		GetDlgItemText(IDC_PROXYLIST, list);
		GetDlgItemText(IDC_PROXYBYPASS, bypass);
		list.Trim();
		bypass.Trim();
		if (list.IsEmpty()) {
			AtlMessageBox(m_hWnd, L"\"Use these proxies\" needs a proxy list.", IDR_MAINFRAME, MB_ICONWARNING);
			GotoDlgCtrl(GetDlgItem(IDC_PROXYLIST));
			return false;
		}
	}
	if (usage != m_Usage || list != m_ProxyList || bypass != m_ProxyBypass) {
		// the lists must be null unless overriding
		bool lists = usage == BG_JOB_PROXY_USAGE_OVERRIDE;
		auto hr = m_pJob->SetProxySettings(usage, lists ? (PCWSTR)list : nullptr,
			lists && !bypass.IsEmpty() ? (PCWSTR)bypass : nullptr);
		if (!Check(hr, L"proxy settings"))
			return false;
	}

	if (m_spHttp) {
		CString headers;
		GetDlgItemText(IDC_HEADERS, headers);
		headers = NormalizeHeaders(headers);
		if (headers != m_Headers &&
			!Check(m_spHttp->SetCustomHeaders(headers.IsEmpty() ? nullptr : (PCWSTR)headers), L"custom headers"))
			return false;

		auto flags = GetSecurityFlagsFromUI();
		if (flags != m_SecurityFlags && !Check(m_spHttp->SetSecurityFlags(flags), L"security flags"))
			return false;
	}

	if (m_spJob2) {
		CString user, password;
		GetDlgItemText(IDC_CRED_USER, user);
		GetDlgItemText(IDC_CRED_PASSWORD, password);
		user.Trim();
		if (!user.IsEmpty()) {
			BG_AUTH_CREDENTIALS cred{};
			cred.Target = (BG_AUTH_TARGET)m_CredTarget.GetItemData(m_CredTarget.GetCurSel());
			cred.Scheme = (BG_AUTH_SCHEME)m_CredScheme.GetItemData(m_CredScheme.GetCurSel());
			cred.Credentials.Basic.UserName = user.GetBuffer();
			cred.Credentials.Basic.Password = password.GetBuffer();
			auto hr = m_spJob2->SetCredentials(&cred);
			::SecureZeroMemory(password.GetBuffer(), password.GetLength() * sizeof(WCHAR));
			if (!Check(hr, L"credentials"))
				return false;
		}
	}
	return true;
}

LRESULT CJobSettingsDlg::OnRemoveCredentials(WORD, WORD, HWND, BOOL&) {
	auto target = (BG_AUTH_TARGET)m_CredTarget.GetItemData(m_CredTarget.GetCurSel());
	auto scheme = (BG_AUTH_SCHEME)m_CredScheme.GetItemData(m_CredScheme.GetCurSel());
	auto hr = m_spJob2->RemoveCredentials(target, scheme);
	if (hr == S_FALSE) {
		// none were set for this target and scheme
		AtlMessageBox(m_hWnd, L"The job has no credentials for this target and scheme.", IDR_MAINFRAME, MB_ICONINFORMATION);
	}
	else if (SUCCEEDED(hr)) {
		AtlMessageBox(m_hWnd, (PCWSTR)(L"Removed the " + GetCombo(m_CredScheme) + L" credentials for the " + GetCombo(m_CredTarget).MakeLower() + L"."),
			IDR_MAINFRAME, MB_ICONINFORMATION);
	}
	else {
		AtlMessageBox(m_hWnd, (PCWSTR)(L"Failed to remove the credentials.\n\n" + CView::ErrorText(hr)), IDR_MAINFRAME, MB_ICONERROR);
	}
	return 0;
}

CString CJobSettingsDlg::GetCombo(CComboBox const& cb) {
	CString text;
	cb.GetLBText(cb.GetCurSel(), text);
	return text;
}

LRESULT CJobSettingsDlg::OnOK(WORD, WORD, HWND, BOOL&) {
	if (SaveSettings())
		EndDialog(IDOK);
	return 0;
}

LRESULT CJobSettingsDlg::OnCancel(WORD, WORD, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}
