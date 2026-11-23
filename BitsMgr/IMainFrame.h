#pragma once

struct IMainFrame {
	virtual BOOL TrackPopupMenu(HMENU hMenu, DWORD flags = 0, const POINT* pt = nullptr) = 0;
	// pane: a status bar pane ID (IDS_PANE_*)
	virtual void SetStatusText(int pane, PCWSTR text) = 0;
};
