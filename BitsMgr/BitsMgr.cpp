// BitsMgr.cpp : main source file for BitsMgr.exe
//

#include "pch.h"
#include "MainFrm.h"
#include <WTLHelper.h>
#include "AppSettings.h"

CAppModule _Module;
AppSettings _Settings;

int Run(LPTSTR /*lpstrCmdLine*/ = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES);
	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// the mode must be set before any window is created; without saved settings it follows the system
	if (AppSettings::Get().Load(L"Software\\ScorpioSoftware\\BitsMgr"))
		WTLHelper::InitDarkMode(AppSettings::Get().DarkMode() ? DarkModeKind::Dark : DarkModeKind::Light);
	else
		WTLHelper::InitDarkMode();

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
