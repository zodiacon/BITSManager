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
- The project's linker manifest requests `HighestAvailable` UAC level; `BitsMgr.manifest` (merged in) declares per-monitor v2 DPI awareness. When elevated, the app enumerates BITS jobs of all users (`BG_JOB_ENUM_ALL_USERS`); otherwise only the current user's jobs. To launch without a UAC prompt for testing, set `__COMPAT_LAYER=RunAsInvoker`.

## Testing

There are no automated tests; changes are checked by running the app against real BITS jobs. **The list shows the machine's real jobs** (other apps', such as Outlook's address book download, whose files and settings BITS can't restore once changed). Test with jobs created for the purpose (`Start-BitsTransfer -Asynchronous -Suspended` with a `http://localhost:1/...` source, which never downloads anything), and before driving any command that changes a job, verify which job is selected: the list's order depends on the saved sort, so a row position is not a job. Opening the properties dialog and reading its GUID field (`WM_GETTEXT`; `GetWindowText` doesn't read another process's edit controls) identifies it; Add File and Settings opened from that dialog act on the same job. Remove the test jobs afterwards (`Remove-BitsTransfer`).

## Architecture

Small WTL SDI app; all source is in `BitsMgr/` with precompiled header `pch.h`. It follows the dark mode setup of WinSysLive (`C:\dev\winsyslive\winsyslive`).

- `BitsMgr.cpp`: entry point. Loads `AppSettings` from `HKCU\Software\ScorpioSoftware\BitsMgr` and calls `WTLHelper::InitDarkMode(...)` *before* any window is created (with no saved settings, the mode follows the system), then runs the message loop with `CMainFrame`.
- `AppSettings.h`: a WTLHelper `Settings` subclass (`BEGIN_SETTINGS`/`SETTING`/`DEF_SETTING`); the global instance is reached through `AppSettings::Get()`. Call `Save()` after changing a value. Besides the declared settings, two binary values are written on close (and on `WM_ENDSESSION`): the window placement and the job list's `CView::ListLayout` (widths, order, sort). Read binary structs with `GetBinaryIfSize`, which ignores a saved value of another size; changing `ListLayout` or adding a column just discards the old layout.
- `MainFrm.*`: `CMainFrame`: toolbar, status bar, native menus whose icons are set by `WTLHelper::InitMenu` (`CMainFrame::InitMenu` holds the command→icon table, and is also applied to context menus in `TrackPopupMenu`). Options > Dark Mode toggles through `SetDarkMode` → `WTLHelper::SwitchToMode`, then rebuilds the menu icon bitmaps (they're drawn on the menu background), sets the status bar background by hand and redraws all thread windows. Implements `IMainFrame` (popup-menu tracking for the view). Job commands are routed to the view via `CHAIN_MSG_MAP_ALT_MEMBER(m_view, 1)`, so job-related `COMMAND_ID_HANDLER`s live in `CView`'s `ALT_MSG_MAP(1)`.
- `View.*`: `CView`, a virtual list view (`CVirtualListView` + `CFrameView` from WTLHelper) that holds the `IBackgroundCopyManager` and the job list. It is a member of the frame, so the constructor calls `m_view.SetStatic()`; otherwise `CFrameView::OnFinalMessage` would `delete` it. Jobs are stored as `shared_ptr<JobInfo>` in both `m_Jobs` (display/sort order) and `m_JobsMap` (keyed by job GUID). A 1-second `WM_TIMER` calls `Refresh()`, which updates existing jobs and enumerates for new ones. Column text, row icons, and sorting are provided via the `GetColumnText` / `GetRowImage` / `DoSort` callbacks that `CVirtualListView` expects; adding a column means updating all three plus the column setup in `OnCreate`. The Error column's text (`DescribeError`: description, code, context, file) is fetched again only when the job's error code changes.
- `JobPropertiesDlg.*`: job details dialog (uses `IBackgroundCopyJob2` via QI for extra info, URL list with copy context menu). Chains `CDialogHelper` for DPI-aware button icons. A 1-second timer updates state, progress, error and per-file progress (`UpdateLive`); the editable fields are left alone. Its `AFX_DIALOG_LAYOUT` resource has one row per control in template order, so new controls go at the end of the template with a matching row.
- Status bar: a `CMultiPaneStatusBarCtrl` whose pane IDs are string resources (`IDS_PANE_*`); each string is a template of the longest text and sets the pane's width. The view writes the job counts after every refresh through `IMainFrame::SetStatusText`.
- Elevation: not elevated, File > Run as Administrator restarts the app with the `runas` verb (settings saved first); elevated, the item is removed and the title says so.
- `NewJobDlg.*`: File > New Job; collects name, type (download/upload), priority, one URL and local file. `CView::OnNewJob` creates the job (`CreateJob`, `AddFile`, optionally `Resume`) and cancels it if adding the file fails.
- `AddFileDlg.*`: the properties dialog's Add File; `AddFile` on the job (BITS refuses a second file for upload jobs, and any file for finished ones), after which `UpdateLive` shows the new row.
- `JobSettingsDlg.*`: Job > Settings (and the properties dialog's Settings button): retry delay, no-progress timeout, notification flags, proxy, HTTP headers and security flags (`IBackgroundCopyJobHttpOptions`), credentials (`IBackgroundCopyJob2`). It keeps the values it loaded and sets only what changed. Credentials can't be read back: they're set only if a user name is typed, and Remove acts at once.

DPI: per-monitor v2. Sizes are given at 96 DPI and scaled with `DpiHelper`. On `WM_DPICHANGED` the frame takes the suggested rectangle, rebuilds the tool bar images (`ToolbarHelper::UpdateToolBarImages`, then resizes its rebar band), the menu icons (`InitMenu` at `SmallIconSize`, a shield icon per DPI since `InitMenu` caches by icon handle), and the status bar panes (`SetPanes` again), and calls `CView::OnDpiChanged` for the row icons. Column widths rescale themselves: `ColumnManager::AddColumn` scales them and enables `ListViewHelper::EnableColumnDpiScaling`; a dialog's list view needs both done by hand. Dialogs scale themselves, and `CDialogHelper` redoes their button icons.
- `SecurityHelper.*`: elevation check and shield icon.

Which job states a command applies to is decided in one place, `CView::CanApply`. It drives the toolbar and menu enabling (`UpdateUI`, run after every refresh), the context menu (`EnableMenuItems`), and which selected jobs a command acts on (`ApplyToSelection`). Acknowledged and canceled jobs (`IsFinalState`) are no longer in BITS: they stay listed, unqueried, until Job > Delete Completed Jobs. The list is virtual, so selection is by index; anything that reorders `m_Jobs` must carry the selection over by job ID (`GetSelectedIds`/`SelectIds`).

BITS failures are reported with `CView::ShowError`, which asks BITS for the description (`GetErrorDescription` knows the `BG_E_*` codes). Notify command lines: BITS requires the parameter string to begin with the program; the properties dialog shows and edits only the arguments after it.

Dialogs and controls are themed automatically by WTLHelper's per-thread window creation hook; no per-window dark mode code is needed.

Helpers such as `SortHelper`, `ClipboardHelper`, `ListViewHelper`, `VirtualListView`, `FrameView`, `DialogHelper`, `Settings` come from the `wtlhelper` submodule (`..\wtlhelper\WTLHelper` on the include path; it has its own `CLAUDE.md`). Prefer reusing them over writing new utilities; changes to them belong in the submodule repo and affect other apps that use it.

Resources (menus, dialogs, icons, toolbar bitmaps) are in `BitsMgr.rc` (Windows-1252 encoded, CRLF) / `resource.h` / `res/`.
