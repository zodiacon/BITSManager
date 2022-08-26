#include "pch.h"
#include "ListViewHelper.h"

CString ListViewHelper::GetLineText(HWND hListView, int row, PCWSTR separator) {
	CListViewCtrl lv(hListView);
	auto columns = lv.GetHeader().GetItemCount();
	CString text, item;
	for (int i = 0; i < columns; i++) {
		lv.GetItemText(row, i, item);
		text += item + separator;
	}
	text.SetAt(text.GetLength() - 1, L'\n');
	return text;
}
