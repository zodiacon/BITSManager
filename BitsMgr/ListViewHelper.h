#pragma once

class ListViewHelper abstract final{
public:
	static CString GetLineText(HWND hListView, int row, PCWSTR separator = L",");
};

