// AutoPage.cpp : automatic page turning timer control
#include "Reader.h"
void StartAutoPage(HWND hWnd)
{
    if (_item && !_IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetPageLength(), NULL);
        }
        else
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
        }
        _IsAutoPage = TRUE;
        UpdateProgess();
    }
}

void StopAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
        _IsAutoPage = FALSE;
        UpdateProgess();
    }
}

void PauseAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
    }
}

void ResumeAutoPage(HWND hWnd)
{
    if (_item && _IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetPageLength(), NULL);
        }
        else
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
        }
    }
    else
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
        _IsAutoPage = FALSE;
    }
}

void ResetAutoPage(HWND hWnd)
{
    if (_item && _IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            KillTimer(hWnd, IDT_TIMER_PAGE);
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetPageLength(), NULL);
        }
    }
}
