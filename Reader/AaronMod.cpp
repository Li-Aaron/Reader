// AaronMod.cpp : aaron-branch customizations (see AaronMod.h)
#include "AaronMod.h"
#include "Reader.h"

// Ctrl+H toggles the border, alongside the configurable keymap (F12 default)
BOOL AM_KeyDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if ('H' == wParam)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            OnHideBorder(hWnd, message, wParam, lParam);
            return TRUE;
        }
    }
    return FALSE;
}

// mouse XBUTTON1 (side key 4) / XBUTTON2 (side key 5) page turning
BOOL AM_XButtonDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (HIWORD(wParam) == XBUTTON1)
    {
        OnPageUp(hWnd, message, wParam, lParam);
        return TRUE;
    }
    if (HIWORD(wParam) == XBUTTON2)
    {
        OnPageDown(hWnd, message, wParam, lParam);
        return TRUE;
    }
    return FALSE;
}

// delete the selected bookmark of the mark treeview (right-click menu / Del)
BOOL AM_DeleteBookMark(HWND hWnd)
{
    HTREEITEM Selected;
    TVITEM item;

    if (!IsWindowVisible(_hTreeMark))
        return FALSE;

    Selected = TreeView_GetSelection(_hTreeMark);
    if (!Selected)
        return FALSE;

    item.mask       = TVIF_PARAM;
    item.hItem      = Selected;
    TreeView_GetItem(_hTreeMark, &item);

    if (_Cache.del_mark(_item, (int)item.lParam))
    {
        Save(hWnd);
        OnUpdateBookMark(hWnd);
        return TRUE;
    }
    return FALSE;
}

// bookmark list text: "chapter | first line at mark (xx%)"
static void _mark_display_text(TCHAR *buf, int bufsize, int pos)
{
    const wchar_t *text = _Book->GetText();
    int textlen = _Book->GetTextLength();
    chapters_t *chs = _Book->GetChapters();
    const wchar_t *title = NULL;
    TCHAR tmp[600];
    int p, start, linelen, i;
    size_t n;

    buf[0] = 0;
    if (!text || pos < 0 || pos >= textlen)
        return;

    // find the chapter containing pos (last chapter with index <= pos)
    if (chs)
    {
        for (i = 0; i < (int)chs->size(); i++)
        {
            if ((*chs)[i].index <= pos)
                title = (*chs)[i].title.c_str();
            else
                break;
        }
    }

    // excerpt: first non-empty line at/after pos, capped
    p = pos;
    while (p < textlen && (text[p] == L'\r' || text[p] == L'\n'))
        p++;
    start = p;
    while (p < textlen && text[p] != L'\r' && text[p] != L'\n' && (p - start) < 120)
        p++;
    linelen = p - start;

    if (title && *title
        && linelen > 0 && (int)wcslen(title) == linelen
        && 0 == wcsncmp(title, text + start, linelen))
    {
        // mark sits at the chapter head, excerpt is the title itself
        title = NULL;
    }

    n = 0;
    if (title && *title)
        n += _sntprintf(tmp + n, 599 - n, _T("%s | "), title);
    if (linelen > 0)
        n += _sntprintf(tmp + n, 599 - n, _T("%.*s"), linelen, text + start);
    if (textlen > 0)
        _sntprintf(tmp + n, 600 - n, _T(" (%d%%)"), pos * 100 / textlen);

    _tcsncpy(buf, tmp, bufsize - 1);
    buf[bufsize - 1] = 0;
}

LRESULT OnUpdateBookMark(HWND hWnd)
{
    const int MAX_MARK_TEXT = 256;
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    int i;
    TCHAR szText[MAX_MARK_TEXT] = {0};

    TreeView_DeleteAllItems(_hTreeMark);
    if (_Book && !_Book->IsLoading() && _item)
    {
        tvi.mask = TVIF_TEXT | TVIF_PARAM;

        for (i=0; i<_item->mark_size; i++)
        {
            _mark_display_text(szText, MAX_MARK_TEXT, _item->mark[i]);

            tvi.pszText = szText;
            tvi.cchTextMax = MAX_MARK_TEXT;
            tvi.lParam = (LPARAM)i;
            tvins.item = tvi;
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            hPrev = (HTREEITEM)SendMessage(_hTreeMark, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }

    return 0;
}
