// Globals.cpp : single definition of the globals declared in Reader.h
#include "Reader.h"

Cache               _Cache(CACHE_FILE_NAME);
header_t*           _header                 = NULL;
item_t*             _item                   = NULL;
HWND                _hFindDlg               = NULL;
HWND                _hTreeView              = NULL;
HWND                _hTreeMark              = NULL;
UINT                _uFindReplaceMsg        = 0;
window_info_t       _WndInfo                = { 0 };
BOOL                _IsAutoPage             = FALSE;
#ifdef ENABLE_NETWORK
Upgrade             _Upgrade;
#endif
Book *              _Book                   = NULL;
loading_data_t *    _loading                = NULL;
HHOOK               _hMouseHook             = NULL;
#if ENABLE_GLOBAL_KEY
HHOOK               _hKeyboardHook          = NULL;
#endif
HWND                _hWnd                   = NULL;
NOTIFYICONDATA      _nid                    = { 0 };
BYTE                _textAlpha              = 0xFF;
BOOL                _menuInvalid            = FALSE;

// app instance / strings (previously at the top of Reader.cpp)
HINSTANCE           hInst                   = NULL;
TCHAR               szTitle[256]            = { 0 };
TCHAR               szWindowClass[256]      = { 0 };
TCHAR               szStatusClass[256]      = { 0 };
