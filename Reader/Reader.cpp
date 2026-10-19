// Reader.cpp : Defines the entry point for the application.
//

#include "Reader.h"
#include "TextBook.h"
#include "EpubBook.h"
#include "MobiBook.h"
#include "Keyset.h"
#include "Editctrl.h"
#include "Advset.h"
#include "DPIAwareness.h"
#include "DisplaySet.h"
#include "AaronMod.h"
#if ENABLE_TAG
#include "tagset.h"
#endif
#include <stdio.h>
#include <shlwapi.h>
#include <CommDlg.h>
#include <commctrl.h>
#include <vector>
#ifdef _DEBUG
#include "dump.h"
#endif

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define MIN_ALPHA_VALUE             0x64

// Globals (hInst / szTitle / szWindowClass / szStatusClass and the shared
// state) are defined in Globals.cpp and declared extern in Reader.h.

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Setting(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    JumpProgress(HWND, UINT, WPARAM, LPARAM);
#ifdef ENABLE_NETWORK
INT_PTR CALLBACK    UpgradeProc(HWND, UINT, WPARAM, LPARAM);
#endif

static void _free_resource(HWND hWnd);
static void _update_data(HWND hWnd, BOOL keep_header, BOOL do_save);


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: Place code here.
#ifdef _DEBUG
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#endif
    MSG msg;
    HACCEL hAccelTable;
    HANDLE hMutex;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_READER, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_STATUSBAR, szStatusClass, MAX_LOADSTRING);

    // check if application is already running
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, szTitle);
    if (hMutex == NULL && ERROR_FILE_NOT_FOUND == GetLastError())
    {
        hMutex = CreateMutex(NULL, TRUE, szTitle);
    }
    else
    {
        // There's another instance running.
        HWND hWnd = NULL;
        LPWSTR *argv;
        int argc;
        COPYDATASTRUCT copyData;

        EnumWindows(EnumWindowsProc, (LPARAM)&hWnd);

        // open new file
        if (hWnd)
        {
            argv = CommandLineToArgvW(GetCommandLineW(), &argc);
            if (argc > 1)
            {
                // open input argument file name
                copyData.dwData = 0;
                copyData.cbData = (DWORD)(wcslen(argv[1]) + 1) * sizeof(wchar_t);
                copyData.lpData = argv[1];
                SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM)&copyData);
            }
        }
        return 0;
    }

    // init com
    CoInitialize(NULL);

    // init gdiplus
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // load cache file
    if (!Init())
    {
        return FALSE;
    }

    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_READER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!_hFindDlg || !IsDialogMessage(_hFindDlg, &msg))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    Exit();

    // uninit gdiplus
    Gdiplus::GdiplusShutdown(gdiplusToken);

    // uninit com
    CoUninitialize();

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
    wcex.lpfnWndProc      = WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOK));
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = CreateSolidBrush(_header->bg_color);//(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName     = MAKEINTRESOURCE(IDC_READER);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm          = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BOOK));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   // prepare for XP style controls
   InitCommonControls();

   // update for dpi
   UpdateRectForDpi(&_header->placement.rcNormalPosition);
   UpdateFontForDpi(&_header->font);
#if ENABLE_TAG
   for (int i = 0; i < MAX_TAG_COUNT; i++)
   {
       UpdateFontForDpi(&_header->tags[i].font);
   }
#endif

   // fixed rect exception bug.
#if 0
    int sw,sh;
    sw = GetCxScreenForDpi();
    sh = GetCyScreenForDpi();
    if (_header->rect.left < 0 && _header->rect.right - _header->rect.left <= sw)
    {
        _header->rect.right -= _header->rect.left;
        _header->rect.left = 0;
    }
    if (_header->rect.right > sw && _header->rect.right - _header->rect.left <= sw)
    {
        _header->rect.left -= _header->rect.right - sw;
        _header->rect.right = sw;
    }
    if (_header->rect.top < 0 && _header->rect.bottom - _header->rect.top <= sh)
    {
        _header->rect.bottom -= _header->rect.top;
        _header->rect.top = 0;
    }
    if (_header->rect.bottom > sh && _header->rect.bottom - _header->rect.top <= sh)
    {
        _header->rect.top -= _header->rect.bottom - sh;
        _header->rect.bottom = sh;
    }
    if (_header->rect.left < 0 || _header->rect.top < 0
        || _header->rect.right > sw || _header->rect.bottom > sh)
    {
        // default rect
        _header->rect.left = (GetCxScreenForDpi() - DEFAULT_APP_WIDTH) / 2;
        _header->rect.top = (GetCyScreenForDpi() - DEFAULT_APP_HEIGHT) / 2;
        _header->rect.right = _header->rect.left + DEFAULT_APP_WIDTH;
        _header->rect.bottom = _header->rect.top + DEFAULT_APP_HEIGHT;
        UpdateLayoutForDpi(&_header->rect);
    }
#endif
   
   _header->style &= ~WS_MINIMIZE;
   _header->style &= ~WS_DISABLED;
   if (_header->placement.showCmd == SW_SHOWMINIMIZED || _header->placement.showCmd == SW_MINIMIZE)
   {
       _header->placement.showCmd = SW_SHOWNORMAL;
   }

   hWnd = CreateWindowEx(_header->exstyle, szWindowClass, szTitle, _header->style,
       _header->placement.rcNormalPosition.left, _header->placement.rcNormalPosition.top, 
       _header->placement.rcNormalPosition.right - _header->placement.rcNormalPosition.left,
       _header->placement.rcNormalPosition.bottom - _header->placement.rcNormalPosition.top,
      NULL, NULL, hInstance, NULL);

   SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
   SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);

   SetWindowPlacement(hWnd, &_header->placement);

   if (!hWnd)
   {
      return FALSE;
   }
   _hWnd = hWnd;

   if (!_WndInfo.bLayered)
   {
       SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
   }

   ShowSysTray(hWnd, _header->show_systray);
   ShowInTaskbar(hWnd, !_header->hide_taskbar);

   if (_WndInfo.status != ds_normal)
   {
       OnDraw(hWnd);
   }

   //ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    LRESULT hit;
    POINT pt;
    RECT rc;
    int caption_h;
    int border_w;
    static RECT s_rect;

    if (message == _uFindReplaceMsg)
    {
        OnFindText(hWnd, message, wParam, lParam);
        return 0;
    }
    if (WM_TASKBAR_CREATED == message)
    {
        // Explorer may recreate taskbar buttons for visible top-level windows.
        // Reapply the user's taskbar preference before restoring any tray icon.
        ShowInTaskbar(hWnd, !_header->hide_taskbar);
        if (_header->show_systray)
        {
            // Explorer discarded the old icon, so force it to be added again.
            _nid.uFlags = 0;
            ShowSysTray(hWnd, TRUE);
        }
    }
    switch (message)
    {
    case WM_COMMAND:
        PauseAutoPage(hWnd);
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
        {
            int item_id = wmId - IDM_OPEN_BEGIN;
            OnOpenItem(hWnd, item_id, FALSE);
            ResumeAutoPage(hWnd);
            break;
        }
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_OPEN:
            OnOpenFile(hWnd, message, wParam, lParam);
            break;
        case IDM_CLEAR:
            OnClearFileList(hWnd, message, wParam, lParam);
            break;
        case IDM_DISPLAY:
            OpenDisplaySetDlg();
            break;
        case IDM_CONFIG:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTING), hWnd, Setting);
            break;
        case IDM_KEYSET:
            KS_OpenDlg();
            break;
        case IDM_ADVSET:
            ADV_OpenDlg(hInst, hWnd, &(_header->chapter_rule));
            break;
#if ENABLE_TAG
        case IDM_TAGSET:
            TS_OpenDlg();
            if (_Book && !_Book->IsLoading())
            {
                _Book->ReDraw(hWnd);
            }
            break;
#endif
        case IDM_DEFAULT:
            OnRestoreDefault(hWnd, message, wParam, lParam);
            break;
        case IDM_VIEW:
            if (IsWindowVisible(_hTreeView))
            {
                ShowWindow(_hTreeView, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading())
                {
                    TVITEM tvi;
                    HTREEITEM hti;

                    if (_Book->GetChapters()->empty())
                        break;
                    int index = _Book->GetCurChapterIndex();
                    if (index == -1)
                    {
                        break;
                    }
                    else if (index == 0)
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        TreeView_EnsureVisible(_hTreeView, hti);
                        TreeView_SelectItem(_hTreeView, hti);
                    }
                    else
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        do 
                        {
                            tvi.mask       = TVIF_PARAM;
                            tvi.hItem      = hti;
                            TreeView_GetItem(_hTreeView, &tvi);
                            if (tvi.lParam == index)
                            {
                                TreeView_EnsureVisible(_hTreeView, hti);
                                TreeView_SelectItem(_hTreeView, hti);
                                break;
                            }
                            hti = TreeView_GetNextSibling(_hTreeView, hti);
                        } while (hti);
                    }

                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeView, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeView);
                }
            }
            break;
        case IDM_MARK:
            if (IsWindowVisible(_hTreeMark))
            {
                ShowWindow(_hTreeMark, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading() && _item)
                {
                    if (_item->mark_size <= 0)
                        break;
                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeMark, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeMark);
                }
            }
            break;
        default:
            ResumeAutoPage(hWnd);
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ResumeAutoPage(hWnd);
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case TVN_SELCHANGED:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeView)
            {
                if (IsWindowVisible(_hTreeView))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeView);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeView, &item);
                        if (_Book && !_Book->IsLoading())
                            _Book->JumpChapter(hWnd, (int)item.lParam);
                        //ShowWindow(_hTreeView, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_VIEW, NULL);
                    }
                }
            }
            break;
        case NM_RCLICK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                HTREEITEM hItem = TreeView_GetNextItem(((LPNMHDR)lParam)->hwndFrom, 0, TVGN_DROPHILITE);
                if(hItem)
                    TreeView_SelectItem(((LPNMHDR)lParam)->hwndFrom, hItem);
            }
            break;
        case TVN_KEYDOWN:
            // Del key deletes the selected book mark
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark
                && ((LPNMTVKEYDOWN)lParam)->wVKey == VK_DELETE)
            {
                AM_DeleteBookMark(hWnd);
            }
            break;
        case NM_DBLCLK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                if (IsWindowVisible(_hTreeMark))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeMark);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeMark, &item);
                        if (_Book && !_Book->IsLoading() && _item)
                        {
                            _item->index = _item->mark[item.lParam];
                            _Book->ReDraw(hWnd);
                        }
                        //ShowWindow(_hTreeMark, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_MARK, NULL);
                    }
                }
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        if (wParam == (WPARAM)_hTreeMark)
        {
            HTREEITEM Selected;
            TVITEM item;
            pt.x = LOWORD(lParam); 
            pt.y = HIWORD(lParam);
            Selected = TreeView_GetSelection(_hTreeMark);
            if (Selected)
            {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu)
                {
                    InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_MK_DEL, _T("Delete"));
                    SetForegroundWindow(hWnd);
                    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hTreeMark, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_MK_DEL == ret)
                    {
                        AM_DeleteBookMark(hWnd);
                    }
                }
            }
        }
        break;
    case WM_MENURBUTTONUP:
        wmId = GetMenuItemID((HMENU)lParam, (int)wParam);
        if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
        {
            int item_id = wmId - IDM_OPEN_BEGIN;
            if (IDYES == MessageBoxFmt_(hWnd, IDS_WARN, MB_ICONINFORMATION | MB_YESNO, IDS_DELETE_FILE_CFM, _Cache.get_item(item_id)->file_name))
            {
                _Cache.delete_item(item_id);
                OnUpdateMenu(hWnd);
                if (item_id == 0 && _Book)
                {
                    // open the last file
                    if (_header->item_count > 0)
                    {
                        item_t* item = _Cache.get_item(0);
                        OnOpenBook(hWnd, item->file_name, TRUE);
                    }
                    else
                    {
                        if (_Book)
                        {
                            delete _Book;
                            _Book = NULL;
                        }
                        PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);
                        Invalidate(hWnd, TRUE, FALSE);
                    }
                }
                break;
            }
            break;
        }
        break;
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        if (!_WndInfo.bLayered)
            OnPaint(hWnd, hdc);
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        StopAutoPage(hWnd);
        if (_header->show_systray)
        {
            _tcscpy(_nid.szInfoTitle, szTitle);
            _tcscpy(_nid.szInfo, _T("I'm here!"));
            _nid.uTimeout = 1000;
            Shell_NotifyIcon(NIM_MODIFY, &_nid);
            ShowHideWindow(hWnd);
        }
        else
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:
        _free_resource(hWnd);
        _update_data(hWnd, FALSE, FALSE);
        PostQuitMessage(0);
        break;
    case WM_NCHITTEST:
        hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT && _WndInfo.status == ds_borderless)
        {
            border_w = GetWidthForDpi(8);
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            GetClientRectExceptStatusBar(hWnd, &rc);

            if (pt.x < rc.left + border_w)
            {
                if (pt.y < rc.top + border_w)
                    hit = HTTOPLEFT;
                else if (pt.y > rc.bottom - border_w)
                    hit = HTBOTTOMLEFT;
                else
                    hit = HTLEFT;
            }
            else if (pt.x > rc.right - border_w)
            {
                if (pt.y < rc.top + border_w)
                    hit = HTTOPRIGHT;
                else if (pt.y > rc.bottom - border_w)
                    hit = HTBOTTOMRIGHT;
                else
                    hit = HTRIGHT;
            }
            else if (pt.y < rc.top + border_w)
            {
                hit = HTTOP;
            }
            else if (pt.y > rc.bottom - border_w)
            {
                hit = HTBOTTOM;
            }
            else
            {
                if (_header->page_mode == 2)
                {
                    rc.left = rc.right/3;
                    rc.right = rc.right/3 * 2;
                    if (PtInRect(&rc, pt))
                        hit = HTCAPTION;
                }
                else
                {
                    caption_h = GetWidthForDpi(60);
                    rc.bottom = rc.bottom/2 > caption_h ? caption_h : rc.bottom/2;
                    if (PtInRect(&rc, pt))
                        hit = HTCAPTION;
                }
            }
        }
        return hit;
    case WM_SIZING:
        if (EC_IsEditMode())
        {
            RECT *lprc = NULL;
            lprc = (RECT *)lParam;
            lprc->left = s_rect.left;
            lprc->top = s_rect.top;
            lprc->right = s_rect.right;
            lprc->bottom = s_rect.bottom;
            return TRUE;
        }
        break;
    case WM_SIZE:
        if (!IsIconic(hWnd))
            OnSize(hWnd, message, wParam, lParam);
        GetWindowRect(hWnd, &s_rect);
        ResetAutoPage(hWnd);
        break;
    case WM_KEYDOWN:
#if ENABLE_GLOBAL_KEY
        if (_hKeyboardHook)
            break;
#endif

        if (_Book && _Book->IsLoading())
            break;

        if (KS_KeyDownProc(hWnd, message, wParam, lParam))
            break;

        if (VK_ESCAPE == wParam)
        {
            if (_WndInfo.status == ds_fullscreen)
            {
                OnFullScreen(hWnd, message, wParam, lParam);
            }
            else
            {
                if (_header->disable_eschide == 0)
                    OnHideWin(hWnd, message, wParam, lParam);
            }
        }
        else if (AM_KeyDown(hWnd, message, wParam, lParam))
        {
        }
        else if ('P' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000
                && GetAsyncKeyState(VK_SHIFT) & 0x8000
                && GetAsyncKeyState(18) & 0x8000) // alt
            {
                if (!_hMouseHook)
                    _hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInst, NULL);
                else
                {
                    UnhookWindowsHookEx(_hMouseHook);
                    _hMouseHook = NULL;
                }
            }
        }
        break;
    case WM_HOTKEY:
        KS_HotKeyProc(hWnd, message, wParam, lParam);
        break;
    case WM_XBUTTONDOWN:
        // mouse side keys 4/5 page turning (aaron customization)
        AM_XButtonDown(hWnd, message, wParam, lParam);
        break;
    case WM_LBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageDown(hWnd, message, wParam, lParam);
        }
        else if (_header->page_mode == 2)
        {
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            GetClientRectExceptStatusBar(hWnd, &rc);
            rc.left = rc.right/3 * 2;
            if (PtInRect(&rc, pt))
            {
                OnPageDown(hWnd, message, wParam, lParam);
            }
            else
            {
                rc.left = 0;
                rc.right = rc.right/3;
                if (PtInRect(&rc, pt))
                {
                    OnPageUp(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageUp(hWnd, message, wParam, lParam);
        }
        break;
    case WM_NCLBUTTONDOWN:
        if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_hMouseHook)
        {
            switch (wParam)
            {
            case HTMINBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return FALSE;
            case HTMAXBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                return FALSE;
            case HTCLOSE:
                UnhookWindowsHookEx(_hMouseHook);
                _hMouseHook = NULL;
                break;
            default:
                break;
            }
        }
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_NCRBUTTONDOWN:
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && _header->disable_lrhide == 0) //not work
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_MOUSEWHEEL:
        {
            const BYTE MIN_ALPHA = 0x01;
            const BYTE MAX_ALPHA = 0xff;
            const BYTE UNIT_STEP = 0x05;
            if (IsWindowVisible(_hTreeView))
            {
                SetFocus(_hTreeView);
                break;
            }
            if (IsWindowVisible(_hTreeMark))
            {
                SetFocus(_hTreeMark);
                break;
            }
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MAX_ALPHA;
                    }
                    else
                    {
                        if (_header->alpha < MAX_ALPHA - UNIT_STEP)
                            _header->alpha += UNIT_STEP;
                        else
                            _header->alpha = MAX_ALPHA;
                    }
                    if (_WndInfo.bLayered)
                        OnDraw(hWnd);
                    else
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha < MAX_ALPHA - UNIT_STEP)
                        _textAlpha += UNIT_STEP;
                    else
                        _textAlpha = MAX_ALPHA;
                    if (_WndInfo.bLayered)
                        OnDraw(hWnd);
                }
                else
                {
                    OnLineUp(hWnd, message, wParam, lParam);
                }
            }
            else
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MIN_ALPHA;
                    }
                    else
                    {
                        if (_header->alpha > MIN_ALPHA + UNIT_STEP)
                            _header->alpha -= UNIT_STEP;
                        else
                            _header->alpha = MIN_ALPHA;
                    }
                    if (_WndInfo.bLayered)
                        OnDraw(hWnd);
                    else
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha > 0x1f + UNIT_STEP)
                        _textAlpha -= UNIT_STEP;
                    else
                        _textAlpha = 0x1f;
                    if (_WndInfo.bLayered)
                        OnDraw(hWnd);
                }
                else
                {
                    OnLineDown(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_DROPFILES:
        OnDropFiles(hWnd, message, wParam, lParam);
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case IDT_TIMER_PAGE:
            if ((_header->autopage_mode & 0x0f) == apm_page)
                OnPageDown(hWnd, message, wParam, lParam);
            else
                OnLineDown(hWnd, message, wParam, lParam);
            if (_Book && _Book->IsLastPage())
                StopAutoPage(hWnd);
            else
                ResetAutoPage(hWnd);
            break;
#ifdef ENABLE_NETWORK
        case IDT_TIMER_UPGRADE:
            _Upgrade.Check(UpgradeCallback, hWnd);
            break;
#endif
        case IDT_TIMER_LOADING:
            {
                GUID Guid;
                KillTimer(hWnd, IDT_TIMER_LOADING);
                Guid = Gdiplus::FrameDimensionTime;
                _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);
                SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)(_loading->item[0].value))[_loading->frameIndex] * 10, NULL);
                _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
                Invalidate(hWnd, TRUE, FALSE);
            }
            break;
        default:
            break;
        }
        break;
    case WM_UPDATE_CHAPTERS:
        OnUpdateChapters(hWnd);
        OnUpdateBookMark(hWnd);
        break;
#ifdef ENABLE_NETWORK
    case WM_NEW_VERSION:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_UPGRADE), hWnd, UpgradeProc);
        break;
#endif
    case WM_OPEN_BOOK:
        OnOpenBookResult(hWnd, wParam == 1);
        break;
    case WM_COPYDATA:
        OnCopyData(hWnd, message, wParam, lParam);
        break;
    case WM_BOOK_EVENT:
        {
            book_event_data_t* be = (book_event_data_t*)lParam;
            if (_Book && (!be || _Book == be->_this))
                _Book->OnBookEvent(hWnd, message, wParam, lParam);
        }
        break;
    case WM_SAVE_CACHE:
        OnSave(hWnd);
        break;
    case WM_SYSTRAY:
        switch(lParam)
        {
        case WM_LBUTTONUP:
            if (!IsWindowVisible(hWnd))
                ShowWindow(hWnd, SW_SHOW);
            if (IsIconic(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            {
                int ret;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                if(hMenu)
                {
                    InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_ST_OPEN, _T("Open"));
                    InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_ST_EXIT, _T("Exit"));
                    SetForegroundWindow(hWnd);
                    ret = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_ST_OPEN == ret)
                    {
                        if (!IsWindowVisible(hWnd))
                            ShowWindow(hWnd, SW_SHOW);
                        if (IsIconic(hWnd))
                            ShowWindow(hWnd, SW_RESTORE);
                        SetForegroundWindow(hWnd);
                    }
                    else if (IDM_ST_EXIT == ret)
                    {
                        DestroyWindow(hWnd);
                    }
                }
            }
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            hdc = (HDC)wParam;
            SetTextColor(hdc, _header->font_color);
            SetBkColor(hdc, _header->bg_color);
            SetDCBrushColor(hdc, _header->bg_color);
            return (LRESULT) GetStockObject(DC_BRUSH);
        }
        break;
    case WM_QUERYENDSESSION:
        //Exit(); // save data when poweroff ?
        return TRUE;
    case WM_ENDSESSION:
        _free_resource(hWnd);
        _update_data(hWnd, FALSE, FALSE);
        Exit();
        return 0;
        break;
    case 0x02E0: //WM_DPICHANGED
        {            
            NONCLIENTMETRICS theMetrics;
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
            HFONT hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
            SendMessage(_hTreeView, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            DpiChanged(hWnd, &_header->font, &_header->placement.rcNormalPosition, wParam, (RECT*)lParam);
            DpiChanged(hWnd, &_header->font, &_header->fs_placement.rcNormalPosition, wParam, (RECT*)lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
// Message handler for about box.
#ifdef ENABLE_NETWORK
INT_PTR CALLBACK UpgradeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    json_item_data_t *vinfo;
    switch (message)
    {
    case WM_INITDIALOG:
        vinfo = _Upgrade.GetVersionInfo();
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_VERSION), vinfo->version.c_str());
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_DESC), vinfo->desc.c_str());
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON_UPGRADE_INGORE:
            vinfo = _Upgrade.GetVersionInfo();
            wcscpy(_header->ingore_version, vinfo->version.c_str());
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_UPGRADE_DOWN:
            vinfo = _Upgrade.GetVersionInfo();
            ShellExecute(NULL, _T("open"), vinfo->url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif

LRESULT OnCreate(HWND hWnd)
{
    LPWSTR* argv;
    int argc;
    BOOL flag = TRUE;
    
    _WndInfo.bLayered = (_header->style & WS_CAPTION) == 0;
    _WndInfo.bLayered_bak = _WndInfo.bLayered;
    _WndInfo.bTopMost = (_header->exstyle & WS_EX_TOPMOST) == WS_EX_TOPMOST;
    _WndInfo.status = (_header->style & WS_MINIMIZEBOX) == 0 ? ds_fullscreen : (((_header->style & WS_CAPTION) == 0) ? ds_borderless : ds_normal);

    _WndInfo.hMenu = GetMenu(hWnd);
    // create status bar
    _WndInfo.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _T("Please open a text."), hWnd, IDC_STATUSBAR);
    // register find text dialog
    _uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
    // create chapters tree view
    _hTreeView = CreateWindow(WC_TREEVIEW, _T("Tree View"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    _hTreeMark = CreateWindow(WC_TREEVIEW, _T("Tree Mark"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL /*| TVS_NOTOOLTIPS*/ | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    //TreeView_SetBkColor(_hTreeView, GetSysColor(COLOR_MENU));
    //TreeView_SetBkColor(_hTreeMark, GetSysColor(COLOR_MENU));
    SetTreeviewFont();

    RemoveMenus(hWnd, FALSE);
    OnUpdateMenu(hWnd);

    // open file
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1)
    {
        // open input argument file name
        TCHAR fullname[MAX_PATH];
        GetFullPathName(argv[1], MAX_PATH, fullname, NULL);
        // check if file vaild
        if (IsVaildFile(NULL, fullname, NULL))
        {
            OnOpenBook(hWnd, fullname, FALSE);
            flag = FALSE;
        }
    }
    if (flag)
    {
        // open the last file
        if (_header->item_count > 0)
        {
            item_t* item = _Cache.get_item(0);
            OnOpenBook(hWnd, item->file_name, FALSE);
        }
    }

    // check upgrade
#ifdef ENABLE_NETWORK
    CheckUpgrade(hWnd);
#endif

    // init keyset
    KS_Init(hWnd, _header->keyset);

    if (_WndInfo.status > ds_normal)
    {
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);
        SetMenu(hWnd, NULL);
    }
    return 0;
}

LRESULT OnUpdateMenu(HWND hWnd)
{
    static Gdiplus::Bitmap* s_bitmap = NULL;
    static HGLOBAL s_hMemory = NULL;
    static HBITMAP hBitmap = NULL;
    TCHAR buf[MAX_LOADSTRING];
    int menu_begin_id = IDM_OPEN_BEGIN;
    HMENU hMenuBar = GetMenu(hWnd);
    HMENU hFile = GetSubMenu(hMenuBar, 0);
    MENUITEMINFO mi = { 0 };

    if (!hMenuBar)
    {
        _menuInvalid = TRUE;
        return 0;
    }

    if (hFile)
    {
        DeleteMenu(hMenuBar, 0, MF_BYPOSITION);
        hFile = NULL;
    }
    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
    if (!s_bitmap)
    {
        LoadResourceImage(MAKEINTRESOURCE(IDB_PNG_NEW), _T("PNG"), &s_bitmap, &s_hMemory);
    }
    if (s_bitmap)
    {
        s_bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
    }
    hFile = CreateMenu();
    LoadString(hInst, IDS_MENU_OPEN, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_OPEN, buf);
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        AppendMenu(hFile, MF_STRING, (UINT_PTR)menu_begin_id, item->file_name);
        menu_begin_id++;
    }
    if (_header->item_count > 0)
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    LoadString(hInst, IDS_MENU_CLEAR, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_CLEAR, buf);
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    LoadString(hInst, IDS_MENU_EXIT, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_EXIT, buf);
    LoadString(hInst, IDS_MENU_FILE, buf, MAX_LOADSTRING);
    InsertMenu(hMenuBar, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hFile, buf);
    DrawMenuBar(hWnd);

    return 0;
}

LRESULT OnOpenItem(HWND hWnd, int item_id, BOOL forced)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    if (!forced && _item && _item->id == item_id && _Book && !_Book->IsLoading())
    {
        return 0;
    }

    BOOL bNotExist = FALSE;
    item_t* item = _Cache.get_item(item_id);
    if (!item)
    {
        bNotExist = TRUE;
    }
    else
    {
        if (PathFileExists(item->file_name))
        {
            OnOpenBook(hWnd, item->file_name, forced);
        }
        else
        {
            bNotExist = TRUE;
        }
    }

    if (bNotExist)
    {
        _Cache.delete_item(item_id);
        // update menu
        OnUpdateMenu(hWnd);
        return 0L;
    }

    return 0;
}

LRESULT OnClearFileList(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_header->item_count <= 0)
    {
        return 0;
    }

    if (IDYES != MessageBox_(hWnd, IDS_CLEAR_ALL_FILES, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
    {
        return 0;
    }

    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }
    _Cache.delete_all_item();
    OnUpdateMenu(hWnd);
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);
    Invalidate(hWnd, TRUE, FALSE);
    SetWindowText(hWnd, szTitle);
    StopLoadingImage(hWnd);
    StopAutoPage(hWnd);
    Save(hWnd);
    return 0;
}

LRESULT OnRestoreDefault(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int lastrule = _header->chapter_rule.rule;
    int lastmenufont = _header->meun_font_follow;

    if (IDNO == MessageBox_(hWnd, IDS_RESTORE_DEFAULT, IDS_WARN, MB_ICONINFORMATION|MB_YESNO))
    {
        return 0;
    }

    KS_UnRegisterAllHotKey(hWnd);
    _Cache.default_header();
    UpdateRectForDpi(&_header->placement.rcNormalPosition);
    UpdateRectForDpi(&_header->fs_placement.rcNormalPosition);
    UpdateFontForDpi(&_header->font);
#if 0 // needn't
#if ENABLE_TAG
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        UpdateFontForDpi(hWnd, &_header->tags[i].font);
    }
#endif
#endif
    SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);

    KS_RegisterAllHotKey(hWnd);
    SetGlobalKey(hWnd);
    ShowSysTray(hWnd, _header->show_systray);
    ShowInTaskbar(hWnd, !_header->hide_taskbar);

    SetWindowPlacement(hWnd, &_header->placement);

    if (!_WndInfo.bLayered)
    {
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
    }

    if (lastrule != _header->chapter_rule.rule)
    {
        // reload book
        OnOpenItem(hWnd, 0, TRUE);
    }
    else
    {
        if (_Book)
            _Book->ReDraw(hWnd);
    }
    if (lastmenufont != _header->meun_font_follow)
        SetTreeviewFont();

    Save(hWnd);
    return 0;
}

LRESULT OnPaint(HWND hWnd, HDC hdc)
{
    RECT rc;
    HBRUSH hBrush = NULL;
    HDC memdc = NULL;
    HBITMAP hBmp = NULL;
    Gdiplus::Bitmap *image;
    Gdiplus::Graphics *g = NULL;
    Gdiplus::Rect rect;
    UINT w,h;
    double scale;

    GetClientRectExceptStatusBar(hWnd, &rc);

    // memory dc
    memdc = CreateCompatibleDC(hdc);

    // load bg image
    image = LoadBGImage(rc.right-rc.left,rc.bottom-rc.top);
    if (image)
    {
        image->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hdc, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);

        // set bg color
        hBrush = CreateSolidBrush(_header->bg_color);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }

    if (_Book && !_Book->IsLoading())
    {
        _Book->DrawPage(hWnd, memdc, &rc, FALSE);
    }
    if (_loading && _loading->enable)
    {
        g = new Gdiplus::Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), Gdiplus::UnitPixel);
    }

    BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, memdc, rc.left, rc.top, SRCCOPY);

    DeleteObject(hBmp);
    DeleteObject(hBrush);
    DeleteDC(memdc);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return 0;
}

VOID OnDraw(HWND hWnd)
{
    RECT rc;
    HDC hdc_screen = NULL;
    HDC hdc_text = NULL;
    HDC memdc = NULL;
    HBITMAP hbitmap_bg = NULL;
    Gdiplus::Bitmap *image;
    Gdiplus::Graphics *g = NULL;
    Gdiplus::Rect rect;
    INT w,h;
    double scale;    
    BYTE alpha = _header->alpha;
    BOOL is_blank = TRUE;

    GetClientRectExceptStatusBar(hWnd, &rc);

    w = rc.right-rc.left;
    h = rc.bottom-rc.top;

    // memory dc
    hdc_screen = GetDC(NULL);
    memdc = CreateCompatibleDC(hdc_screen);

    // draw text to dc, DrawPage() will create bitmap
    if (_Book && !_Book->IsLoading())
    {
        hdc_text = CreateCompatibleDC(hdc_screen);
        _Book->DrawPage(hWnd, hdc_text, &rc, TRUE);
        is_blank = _Book->IsBlankPage();
    }

    if (is_blank)
    {
        alpha = _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha;
    }

    // load bg image
    image = LoadBGImage(w, h, alpha);
    if (image)
    {
        image->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbitmap_bg);
        SelectObject(memdc, hbitmap_bg);
    }
    else
    {
        hbitmap_bg = CreateAlphaBGColorBitmap(hWnd, _header->bg_color, rc.right-rc.left, rc.bottom-rc.top, alpha);
        SelectObject(memdc, hbitmap_bg);
    }
    
    if (_loading && _loading->enable)
    {
        g = new Gdiplus::Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), Gdiplus::UnitPixel);
    }

    // alpha blend text to backgroud image
    if (hdc_text)
    { 
        BLENDFUNCTION bf;
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.SourceConstantAlpha = 0xFF; 
        bf.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(memdc, 0, 0, w, h, hdc_text, 0, 0, w, h, bf);

        // Clean up 
        DeleteDC(hdc_text); 
    }

    // update layered
    RECT winRect;
    GetWindowRect(hWnd, &winRect);
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 0xFF;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptPos = {winRect.left, winRect.top};
    SIZE sizeWnd = {winRect.right - winRect.left, winRect.bottom - winRect.top};
    POINT ptSrc = {0, 0};
    UpdateLayeredWindow(hWnd, hdc_screen, &ptPos, &sizeWnd, memdc, &ptSrc, 0, &blend, ULW_ALPHA);
    

    DeleteObject(hbitmap_bg);
    DeleteDC(memdc);
    ReleaseDC(NULL, hdc_screen);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return;
}

BOOL ResetLayerd(HWND hWnd)
{
    LONG_PTR exstyle;

    if (_WndInfo.bLayered_bak != _WndInfo.bLayered)
    {
        exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        exstyle &= ~WS_EX_LAYERED;
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);
        exstyle |= WS_EX_LAYERED;
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);
        _WndInfo.bLayered_bak = _WndInfo.bLayered;
        return TRUE;
    }

    return FALSE;
}

VOID Invalidate(HWND hWnd, BOOL bOnlyClient, BOOL bErase)
{
    RECT rect;
    if (_WndInfo.bLayered)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        if (ResetLayerd(hWnd))
        {
            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        }
        if (bOnlyClient)
        {
            GetClientRectExceptStatusBar(hWnd, &rect);
            InvalidateRect(hWnd, &rect, bErase);
        }
        else
        {
            InvalidateRect(hWnd, NULL, bErase);
        }
    }
}

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_WndInfo.status == ds_borderless)
    {
        OnDraw(hWnd);
    }
    SendMessage(_WndInfo.hStatusBar, message, wParam, lParam);
    return 0;
}

LRESULT OnHideWin(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ShowHideWindow(hWnd);
    return 0;
}

static void CorrectWindowContentRect(HWND hWnd, const RECT *target, int excludedBottom, UINT flags)
{
    RECT actualContent;
    RECT actualWindow;

    for (int i = 0; i < 3; i++)
    {
        GetClientRect(hWnd, &actualContent);
        actualContent.bottom -= excludedBottom;
        ClientToScreen(hWnd, reinterpret_cast<POINT*>(&actualContent.left));
        ClientToScreen(hWnd, reinterpret_cast<POINT*>(&actualContent.right));

        int offsetX = target->left - actualContent.left;
        int offsetY = target->top - actualContent.top;
        int offsetWidth = (target->right - target->left) - (actualContent.right - actualContent.left);
        int offsetHeight = (target->bottom - target->top) - (actualContent.bottom - actualContent.top);

        if (offsetX == 0 && offsetY == 0 && offsetWidth == 0 && offsetHeight == 0)
            break;

        GetWindowRect(hWnd, &actualWindow);
        SetWindowPos(hWnd, NULL,
            actualWindow.left + offsetX,
            actualWindow.top + offsetY,
            actualWindow.right - actualWindow.left + offsetWidth,
            actualWindow.bottom - actualWindow.top + offsetHeight,
            flags);
    }
}

LRESULT OnHideBorder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rcWin;
    RECT rcContent;
    RECT rcStatus;
    Book *pBook = NULL;
    WNDCLASSEX wcex;
    static HICON hIconSm = NULL;
    const UINT windowPosFlags = SWP_NOREDRAW;

    if (hIconSm == NULL)
    {
        GetClassInfoEx(hInst, szWindowClass, &wcex);
        hIconSm = wcex.hIconSm;
    }
    
    if (_WndInfo.status == ds_fullscreen)
        return 0;

    GetClientRectExceptStatusBar(hWnd, &rcWin);
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rcWin.left));
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rcWin.right));
    rcContent = rcWin;

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0); // lock redraw
    pBook = _Book;_Book = NULL; // lock onsize
    if (_WndInfo.status == ds_normal) // hide border
    {
        _header->style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE) & (~(WS_CAPTION | WS_THICKFRAME | /*WS_MINIMIZEBOX |*/ WS_MAXIMIZEBOX | WS_SYSMENU)) | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        _header->exstyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE) & (~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

        SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);
        SetMenu(hWnd, NULL);
        _WndInfo.bLayered = TRUE;
        _WndInfo.status = ds_borderless;
        SetWindowPos(hWnd, NULL, rcWin.left, rcWin.top, rcWin.right-rcWin.left, rcWin.bottom-rcWin.top, windowPosFlags);
        CorrectWindowContentRect(hWnd, &rcContent, 0, windowPosFlags);
    }
    else if (_WndInfo.status == ds_borderless)// show border
    {
        _header->style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW;
        _header->exstyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_LAYERED;

        GetWindowRect(_WndInfo.hStatusBar, &rcStatus);
        rcWin = rcContent;
        rcWin.bottom += rcStatus.bottom - rcStatus.top;
        AdjustWindowRectEx(&rcWin, _header->style, TRUE, _header->exstyle);

        SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm); // fixed bug
        ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
        SetMenu(hWnd, _WndInfo.hMenu);
        _WndInfo.bLayered = FALSE;
        _WndInfo.status = ds_normal;
        SetWindowPos(hWnd, NULL, rcWin.left, rcWin.top, rcWin.right-rcWin.left, rcWin.bottom-rcWin.top, windowPosFlags);
        CorrectWindowContentRect(hWnd, &rcContent, rcStatus.bottom - rcStatus.top, windowPosFlags);

        if (_menuInvalid)
        {
            _menuInvalid = FALSE;
            OnUpdateMenu(hWnd);
        }
    }
    _Book = pBook;
    
    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0); // lock redraw
    // repaint for alpha
    Invalidate(hWnd, FALSE, FALSE);
    return 0;
}

LRESULT OnFullScreen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MONITORINFO mi;
    RECT rc;

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0); // lock redraw
    if (_WndInfo.status < ds_fullscreen) // fullscreen
    {
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),&mi);
        rc = mi.rcMonitor;

        _header->fs_style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
        _header->fs_exstyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        GetWindowPlacement(hWnd, &_header->fs_placement);

        if ((WS_MAXIMIZE & _header->fs_style) == WS_MAXIMIZE)
        {
            GetWindowRect(hWnd, &_header->fs_rect);
        }

        _header->style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE) & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
        _header->exstyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE) & (~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

        SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);
        SetMenu(hWnd, NULL);
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_DRAWFRAME);
        _WndInfo.bLayered = TRUE;
        _WndInfo.status = ds_fullscreen;
    }
    else // exit fullscreen
    {
        _header->style = _header->fs_style;
        _header->exstyle = _header->fs_exstyle;
        SetWindowLongPtr(hWnd, GWL_STYLE, _header->style);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _header->exstyle);
        if ((WS_CAPTION & _header->style) == WS_CAPTION)
        {
            ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
            SetMenu(hWnd, _WndInfo.hMenu);
            _WndInfo.bLayered = FALSE;
            _WndInfo.status = ds_normal;
        }
        else
        {
            _WndInfo.bLayered = TRUE;
            _WndInfo.status = ds_borderless;
            if ((WS_MAXIMIZE & _header->fs_style) == WS_MAXIMIZE)
            {
                SetWindowPos(hWnd, NULL, _header->fs_rect.left, _header->fs_rect.top, 
                    _header->fs_rect.right-_header->fs_rect.left, _header->fs_rect.bottom-_header->fs_rect.top, SWP_DRAWFRAME);
            }
        }
        _header->placement = _header->fs_placement;
        SetWindowPlacement(hWnd, &_header->fs_placement);
    }

    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0); // lock redraw
    // repaint for alpha
    Invalidate(hWnd, FALSE, FALSE);

    return 0;
}

LRESULT OnTopmost(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SetWindowPos(hWnd, _WndInfo.bTopMost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    _WndInfo.bTopMost = !_WndInfo.bTopMost;
    return 0;
}

LRESULT OnOpenFile(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    TCHAR szFileName[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);  
    ofn.hwndOwner = hWnd;  
    ofn.lpstrFilter = _T("Files (*.txt;*.epub;*.mobi)\0*.txt;*.epub;*.mobi\0\0");
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    BOOL bSel = GetOpenFileName(&ofn);
    if (!bSel)
    {
        return 0;
    }

    OnOpenBook(hWnd, szFileName, FALSE);
    return 0;
}

LRESULT OnAddMark(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int ret;
    if (_Book && !_Book->IsLoading())
    {
        ret = MessageBox_(hWnd, IDS_ADD_BOOKMARK_TIPS, IDS_BOOKMARK, MB_ICONINFORMATION|MB_YESNO);
        if (ret == IDYES)
        {
            if (_Cache.add_mark(_item, _item->index))
            {
                OnUpdateBookMark(hWnd);
                Save(hWnd);
            }
        }
    }
    return 0;
}

LRESULT OnAutoPage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        if (_IsAutoPage)
        {
            StopAutoPage(hWnd);
        }
        else
        {
            StartAutoPage(hWnd);
        }
    }
    return 0;
}

LRESULT OnSearch(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        OnFindText(hWnd, message, wParam, lParam);
    return 0;
}

LRESULT OnJump(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        DialogBox(hInst, MAKEINTRESOURCE(IDD_JUMP_PROGRESS), hWnd, JumpProgress);
    return 0;
}

int Editwordbreakproc(
    LPTSTR lpch,
    int ichCurrent,
    int cch,
    int code
    )
{
    if (WB_ISDELIMITER == code && cch == 0x0a)
    {
        return TRUE;
    }
    return FALSE;
}

LRESULT OnEditMode(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR *text = NULL;
    BOOL readonly = TRUE;

    if (EC_IsEditMode())
    {
        EC_LeaveEditMode();
        return 0;
    }

    if (_Book && !_Book->IsLoading())
    {
        if (!_Book->IsCoverPage() && _Book->GetCurPageText(&text))
        {
            _WndInfo.status_bak = _WndInfo.status;
            if (_WndInfo.status == ds_fullscreen)
            {
                OnFullScreen(hWnd, message, wParam, lParam);
            }

            if (_WndInfo.status == ds_borderless)
            {
                OnHideBorder(hWnd, message, wParam, lParam);
            }

            if (_Book->GetBookType() == book_text)
            {
                readonly = FALSE;
            }

            EC_EnterEditMode(hInst, hWnd, &_header->font, text, readonly);

            free(text);
        }
    }

    return 0;
}

LRESULT OnPageUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->PageUp(hWnd);
    }
    return 0;
}

LRESULT OnPageDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->PageDown(hWnd);
    }
    return 0;
}

LRESULT OnLineUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->LineUp(hWnd);
    }
    return 0;
}

LRESULT OnLineDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->LineDown(hWnd);
    }
    return 0;
}

LRESULT OnChapterUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->JumpPrevChapter(hWnd);
        Save(hWnd);
    }
    return 0;
}

LRESULT OnChapterDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->JumpNextChapter(hWnd);
        Save(hWnd);
    }
    return 0;
}

LRESULT OnFontZoomIn(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //const int ZOOM_MIN = 1;
    const int ZOOM_MAX = 72;
    int PointSize;

    if (_header && _Book && !_Book->IsLoading())
    {
        PointSize = abs(MulDiv(_header->font.lfHeight, 72, GetDeviceCaps(GetDC(NULL), LOGPIXELSY)));
        if (PointSize < ZOOM_MAX)
        {
            PointSize++;
            _header->font.lfHeight = -MulDiv(PointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
            _Book->ReDraw(hWnd);
            Save(hWnd);
        }
    }
    return 0;
}

LRESULT OnFontZoomOut(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const int ZOOM_MIN = 1;
    //const int ZOOM_MAX = 72;
    int PointSize;

    if (_header && _Book && !_Book->IsLoading())
    {
        PointSize = abs(MulDiv(_header->font.lfHeight, 72, GetDeviceCaps(GetDC(NULL), LOGPIXELSY)));
        if (PointSize > ZOOM_MIN)
        {
            PointSize--;
            _header->font.lfHeight = -MulDiv(PointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
            _Book->ReDraw(hWnd);
            Save(hWnd);
        }
    }
    return 0;
}

LRESULT OnDropFiles(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDROP hDropInfo = (HDROP)wParam;
    UINT  nFileCount = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
    TCHAR szFileName[MAX_PATH] = _T("");
    DWORD dwAttribute;
    TCHAR *ext = NULL;

    for (UINT i = 0; i < nFileCount; i++)
    {
        ::DragQueryFile(hDropInfo, i, szFileName, sizeof(szFileName));
        dwAttribute = ::GetFileAttributes(szFileName);

        if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {          
            continue;
        }

        // check is txt file
        ext = PathFindExtension(szFileName);
        if (ext && _tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")) && _tcscmp(ext, _T(".mobi")))
        {
            continue;
        }

        // open file
        OnOpenBook(hWnd, szFileName, FALSE);

        // just open first file
        break;
    }

    ::DragFinish(hDropInfo);
    return 0;
}

LRESULT OnFindText(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static FINDREPLACE fr;       // common dialog box structure
    static TCHAR szFindWhat[80] = {0}; // buffer receiving string
    int len;

    if (message == _uFindReplaceMsg)
    {
        // do search
        if (!_Book)
            return 0;
        len = (int)_tcslen(szFindWhat);
        if (fr.Flags & FR_DIALOGTERM)
        {
            // close dlg
            DestroyWindow(_hFindDlg);
            _hFindDlg = NULL;
        }
        else if (fr.Flags & FR_DOWN) // back search
        {
            for (int i=_item->index+1; i<_Book->GetTextLength()-len+1; i++)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->ReDraw(hWnd);
                    Save(hWnd);
                    break;
                }
            }
        }
        else // front search
        {
            for (int i=_item->index-1; i>=0; i--)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->ReDraw(hWnd);
                    Save(hWnd);
                    break;
                }
            }
        }
    }
    else
    {
        if (!IsWindow(_hFindDlg))
        {
            // Initialize FINDREPLACE
            ZeroMemory(&fr, sizeof(fr));
            fr.lStructSize = sizeof(fr);
            fr.hwndOwner = hWnd;
            fr.hInstance = hInst;
            fr.lpstrFindWhat = szFindWhat;
            fr.wFindWhatLen = 80;
            fr.Flags = FR_DOWN;

            _hFindDlg = FindText(&fr);
        }
    }

    return 0;
}

LRESULT OnUpdateChapters(HWND hWnd)
{
    chapters_t *chapters;
    int i;

    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    
    TreeView_DeleteAllItems(_hTreeView);
    if (_Book)
    {
        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */| TVIF_PARAM;

        chapters = _Book->GetChapters();
        for (i = 0; i < (int)chapters->size(); i++)
        {
            tvi.pszText = (TCHAR*)(*chapters)[i].title.c_str();
            tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
            tvi.lParam = (LPARAM)i;
            tvins.item = tvi;
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            // Add the item to the tree-view control. 
            hPrev = (HTREEITEM)SendMessage(_hTreeView, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }

    return 0;
}


LRESULT OnOpenBookResult(HWND hWnd, BOOL result)
{
    item_t *item = NULL;
    UINT type;
    TCHAR fileName[MAX_PATH] = { 0 };
    
    //EnableWindow(hWnd, TRUE);
    if (!result)
    {
        StopLoadingImage(hWnd);
        _tcscpy(fileName, _Book->GetFileName());
        type = MB_OK;
        delete _Book;
        _Book = NULL;
        if (IDRETRY == MessageBox_(hWnd, IDS_OPEN_FILE_FAILED, IDS_ERROR, type | MB_ICONERROR))
        {
            OnOpenBook(hWnd, fileName, FALSE);
        }
        // Update title
        UpdateTitle(hWnd);
        Invalidate(hWnd, TRUE, FALSE);
        return FALSE;
    }

    // create new item
    item = _Cache.find_item(_Book->GetFileName());
    if (NULL == item)
    {
        item = _Cache.new_item(_Book->GetFileName());
        _header = _Cache.get_header(); // after realloc the header address has been changed
    }

    // open item
    _item = _Cache.open_item(item->id);

    // set param
    _Book->Init(&_item->index, _header);
    _Book->SetChapterRule(&(_header->chapter_rule));

    // update menu
    OnUpdateMenu(hWnd);

    // Update title
    UpdateTitle(hWnd);

    // update chapter
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);

    // save
    Save(hWnd);

    // stoploading & repaint
    StopLoadingImage(hWnd);

    return 0;
}

LRESULT OnCopyData(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT*)lParam;
    TCHAR fullname[MAX_PATH];

    switch (pCopyData->dwData)
    {
    case 0: // extern open file
        GetFullPathName((TCHAR*)pCopyData->lpData, MAX_PATH, fullname, NULL);
        // check if file vaild
        if (IsVaildFile(NULL, fullname, NULL))
        {
            OnOpenBook(hWnd, fullname, FALSE);
        }
        break;
    }

    return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LPMOUSEHOOKSTRUCT mh = NULL;
    RECT rect;

    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_MOUSEMOVE:
            mh = (LPMOUSEHOOKSTRUCT)lParam;
            GetWindowRect(_hWnd, &rect);
            if (PtInRect(&rect, mh->pt))
            {
                if (!IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_SHOW);
            }
            else
            {
                if (IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_HIDE);
            }
            break;
        default:
            break;
        }
    }
    
    return CallNextHookEx(_hMouseHook, nCode, wParam, lParam);
}

#if ENABLE_GLOBAL_KEY
extern BOOL IsCoveredByOtherWindow(HWND hWnd);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT const * pData;
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_KEYDOWN)
        {
            do
            {
                if (IsCoveredByOtherWindow(_hWnd))
                    break;

                pData = (KBDLLHOOKSTRUCT*)lParam;
                if (KS_KeyDownProc(_hWnd, WM_KEYDOWN, pData->vkCode, 0))
                    break;

#if 1
                if (VK_ESCAPE == pData->vkCode)
                {
                    if (_WndInfo.status == ds_fullscreen)
                    {
                        OnFullScreen(_hWnd, WM_KEYDOWN, pData->vkCode, lParam);
                    }
                }
                else if ('P' == pData->vkCode)
                {
                    if (GetAsyncKeyState(VK_CONTROL) & 0x8000
                        && GetAsyncKeyState(VK_SHIFT) & 0x8000
                        && GetAsyncKeyState(18) & 0x8000) // alt
                    {
                        if (!_hMouseHook)
                            _hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInst, NULL);
                        else
                        {
                            UnhookWindowsHookEx(_hMouseHook);
                            _hMouseHook = NULL;
                        }
                    }
                }
#endif
            } while (0);
        }
    }
    return CallNextHookEx(_hKeyboardHook, nCode, wParam, lParam);
}
#endif

BOOL IsVaildFile(HWND hWnd, TCHAR *filename, int *p_size)
{
    TCHAR *ext = NULL;
    FILE *fp = NULL;
    int size = 0;

    if (p_size) *p_size = 0;
    ext = PathFindExtension(filename);

    if (_tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")) && _tcscmp(ext, _T(".mobi")))
    {
        if (hWnd)
            MessageBox_(hWnd, IDS_UNKNOWN_FORMAT, IDS_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    fp = _tfopen(filename, _T("rb"));
    if (!fp)
    {
        if (hWnd)
            MessageBox_(hWnd, IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);

    if (size == 0)
    {
        if (hWnd)
            MessageBox_(hWnd, IDS_EMPTY_FILE, IDS_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (p_size) *p_size = size;
    return TRUE;
}

void OnOpenBook(HWND hWnd, TCHAR *filename, BOOL forced)
{
    item_t *item = NULL;
    TCHAR *ext = NULL;
    int size = 0;
    TCHAR szFileName[MAX_PATH] = {0};

    _tcscpy(szFileName, filename);
    ext = PathFindExtension(szFileName);

    if (!IsVaildFile(hWnd, szFileName, &size))
    {
        return;
    }

    // check if already exist in file list (by file name)
    item = _Cache.find_item(szFileName);
    if (item)
    {
        if (!forced)
        {
            if (_item && item->id == _item->id && _Book && !_Book->IsLoading()) // current is opened
            {
                OnUpdateMenu(hWnd);
                return;
            }
        }
    }
    _item = NULL;

    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (_tcscmp(ext, _T(".txt")) == 0)
    {
        _Book = new TextBook;
        _Book->SetFileName(szFileName);
        _Book->SetChapterRule(&(_header->chapter_rule));
        _Book->OpenBook(NULL, size, hWnd);
    }
    else if (_tcscmp(ext, _T(".epub")) == 0)
    {
        _Book = new EpubBook;
        _Book->SetFileName(szFileName);
        _Book->OpenBook(hWnd);
    }
    else if (_tcscmp(ext, _T(".mobi")) == 0)
    {
        _Book = new MobiBook;
        _Book->SetFileName(szFileName);
        _Book->OpenBook(hWnd);
    }

    //EnableWindow(hWnd, FALSE);
    PlayLoadingImage(hWnd);
}


VOID GetCacheVersion(TCHAR *ver)
{
    TCHAR version[256] = { 0 };
    GetApplicationVersion(version);
    wcscpy(ver, version);
}

BOOL Init(void)
{
    if (!_Cache.init())
    {
        MessageBox_(NULL, IDS_INIT_CACHE_FAIL, IDS_ERROR, MB_OK);
        return FALSE;
    }

    _header = _Cache.get_header();

    // delete not exist items
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        if (!PathFileExists(item->file_name))
        {
            _Cache.delete_item(i);
            i--;
        }
    }

    // just for debug
#if 0
    extern void TestXpathFromDump(void);
    TestXpathFromDump();
#endif

    return TRUE;
}

void Exit(void)
{
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (!_Cache.exit())
    {
        MessageBox_(NULL, IDS_SAVE_CACHE_FAIL, IDS_ERROR, MB_OK);
    }
#if TEST_MODEL
    logger_destroy();
#endif
}

static void _free_resource(HWND hWnd)
{
    // stop loading
    StopLoadingImage(hWnd);
    if (_loading)
    {
        if (_loading->hMemory)
            ::GlobalFree(_loading->hMemory);
        if (_loading->image)
            delete _loading->image;
        free(_loading->item);
        free(_loading);
        _loading = NULL;
    }
    // close book
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }
    // reset
    ShowSysTray(hWnd, FALSE);
    StopAutoPage(hWnd);
    KS_UnRegisterAllHotKey(hWnd);
    if (_hMouseHook)
    {
        UnhookWindowsHookEx(_hMouseHook);
        _hMouseHook = NULL;
    }
#if ENABLE_GLOBAL_KEY
    if (_hKeyboardHook)
    {
        UnhookWindowsHookEx(_hKeyboardHook);
        _hKeyboardHook = NULL;
    }
#endif
}

static void _update_data(HWND hWnd, BOOL keep_header, BOOL do_save)
{
    RECT rcbak1 = {0}, rcbak2 = {0};
    LOGFONT fontbak = {0};
#if ENABLE_TAG
    LOGFONT tagfontbak[MAX_TAG_COUNT] = { 0 };
#endif

    // backup
    GetWindowPlacement(hWnd, &_header->placement);
    if (keep_header)
    {
        rcbak1 = _header->placement.rcNormalPosition;
        rcbak2 = _header->fs_placement.rcNormalPosition;
        fontbak = _header->font;
    }

    RestoreRectForDpi(&_header->placement.rcNormalPosition);
    RestoreRectForDpi(&_header->fs_placement.rcNormalPosition);
    RestoreFontForDpi(&_header->font);
#if ENABLE_TAG
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        tagfontbak[i] = _header->tags[i].font;
        RestoreFontForDpi(&_header->tags[i].font);
    }
#endif
    _header->style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    _header->exstyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    // save
    if (do_save)
    {
        _Cache.save();
    }    

    // restore
    if (keep_header)
    {
        _header->placement.rcNormalPosition = rcbak1;
        _header->fs_placement.rcNormalPosition = rcbak2;
        _header->font = fontbak;
#if ENABLE_TAG
        for (int i = 0; i < MAX_TAG_COUNT; i++)
        {
            _header->tags[i].font = tagfontbak[i];
        }
#endif
    }
}

void Save(HWND hWnd)
{
#if ENABLE_REALTIME_SAVE
    PostMessage(hWnd, WM_SAVE_CACHE, 0, NULL);
#endif
}

LRESULT OnSave(HWND hWnd)
{
#if ENABLE_REALTIME_SAVE
    _update_data(hWnd, TRUE, TRUE);
#endif
    return 0;
}

void UpdateProgess(void)
{
    TCHAR progress[256] = {0};
    TCHAR str[256] = { 0 };
    double dprog = 0.0;
    int nprog = 0;

    if (EC_IsEditMode())
    {
        LoadString(hInst, IDS_EDIT_MODE_TIPS, progress, 256);
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
        return;
    }

    if (_item && _Book && !_Book->IsLoading())
    {
        dprog = (double)_Book->GetProgress();
        nprog = (int)(dprog * 100);
        dprog = (double)nprog / 100.0;
        if (!_IsAutoPage)
        {
            _stprintf(progress, _T("  %.2f%%  ( %d / %d )"), dprog, _item->index + _Book->GetPageLength(), _Book->GetTextLength());
        }
        else
        {
            LoadString(hInst, IDS_AUTOPAGING, str, 256);
            _stprintf(progress, _T("  %.2f%%  ( %d / %d )  [%s]"), dprog, _item->index + _Book->GetPageLength(), _Book->GetTextLength(), str);
        }
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
    }
    else
    {
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
    }
}

BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc)
{
    RECT rect;
    GetClientRect(hWnd, rc);
    if (IsWindowVisible(_WndInfo.hStatusBar))
    {
        GetClientRect(_WndInfo.hStatusBar, &rect);
        rc->bottom -= rect.bottom;
    }
    return TRUE;
}

BOOL FileExists(TCHAR *file)
{
    DWORD dwAttrib = GetFileAttributes(file);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

#ifdef ENABLE_NETWORK
void CheckUpgrade(HWND hWnd)
{
    // check upgrade
    _Upgrade.SetIngoreVersion(_header->ingore_version);
    _Upgrade.SetCheckVerTime(&_header->checkver_time);
    _Upgrade.Check(UpgradeCallback, hWnd);
    SetTimer(hWnd, IDT_TIMER_UPGRADE, 24 * 60 * 60 * 1000 /*one day*/, NULL);
}

BOOL UpgradeCallback(void *param, json_item_data_t *item)
{
    HWND hWnd = (HWND)param;

    PostMessage(hWnd, WM_NEW_VERSION, 0, NULL);
    return TRUE;
}
#endif

void SetGlobalKey(HWND hWnd)
{
#if ENABLE_GLOBAL_KEY
    if (_header->global_key)
    {
        if (!_hKeyboardHook)
        {
            _hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInst, NULL);
        }
    }
    else
    {
        if (_hKeyboardHook)
        {
            UnhookWindowsHookEx(_hKeyboardHook);
            _hKeyboardHook = NULL;
        }
    }
#endif
}

HHOOK hMsgBoxHook = NULL;
LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    RECT rc;
    int w, h, x, y;
    LPCBT_CREATEWND cbt;
    if (nCode == HCBT_CREATEWND)
    {
        cbt = ((LPCBT_CREATEWND)lParam);

        if (cbt->lpcs->lpszClass == (LPWSTR)(ATOM)32770)  // #32770 = dialog box class
        {
            hParent = cbt->lpcs->hwndParent;
            if (hParent)
            {
                GetWindowRect(hParent, &rc);
                w = rc.right - rc.left;
                h = rc.bottom - rc.top;
                x = rc.left + (w - cbt->lpcs->cx) / 2;
                y = rc.top + (h - cbt->lpcs->cy) / 2;
                cbt->lpcs->x = x;
                cbt->lpcs->y = y;
            }
        }
    }

    return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
}

int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType)
{
    TCHAR szText[MAX_LOADSTRING] = { 0 };
    TCHAR szCaption[MAX_LOADSTRING] = { 0 };
    int res;

    hMsgBoxHook = SetWindowsHookEx(WH_CBT, MsgBoxHookProc, hInst, GetCurrentThreadId());
    LoadString(hInst, textId, szText, MAX_LOADSTRING);
    LoadString(hInst, captionId, szCaption, MAX_LOADSTRING);
    res = MessageBoxEx(hWnd, szText, szCaption, uType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if (hMsgBoxHook)
        UnhookWindowsHookEx(hMsgBoxHook);
    return res;
}

int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...)
{
    TCHAR szText[MAX_LOADSTRING] = { 0 };
    TCHAR szCaption[MAX_LOADSTRING] = { 0 };
    TCHAR szFormat[MAX_LOADSTRING] = { 0 };
    int res;
    va_list args;

    hMsgBoxHook = SetWindowsHookEx(WH_CBT, MsgBoxHookProc, hInst, GetCurrentThreadId());
    LoadString(hInst, formatId, szFormat, MAX_LOADSTRING);
    LoadString(hInst, captionId, szCaption, MAX_LOADSTRING);

    va_start(args, formatId);
    _vsntprintf(szText, MAX_LOADSTRING, szFormat, args);
    va_end(args);
    
    res = MessageBoxEx(hWnd, szText, szCaption, uType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if (hMsgBoxHook)
        UnhookWindowsHookEx(hMsgBoxHook);
    return res;
}
