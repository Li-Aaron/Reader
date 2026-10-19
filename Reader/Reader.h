#ifndef __READER_H__
#define __READER_H__


#include "resource.h"
#include "Cache.h"
#include "Utils.h"
#include "Book.h"
#ifdef ENABLE_NETWORK
#include "Upgrade.h"
#endif
#include <map>
#include <shellapi.h>

#define MAX_LOADSTRING              256

typedef struct loading_data_t
{
    BOOL enable;
    Gdiplus::Bitmap*image;
    Gdiplus::PropertyItem *item;
    UINT frameCount;
    UINT frameIndex;
    HGLOBAL hMemory;
} loading_data_t;

// Globals: defined once in Globals.cpp, shared across Reader/Dialogs/Gfx/
// WindowUtil/AutoPage translation units
extern Cache             _Cache;
extern header_t*         _header;
extern item_t*           _item;
extern HWND              _hFindDlg;
extern HWND              _hTreeView;
extern HWND              _hTreeMark;
extern UINT              _uFindReplaceMsg;
extern window_info_t     _WndInfo;
extern BOOL              _IsAutoPage;
#ifdef ENABLE_NETWORK
extern Upgrade           _Upgrade;
#endif
extern Book *            _Book;
extern loading_data_t *  _loading;
extern HHOOK             _hMouseHook;
#if ENABLE_GLOBAL_KEY
extern HHOOK             _hKeyboardHook;
#endif
extern HWND              _hWnd;
extern NOTIFYICONDATA    _nid;
extern BYTE              _textAlpha;
extern BOOL              _menuInvalid;
// app strings / instance (defined in Globals.cpp as well)
extern HINSTANCE         hInst;
extern TCHAR             szTitle[];
extern TCHAR             szWindowClass[];
extern TCHAR             szStatusClass[];


LRESULT             OnCreate(HWND);
LRESULT             OnUpdateMenu(HWND);
LRESULT             OnOpenItem(HWND, int, BOOL);
LRESULT             OnClearFileList(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnRestoreDefault(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPaint(HWND, HDC);
VOID                OnDraw(HWND);
BOOL                ResetLayerd(HWND);
VOID                Invalidate(HWND, BOOL, BOOL);
LRESULT             OnSize(HWND, UINT, WPARAM, LPARAM);
// WM_KEYWORD begin
LRESULT             OnHideWin(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnHideBorder(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFullScreen(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnTopmost(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnOpenFile(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnAddMark(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnAutoPage(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSearch(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnJump(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnEditMode(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPageUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPageDown(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnLineUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnLineDown(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnChapterUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnChapterDown(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFontZoomIn(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFontZoomOut(HWND, UINT, WPARAM, LPARAM);
// WM_KEYWORD end
LRESULT             OnDropFiles(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFindText(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnUpdateChapters(HWND);
LRESULT             OnUpdateBookMark(HWND);
LRESULT             OnOpenBookResult(HWND, BOOL);
LRESULT             OnCopyData(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    MouseProc(int, WPARAM, LPARAM);
LRESULT CALLBACK    KeyboardProc(int, WPARAM, LPARAM);
void                ShowHideWindow(HWND);
BOOL                IsVaildFile(HWND, TCHAR *, int *);
void                OnOpenBook(HWND, TCHAR *, BOOL);
VOID                GetCacheVersion(TCHAR *);
BOOL                Init(void);
void                Exit(void);
void                Save(HWND);
LRESULT             OnSave(HWND);
void                UpdateProgess(void);
void                UpdateTitle(HWND);
void                RemoveMenus(HWND, BOOL);
BOOL                GetClientRectExceptStatusBar(HWND, RECT*);
void                WheelSpeedInit(HWND);
Gdiplus::Bitmap*    LoadBGImage(int, int, BYTE alpha=0xFF);
BOOL                FileExists(TCHAR *);
void                StartAutoPage(HWND);
void                StopAutoPage(HWND);
void                PauseAutoPage(HWND);
void                ResumeAutoPage(HWND);
void                ResetAutoPage(HWND hWnd);
#ifdef ENABLE_NETWORK
void                CheckUpgrade(HWND);
BOOL                UpgradeCallback(void *, json_item_data_t *);
#endif
BOOL                PlayLoadingImage(HWND);
BOOL                StopLoadingImage(HWND);
ULONGLONG           GetDllVersion(LPCTSTR);
BOOL CALLBACK       EnumWindowsProc(HWND, LPARAM);
void                ShowInTaskbar(HWND, BOOL);
void                ShowSysTray(HWND, BOOL);
HBITMAP             CreateAlphaBGColorBitmap(HWND hWnd, COLORREF inColour, int width, int height, BYTE alpha);
void                SetTreeviewFont();
BOOL                LoadResourceImage(LPCWSTR, LPCWSTR, Gdiplus::Bitmap**, HGLOBAL*);
void                SetGlobalKey(HWND);
int                 MessageBox_(HWND, UINT, UINT, UINT);
int                 MessageBoxFmt_(HWND, UINT, UINT, UINT, ...);


#endif
