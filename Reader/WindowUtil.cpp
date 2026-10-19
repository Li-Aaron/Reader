// WindowUtil.cpp : window helpers (title, menus, taskbar, tray, dpi)
#include "Reader.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

void ShowHideWindow(HWND hWnd)
{
    // show or hide window
    BOOL isShow = IsWindowVisible(hWnd);
    ShowWindow(hWnd, isShow ? SW_HIDE : SW_SHOW);
    if (!isShow)
    {
        if (IsIconic(hWnd))
            ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
}

void UpdateTitle(HWND hWnd)
{
    TCHAR szOldTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szNewTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szFileName[MAX_PATH] = { 0 };
    TCHAR szChapter[MAX_PATH] = { 0 };

    if (_Book)
    {
        memcpy(szFileName, _Book->GetFileName(), _tcslen(_Book->GetFileName()+1) * sizeof(TCHAR));
        PathRemoveExtension(szFileName);
        if (_Book->IsLoading())
        {
            _stprintf(szNewTitle, _T("%s - %s"), szTitle, PathFindFileName(szFileName));
        }
        else
        {
            if (_Book->GetChapterTitle(szChapter, MAX_PATH))
            {
                _stprintf(szNewTitle, _T("%s - %s - %s"), szTitle, PathFindFileName(szFileName), szChapter);
            }
            else
            {
                _stprintf(szNewTitle, _T("%s - %s"), szTitle, PathFindFileName(szFileName));
            }
        }
    }
    else
    {
        _stprintf(szNewTitle, _T("%s"), szTitle);
    }

    GetWindowText(hWnd, szOldTitle, MAX_LOADSTRING + MAX_PATH + MAX_PATH - 1);
    if (_tcscmp(szOldTitle, szNewTitle))
        SetWindowText(hWnd, szNewTitle);
}

void RemoveMenuById(HMENU hMenu, BOOL separator, int id)
{
    HMENU hSubMenu;
    int count;
    int i,j;
    int menuid;

    if (!hMenu)
        return;

    count = GetMenuItemCount(hMenu);
    for (i=0; i<count; i++)
    {
        hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu)
            RemoveMenuById(hSubMenu, separator, id);
        menuid = GetMenuItemID(hMenu, i);
        if (id == menuid)
        {
            if (separator)
            {
                j = i+1;
                RemoveMenu(hMenu, j, MF_BYPOSITION);
            }
            RemoveMenu(hMenu, id, MF_BYCOMMAND);
            return;
        }
    }
}

void RemoveMenus(HWND hWnd, BOOL redraw)
{
    HMENU hMenu;

    hMenu = GetMenu(hWnd);

#if !ENABLE_TAG
    RemoveMenuById(hMenu, FALSE, IDM_TAGSET);
#endif

    if (redraw)
        DrawMenuBar(hWnd);
}

ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
    ULONGLONG ullVersion = 0;
    HINSTANCE hinstDll;
    hinstDll = LoadLibrary(lpszDllName);
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if(SUCCEEDED(hr))
                ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion,0,0);
        }
        FreeLibrary(hinstDll);
    }
    return ullVersion;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    TCHAR szClassName[MAX_LOADSTRING] = {0};
    HWND hForeWnd = NULL;
    DWORD dwForeID;
    DWORD dwCurID;
    HWND *p_hWnd = (HWND*)lParam;

    GetClassName(hWnd, szClassName, MAX_LOADSTRING);
    if (_tcscmp(szClassName, szWindowClass) == 0)
    {
        hForeWnd = GetForegroundWindow();
        dwCurID = GetCurrentThreadId();
        dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
        AttachThreadInput(dwCurID, dwForeID, TRUE);
        ShowWindow(hWnd, SW_SHOWNORMAL);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SetForegroundWindow(hWnd);
        AttachThreadInput(dwCurID, dwForeID, FALSE);
        *p_hWnd = hWnd;
        return FALSE;
    }
    return TRUE;
}

void ShowInTaskbar(HWND hWnd, BOOL bShow)
{
    HRESULT hr;
    ITaskbarList* pTaskbarList;
    hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList );

    if(SUCCEEDED(hr))
    {
        pTaskbarList->HrInit();
        if(bShow)
            pTaskbarList->AddTab(hWnd);
        else
            pTaskbarList->DeleteTab(hWnd);

        pTaskbarList->Release();
    }
}

void ShowSysTray(HWND hWnd, BOOL bShow)
{
    if (bShow)
    {
        if (!_nid.hWnd)
        {
            // create a systray
            ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
            ZeroMemory(&_nid, sizeof(NOTIFYICONDATA));
            if(ullVersion >= MAKEDLLVERULL(6,0,0,0))
                _nid.cbSize = sizeof(NOTIFYICONDATA);
            else if(ullVersion >= MAKEDLLVERULL(5,0,0,0))
                _nid.cbSize = NOTIFYICONDATA_V2_SIZE;
            else
                _nid.cbSize = NOTIFYICONDATA_V1_SIZE;
            _nid.hWnd = hWnd;
            _nid.uID = IDI_SYSTRAY;
            _nid.uCallbackMessage = WM_SYSTRAY;
            _nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BOOK));
            _tcscpy(_nid.szTip, szTitle);
        }
        if (_nid.uFlags == 0)
        {
            _nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
            Shell_NotifyIcon(NIM_ADD, &_nid);
        }
    }
    else
    {
        if (_nid.uFlags)
        {
            _nid.uFlags = 0;
            Shell_NotifyIcon(NIM_DELETE, &_nid);
        }
    }
}

void SetTreeviewFont()
{
    static HFONT s_hFont = NULL;

    if (s_hFont)
    {
        DeleteObject(s_hFont);
        s_hFont = NULL;
    }

    if (_header->meun_font_follow)
    {
        s_hFont = CreateFontIndirect(&_header->font);
        int height = abs((int)(_header->font.lfHeight * 1.5f));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, height, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, height, NULL);
    }
    else
    {
        NONCLIENTMETRICS theMetrics = {0};
        theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        if (Is_WinXP_SP2_or_Later())
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(theMetrics.iPaddedBorderWidth);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
        theMetrics.iMenuHeight = abs((int)(theMetrics.iMenuHeight * 1.25f)); // 1.25 line height
        if (theMetrics.iMenuHeight == 0)
        {
            theMetrics.iMenuHeight = 20;
        }
        s_hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
    }
}
