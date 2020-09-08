#pragma once

struct IMainFrame {
	virtual BOOL TrackPopupMenu(HMENU hMenu, DWORD flags = 0, const POINT* pt = nullptr) = 0;
};
