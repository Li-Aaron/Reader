#ifndef __AARONMOD_H__
#define __AARONMOD_H__

// AaronMod : centralized aaron-branch customizations, kept in one place so
// future merges from upstream binbyu/Reader only need to touch the small
// hook points in Reader.cpp (WndProc):
//   - WM_KEYDOWN  'H'  -> AM_KeyDown()     (Ctrl+H border toggle)
//   - WM_XBUTTONDOWN  -> AM_XButtonDown()  (mouse side keys 4/5 page turn)
//   - bookmark delete -> AM_DeleteBookMark(), called from WM_CONTEXTMENU
//     and TVN_KEYDOWN(Del) in the mark treeview
//   - bookmark list rendering -> OnUpdateBookMark() lives here too

#include "framework.h"
#include <commctrl.h>

BOOL AM_KeyDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL AM_XButtonDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL AM_DeleteBookMark(HWND hWnd);

#endif
