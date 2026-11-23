// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "SecurityHelper.h"
#include <WTLHelper.h>

CMainFrame::CMainFrame() : m_view(this) {
	// the view is a member, not heap-allocated: CFrameView must not delete it in OnFinalMessage
	m_view.SetStatic();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateStatusBar();
	UIUpdateToolBar();

	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_Elevated = SecurityHelper::IsRunningElevated();

	CMenuHandle hMenu = GetMenu();
	if (m_Elevated) {
		// already elevated: no Run as Administrator, and the title says so
		hMenu.GetSubMenu(0).DeleteMenu(ID_FILE_RUNASADMIN, MF_BYCOMMAND);
		CString title;
		GetWindowText(title);
		SetWindowText(title + L" (Administrator)");
	}
	UIAddMenu(hMenu);
	InitMenu(hMenu);

	CToolBarCtrl tb;
	auto hWndToolBar = tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE, 0, ATL_IDW_TOOLBAR);
	InitToolBar(tb);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	UIAddToolBar(hWndToolBar);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	// the panes' widths come from their string resources (templates of the longest text)
	m_hWndStatusBar = m_StatusBar.Create(m_hWnd);
	int panes[] = { ID_DEFAULT_PANE, IDS_PANE_JOBS, IDS_PANE_SCOPE };
	m_StatusBar.SetPanes(panes, _countof(panes), false);
	SetStatusText(IDS_PANE_SCOPE, m_Elevated ? L"All users' jobs" : L"Current user's jobs (not elevated)");

	m_view.SetUpdateUI(this);
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	auto& settings = AppSettings::Get();
	m_view.LoadLayout(settings);
	if (settings.AlwaysOnTop()) {
		SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
		UISetCheck(ID_OPTIONS_ALWAYSONTOP, true);
	}

	bool dark = WTLHelper::IsDarkMode();
	UISetCheck(ID_OPTIONS_DARKMODE, dark);
	SetDarkMode(dark);

	// register object for message filtering and idle updates
	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	return 0;
}

BOOL CMainFrame::TrackPopupMenu(HMENU hMenu, DWORD flags, const POINT* pt) {
	POINT pt2;
	if (pt == nullptr) {
		pt = &pt2;
		::GetCursorPos(&pt2);
	}
	InitMenu(hMenu);
	return ::TrackPopupMenuEx(hMenu, flags, pt->x, pt->y, m_hWnd, nullptr);
}

void CMainFrame::SetStatusText(int pane, PCWSTR text) {
	m_StatusBar.SetPaneText(pane, text);
}

void CMainFrame::SaveSettings() {
	auto& settings = AppSettings::Get();
	settings.SaveWindowPosition(m_hWnd, AppSettings::MainWindowPlacement);
	settings.AlwaysOnTop((GetExStyle() & WS_EX_TOPMOST) != 0);
	m_view.SaveLayout(settings);
	settings.Save();
}

LRESULT CMainFrame::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	SaveSettings();
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnEndSession(UINT, WPARAM wParam, LPARAM, BOOL&) {
	// at logoff or shutdown no window is closed, so this is the last chance to save
	if (wParam)
		SaveSettings();
	return 0;
}

LRESULT CMainFrame::OnRunAsAdmin(WORD, WORD, HWND, BOOL&) {
	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));

	// saved first, so the elevated instance starts with this one's layout
	SaveSettings();
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = path;
	sei.hwnd = m_hWnd;
	sei.nShow = SW_SHOWNORMAL;
	if (::ShellExecuteEx(&sei)) {
		PostMessage(WM_CLOSE);
	}
	else if (auto error = ::GetLastError(); error != ERROR_CANCELLED) {
		AtlMessageBox(m_hWnd, (PCWSTR)(L"Failed to start elevated.\n\n" + CView::ErrorText(HRESULT_FROM_WIN32(error))),
			IDR_MAINFRAME, MB_ICONERROR);
	}
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnAlwaysOnTop(WORD, WORD id, HWND, BOOL&) {
	bool onTop = GetExStyle() & WS_EX_TOPMOST;
	SetWindowPos(onTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	UISetCheck(id, !onTop);
	auto& settings = AppSettings::Get();
	settings.AlwaysOnTop(!onTop);
	settings.Save();

	return 0;
}

LRESULT CMainFrame::OnDarkMode(WORD, WORD id, HWND, BOOL&) {
	auto& settings = AppSettings::Get();
	// without saved settings the mode follows the system's, so toggle what's shown
	bool dark = !WTLHelper::IsDarkMode();
	settings.DarkMode(dark);
	settings.Save();
	SetDarkMode(dark);
	UISetCheck(id, dark);

	return 0;
}

void CMainFrame::SetDarkMode(bool dark) {
	WTLHelper::SwitchToMode(dark ? DarkModeKind::Dark : DarkModeKind::Light, m_hWnd);

	// menu icon bitmaps are drawn on the current menu background, so rebuild them for the new mode
	InitMenu(GetMenu());

	CStatusBarCtrl(m_hWndStatusBar).SetBkColor(dark ? DarkMode::getBackgroundColor() : CLR_DEFAULT);

	::EnumThreadWindows(::GetCurrentThreadId(), [](auto h, auto) {
		::RedrawWindow(h, nullptr, nullptr, RDW_ERASENOW | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		return TRUE;
		}, 0);

	DrawMenuBar();
}

void CMainFrame::InitMenu(HMENU hMenu) {
	// InitMenu keeps a bitmap per icon handle, and GetShieldIcon makes a new icon on every call: get it once
	static const HICON hShield = SecurityHelper::GetShieldIcon();
	static const MenuItemData cmds[] = {
		{ ID_FILE_RUNASADMIN, 0, hShield },
		{ ID_JOB_CANCEL, IDI_CANCEL },
		{ ID_JOB_PAUSE, IDI_SUSPENDED },
		{ ID_JOB_RESUME, IDI_PLAY },
		{ ID_JOB_PROPERTIES, IDI_FILE },
		{ ID_JOB_COMPLETE, IDI_OK },
		{ ID_FILE_NEWJOB, IDI_NEW_GEAR },
		{ ID_FILE_SAVE_AS, IDI_SAVE_AS },
		{ ID_APP_ABOUT, IDI_ABOUT },
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_OPTIONS_ALWAYSONTOP, IDI_PIN },
		{ ID_VIEW_REFRESH, IDI_REFRESH },
	};
	WTLHelper::InitMenu(hMenu, cmds, _countof(cmds));
}

void CMainFrame::InitToolBar(CToolBarCtrl& tb, int size) {
	CImageList tbImages;
	tbImages.Create(size, size, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);

	struct {
		UINT id;
		int image;
		int style = BTNS_BUTTON;
	} buttons[] = {
		{ ID_VIEW_REFRESH, IDI_REFRESH },
		{ 0 },
		{ ID_EDIT_COPY, IDI_COPY },
		{ 0 },
		{ ID_JOB_CANCEL, IDI_CANCEL },
		{ ID_JOB_PAUSE, IDI_SUSPENDED },
		{ ID_JOB_RESUME, IDI_PLAY },
		{ ID_JOB_PROPERTIES, IDI_FILE },
	};
	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = tbImages.AddIcon(AtlLoadIconImage(b.image, 0, size, size));
			tb.AddButton(b.id, b.style, TBSTATE_ENABLED, image, nullptr, 0);
		}
	}
}
