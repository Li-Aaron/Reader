// Dialogs.cpp : modal dialog procedures (about / settings / jump progress)
#include "Reader.h"
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HWND hWnd;
    RECT rc;
    TCHAR version[256];
    TCHAR info[256];

    switch (message)
    {
    case WM_INITDIALOG:
        GetApplicationVersion(version);
        _sntprintf(info, 256, _T("Reader (aaron-mod), Version %s"), version);
        SetDlgItemText(hDlg, IDC_STATIC_VERSION, info);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hWnd = GetDlgItem(hDlg, IDC_STATIC_LINK);
        ClientToScreen(hDlg, &pt);
        GetWindowRect(hWnd, &rc);
        if (PtInRect(&rc, pt))
        {
#ifdef ENABLE_NETWORK
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader"), NULL, NULL, SW_SHOWNORMAL);
#endif
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Setting(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;
    int value = 0;
    int cid;
    LRESULT res;
    TCHAR buf[256] = { 0 };
    RECT rc = { 0 };
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_EDIT_LEFTLINE, _header->left_line_count, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, _header->uElapse, TRUE);
        if (_header->page_mode == 0)
            cid = IDC_RADIO_MODE1;
        else if (_header->page_mode == 1)
            cid = IDC_RADIO_MODE2;
        else
            cid = IDC_RADIO_MODE3;
        SendMessage(GetDlgItem(hDlg, cid), BM_SETCHECK, BST_CHECKED, NULL);
        WheelSpeedInit(hDlg);
        // init window style
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, _header->show_systray ? BST_CHECKED : BST_UNCHECKED, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_SETCHECK, _header->hide_taskbar ? BST_CHECKED : BST_UNCHECKED, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_SETCHECK, _header->disable_lrhide ? BST_UNCHECKED : BST_CHECKED, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_ESCHIDE), BM_SETCHECK, _header->disable_eschide ? BST_UNCHECKED : BST_CHECKED, NULL);
        if ((_header->autopage_mode & 0x0f) == apm_page)
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATWHEEL), BM_SETCHECK, BST_CHECKED, NULL);
        LoadString(hInst, IDS_FIXED_TIME, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(hInst, IDS_DYNAMIC_CALC, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_SETCURSEL, _header->autopage_mode >> 8, NULL);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (!bResult || value == 0)
            {
                MessageBox_(hDlg, IDS_INVALID_AUTOPAGE, IDS_ERROR, MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) != -1)
            {
                _header->wheel_speed = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE1), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                _header->page_mode = 0;
            }
            else
            {
                res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE2), BM_GETCHECK, 0, NULL);
                if (res == BST_CHECKED)
                {
                    _header->page_mode = 1;
                }
                else
                {
                    _header->page_mode = 2;
                }
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_LEFTLINE, &bResult, FALSE);
            if (value != _header->left_line_count)
            {
                _header->left_line_count = value;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (value != (int)_header->uElapse)
            {
                _header->uElapse = value;
            }
            // show taskbar/tray or not
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->hide_taskbar == 0)
                || (res == BST_UNCHECKED && _header->hide_taskbar == 1))
            {
                _header->hide_taskbar = !_header->hide_taskbar;
                ShowInTaskbar(GetParent(hDlg), !_header->hide_taskbar);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->show_systray == 0)
                || (res == BST_UNCHECKED && _header->show_systray == 1))
            {
                _header->show_systray = !_header->show_systray;
                ShowSysTray(GetParent(hDlg), _header->show_systray);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_GETCHECK, 0, NULL);
            _header->disable_lrhide = res == BST_CHECKED ? 0 : 1;
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_ESCHIDE), BM_GETCHECK, 0, NULL);
            _header->disable_eschide = res == BST_CHECKED ? 0 : 1;
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_GETCHECK, 0, NULL);
            _header->autopage_mode = 0;
            _header->autopage_mode |= res == BST_CHECKED ? apm_page : apm_line;
            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_GETCURSEL, 0, NULL);
            _header->autopage_mode |= res == 1 ? apm_count : apm_fixed;
            Save(GetParent(hDlg));
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_TASKBAR:
        case IDC_CHECK_TRAY:
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK JumpProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd;
    TCHAR buf[16] = {0};
    double progress;

    switch (message)
    {
    case WM_INITDIALOG:
        hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
        if (_Book)
        {
            progress = _Book->GetProgress();
            _stprintf(buf, _T("%0.2f"), progress);
            SetWindowText(hWnd, buf);
        }
        SetFocus(hWnd);
        SendMessage(hWnd, EM_SETSEL, 0, -1);
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                if (_item)
                {
                    hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
                    GetWindowText(hWnd, buf, 15);
                    progress = _tstof(buf);
                    if (progress < 0.0 || progress > 100.0)
                    {
                        MessageBox_(hDlg, IDS_INVALID_VALUE, IDS_ERROR, MB_OK|MB_ICONERROR);
                        return (INT_PTR)TRUE;
                    }
                    if (_Book)
                    {
                        _item->index = (int)(progress * _Book->GetTextLength() / 100);
                        if (_item->index == _Book->GetTextLength())
                            _item->index--;
                        _Book->ReDraw(GetParent(hDlg));
                        Save(GetParent(hDlg));
                    }
                }
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

void WheelSpeedInit(HWND hDlg)
{
    HWND hWnd = NULL;
    TCHAR szText[MAX_LOADSTRING] = {0};

    hWnd = GetDlgItem(hDlg, IDC_COMBO_SPEED);
    LoadString(hInst, IDS_SINGLE_LINE, szText, MAX_LOADSTRING);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)szText);
    LoadString(hInst, IDS_DOUBLE_LINE, szText, MAX_LOADSTRING);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)szText);
    LoadString(hInst, IDS_THREE_LINE, szText, MAX_LOADSTRING);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)szText);
    LoadString(hInst, IDS_FULL_PAGE, szText, MAX_LOADSTRING);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)szText);

    if (_header->wheel_speed > ws_fullpage)
        _header->wheel_speed = ws_fullpage;

    SendMessage(hWnd, CB_SETCURSEL, _header->wheel_speed, NULL);
}
