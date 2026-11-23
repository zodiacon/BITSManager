#pragma once

#include <Settings.h>

struct AppSettings : Settings {
	BEGIN_SETTINGS(AppSettings)
		SETTING(DarkMode, 0, SettingType::Bool);
	END_SETTINGS

	DEF_SETTING(DarkMode, int)
};
