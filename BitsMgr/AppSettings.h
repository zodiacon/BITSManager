#pragma once

#include <Settings.h>

struct AppSettings : Settings {
	BEGIN_SETTINGS(AppSettings)
		SETTING(DarkMode, 0, SettingType::Bool);
		SETTING(AlwaysOnTop, 0, SettingType::Bool);
	END_SETTINGS

	DEF_SETTING(DarkMode, int)
	DEF_SETTING(AlwaysOnTop, int)

	// binary settings, absent until first saved
	static constexpr PCWSTR MainWindowPlacement = L"MainWindowPlacement";	// SaveWindowPosition/LoadWindowPosition
	static constexpr PCWSTR JobListLayout = L"JobListLayout";	// CView::ListLayout

	// as GetBinary, but null (not an assertion) if the saved value's size isn't T's, as after T changed
	template<typename T>
	T const* GetBinaryIfSize(PCWSTR name) const {
		auto it = m_settings.find(name);
		if (it == m_settings.end() || it->second.Size != sizeof(T))
			return nullptr;
		return reinterpret_cast<T const*>(it->second.Buffer.get());
	}
};
