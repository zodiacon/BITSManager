// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"
#include "AppSettings.h"
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
	CMenuHandle hMenu = GetMenu();
	UIAddMenu(hMenu);
	InitMenu(hMenu);

	CToolBarCtrl tb;
	auto hWndToolBar = tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE, 0, ATL_IDW_TOOLBAR);
	InitToolBar(tb);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	UIAddToolBar(hWndToolBar);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	CreateSimpleStatusBar();

	m_view.SetUpdateUI(this);
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

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
	static const MenuItemData cmds[] = {
		{ ID_JOB_CANCEL, IDI_CANCEL },
		{ ID_JOB_PAUSE, IDI_SUSPENDED },
		{ ID_JOB_RESUME, IDI_PLAY },
		{ ID_JOB_PROPERTIES, IDI_FILE },
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
