# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

BITS Manager (`BitsMgr`) is a native Win32 GUI tool for viewing and managing Windows BITS (Background Intelligent Transfer Service) jobs. It is C++20 using ATL/WTL, built with Visual Studio 18 (toolset v145, the one the WTLHelper submodule uses). There are no automated tests.

## Build

The `wtlhelper` directory is a git submodule (a shared WTL helper library; its `WTLHelper` static lib project is in `BitsMgr.sln` and referenced by `BitsMgr.vcxproj`). Initialize it before building:

```
git submodule update --init
```

Build with VS 18's MSBuild (`C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe`):

```
msbuild BitsMgr.sln /p:Configuration=Release /p:Platform=x64 /m
```

- Configurations: `Debug`, `Release`, `ReleaseSigned`; platforms: `x64`, `x86` (solution) / `Win32` (project).
- `ReleaseSigned` (x64) runs a post-build `signtool sign /a /n Scorpio ...` (same command as WinSysLive), which needs the maintainer's code signing certificate. Use `Release` for normal builds.
- Static CRT (`/MT`, `/MTd`) everywhere, as WTLHelper requires (a DLL-runtime project fails with LNK2038).
- Dependencies come from vcpkg with user-wide integration: Detours (linked for WTLHelper's dark mode hooks) and WIL. The project sets `VcpkgUseStatic`, so the `x64-windows-static` / `x86-windows-static` triplets must be installed. WTL headers are vendored in `wtlhelper/WTL` (on the include path, including for the resource compiler).
- `pch.h` targets Windows 10 (`WINVER`/`_WIN32_WINNT` `0x0A00`); WTLHelper's DPI helpers need it.
- The manifest requests `HighestAvailable` UAC level. When elevated, the app enumerates BITS jobs of all users (`BG_JOB_ENUM_ALL_USERS`); otherwise only the current user's jobs. To launch without a UAC prompt for testing, set `__COMPAT_LAYER=RunAsInvoker`.

## Architecture

Small WTL SDI app; all source is in `BitsMgr/` with precompiled header `pch.h`. It follows the dark mode setup of WinSysLive (`C:\dev\winsyslive\winsyslive`).

- `BitsMgr.cpp`: entry point. Loads `AppSettings` from `HKCU\Software\ScorpioSoftware\BitsMgr` and calls `WTLHelper::InitDarkMode(...)` *before* any window is created (with no saved settings, the mode follows the system), then runs the message loop with `CMainFrame`.
- `AppSettings.h`: a WTLHelper `Settings` subclass (`BEGIN_SETTINGS`/`SETTING`/`DEF_SETTING`); the global instance is reached through `AppSettings::Get()`. Call `Save()` after changing a value.
- `MainFrm.*`: `CMainFrame`: toolbar, status bar, native menus whose icons are set by `WTLHelper::InitMenu` (`CMainFrame::InitMenu` holds the command→icon table, and is also applied to context menus in `TrackPopupMenu`). Options > Dark Mode toggles through `SetDarkMode` → `WTLHelper::SwitchToMode`, then rebuilds the menu icon bitmaps (they're drawn on the menu background), sets the status bar background by hand and redraws all thread windows. Implements `IMainFrame` (popup-menu tracking for the view). Job commands are routed to the view via `CHAIN_MSG_MAP_ALT_MEMBER(m_view, 1)`, so job-related `COMMAND_ID_HANDLER`s live in `CView`'s `ALT_MSG_MAP(1)`.
- `View.*`: `CView`, a virtual list view (`CVirtualListView` + `CFrameView` from WTLHelper) that holds the `IBackgroundCopyManager` and the job list. It is a member of the frame, so the constructor calls `m_view.SetStatic()`; otherwise `CFrameView::OnFinalMessage` would `delete` it. Jobs are stored as `shared_ptr<JobInfo>` in both `m_Jobs` (display/sort order) and `m_JobsMap` (keyed by job GUID). A 1-second `WM_TIMER` calls `Refresh()`, which updates existing jobs and enumerates for new ones. Column text, row icons, and sorting are provided via the `GetColumnText` / `GetRowImage` / `DoSort` callbacks that `CVirtualListView` expects; adding a column means updating all three plus the column setup in `OnCreate`.
- `JobPropertiesDlg.*`: job details dialog (uses `IBackgroundCopyJob2` via QI for extra info, URL list with copy context menu). Chains `CDialogHelper` for DPI-aware button icons.
- `SecurityHelper.*`: elevation check and shield icon.

Dialogs and controls are themed automatically by WTLHelper's per-thread window creation hook; no per-window dark mode code is needed.

Helpers such as `SortHelper`, `ClipboardHelper`, `ListViewHelper`, `VirtualListView`, `FrameView`, `DialogHelper`, `Settings` come from the `wtlhelper` submodule (`..\wtlhelper\WTLHelper` on the include path; it has its own `CLAUDE.md`). Prefer reusing them over writing new utilities; changes to them belong in the submodule repo and affect other apps that use it.

Resources (menus, dialogs, icons, toolbar bitmaps) are in `BitsMgr.rc` (Windows-1252 encoded, CRLF) / `resource.h` / `res/`.
