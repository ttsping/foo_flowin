#pragma once
#include <cmath>

#ifndef __ATLBASE_H__
#error snap_window.h requires atlbase.h to be included first
#endif

#ifndef __ATLWIN_H__
#error snap_window.h requires atlwin.h to be included first
#endif

#define UWM_MOUSEENTER (WM_USER + 998)
#define UWM_MOUSELEAVE (WM_USER + 999)

template <typename T> class CSnapWindow
{
private:
    enum
    {
        SNAP_TIMER_ID = 0x1102,
        MOUSE_CHECK_TIMER_ID,
    };

    enum class SnapSimulateState
    {
        SNAP_SIM_NONE = -1,
        SNAP_SIM_HIDE = 0,
        SNAP_SIM_SHOW = 1,
    };

protected:
    enum SNAP_STATE
    {
        SNAP_INVALID = -1,
        SNAP_NONE = 0,
        SNAP_LEFT,
        SNAP_TOP,
        SNAP_RIGHT,
        SNAP_BOTTOM,
    };

public:
    CSnapWindow()
    {
        UpdateDPI();
    }
    BEGIN_MSG_MAP(CSnapWindow)
        MESSAGE_HANDLER(WM_MOVING, OnMoving)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDPIChanged)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(UWM_MOUSEENTER, OnMouseEnter)
        MESSAGE_HANDLER(UWM_MOUSELEAVE, OnMouseLeave)
    END_MSG_MAP()

public:
    inline HWND GetHWnd()
    {
        return static_cast<T*>(this)->m_hWnd;
    }

    BOOL IsMouseInWindow()
    {
        POINT pt;
        GetCursorPos(&pt);
        HWND hMyWnd = GetHWnd();
        HWND hWnd = WindowFromPoint(pt);
        while (hWnd && hWnd != hMyWnd)
        {
            hWnd = GetParent(hWnd);
        }

        if (hWnd == NULL)
        {
            GUITHREADINFO gui_info = {0};
            gui_info.cbSize = sizeof(gui_info);
            if (GetGUIThreadInfo(GetWindowThreadProcessId(hMyWnd, nullptr), &gui_info))
            {
                if (gui_info.hwndMenuOwner &&
                    (gui_info.hwndMenuOwner == hMyWnd || IsChild(hMyWnd, gui_info.hwndMenuOwner)))
                {
                    hWnd = gui_info.hwndMenuOwner;
                }
            }
        }
        return hWnd != NULL;
    }

protected:
    LRESULT OnMoving(UINT /*msg*/, WPARAM /*wp*/, LPARAM lp, BOOL& /*handled*/)
    {
        if (!enable_snap_)
            return FALSE;
        LPRECT prc = (LPRECT)lp;
        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return FALSE;
        RECT rect = *prc;
        POINT pt;
        if (GetCursorPos(&pt))
        {
            OffsetRect(&rect, pt.x - (rect.left + snap_dx_), pt.y - (rect.top + snap_dy_));
        }

        // left && right
        if (DetectSnap(rect.left, rect_work.left))
        {
            OffsetRect(&rect, rect_work.left - rect.left, 0);
        }
        else if (DetectSnap(rect.right, rect_work.right))
        {
            OffsetRect(&rect, rect_work.right - rect.right, 0);
        }

        // top && bottom
        if (DetectSnap(rect.top, rect_work.top))
        {
            OffsetRect(&rect, 0, rect_work.top - rect.top);
        }
        else if (DetectSnap(rect.bottom, rect_work.bottom))
        {
            OffsetRect(&rect, 0, rect_work.bottom - rect.bottom);
        }

        *prc = rect;
        return TRUE;
    }

    LRESULT OnEnterSizeMove(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        if (!enable_snap_)
            return 1;
        snap_state_ = SNAP_NONE;
        RECT rect;
        POINT pt;
        if (::GetWindowRect(GetHWnd(), &rect) && GetCursorPos(&pt))
        {
            snap_dx_ = pt.x - rect.left;
            snap_dy_ = pt.y - rect.top;
        }
        return 0;
    }

    LRESULT OnExitSizeMove(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        snap_state_ = CheckSnapState();
        return 0;
    }

    LRESULT OnMouseMove(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        const bool is_snap = (snap_timer_ != NULL) || (snap_state_ != SNAP_NONE);
        if ((snap_auto_hide_ || is_snap) && !mouse_check_timer_)
        {
            ::PostMessage(GetHWnd(), UWM_MOUSEENTER, 0, 0);
            StartMouseCheckTimer();
        }
        return 0;
    }

    LRESULT OnMouseEnter(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        mouse_in_window_ = TRUE;
        const auto state = CheckSnapState();
        if (snap_auto_hide_ || (state == SNAP_NONE && state != snap_state_))
        {
            if (snap_state_ == SNAP_INVALID)
            {
                snap_state_ = state;
            }
            else if (snap_state_ != SNAP_NONE)
            {
                if (state != snap_state_)
                    StartSnapAnimateTimer();
            }
        }
        return 0;
    }

    LRESULT OnMouseLeave(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        mouse_in_window_ = FALSE;
        if ((snap_state_ != SNAP_NONE) && snap_auto_hide_)
        {
            StartSnapAnimateTimer();
        }
        return 0;
    }

    LRESULT OnTimer(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        UINT_PTR id = (UINT_PTR)wp;
        switch (id)
        {
        case SNAP_TIMER_ID:
            if (!AnimateSnappedWindow(mouse_in_window_ || (snap_sim_state_ == SnapSimulateState::SNAP_SIM_SHOW)))
            {
                KillSnapAnimateTimer();
            }
            return 0;

        case MOUSE_CHECK_TIMER_ID:
            if (!IsMouseInWindow())
            {
                ::PostMessage(GetHWnd(), UWM_MOUSELEAVE, 0, 0);
                KillMouseCheckTimer();
            }
            return 0;
        default:
            break;
        }
        return 1;
    }

    LRESULT OnDPIChanged(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        UpdateDPI();
        return TRUE;
    }

protected:
    SNAP_STATE CheckSnapState()
    {
        RECT rect;
        if (!::GetWindowRect(GetHWnd(), &rect))
            return SNAP_NONE;

        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return SNAP_NONE;

        if (rect.left == rect_work.left)
            return SNAP_LEFT;
        if (rect.top == rect_work.top)
            return SNAP_TOP;
        if (rect.right == rect_work.right)
            return SNAP_RIGHT;
        if (rect.bottom == rect_work.bottom)
            return SNAP_BOTTOM;

        return SNAP_NONE;
    }

    BOOL GetSnapWindowRect(LPRECT prc)
    {
        if (!::GetWindowRect(GetHWnd(), prc))
            return FALSE;
        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return TRUE; // use current rect
        int ww = prc->right - prc->left;
        int wh = prc->bottom - prc->top;
        switch (snap_state_)
        {
        case SNAP_LEFT:
            prc->left = rect_work.left;
            prc->right = prc->left + ww;
            break;

        case SNAP_TOP:
            prc->top = rect_work.top;
            prc->bottom = prc->top + wh;
            break;

        case SNAP_RIGHT:
            prc->right = rect_work.right;
            prc->left = prc->right - ww;
            break;

        case SNAP_BOTTOM:
            prc->bottom = rect_work.bottom;
            prc->top = prc->bottom - wh;
            break;

        default:
            break;
        }
        return TRUE;
    }

    VOID RestoreFromSnapHidden()
    {
        if (snap_state_ != SNAP_NONE)
        {
            RECT rect = {0};
            if (GetSnapWindowRect(&rect))
            {
                ::SetWindowPos(GetHWnd(), HWND_TOP, rect.left, rect.top, 0, 0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }

    VOID SimulateSnapToHide()
    {
        if (auto state = CheckSnapState(); state != SNAP_NONE)
        {
            snap_state_ = state;
            snap_sim_state_ = SnapSimulateState::SNAP_SIM_HIDE;
            StartSnapAnimateTimer();
        }
    }

    VOID SimulateSnapToShow()
    {
        if (auto state = CheckSnapState(); (state == SNAP_NONE) && (state != snap_state_))
        {
            snap_sim_state_ = SnapSimulateState::SNAP_SIM_SHOW;
            StartSnapAnimateTimer();
        }
    }

private:
    inline VOID StartSnapAnimateTimer()
    {
        if (snap_timer_ == NULL)
        {
            snap_timer_ = ::SetTimer(GetHWnd(), SNAP_TIMER_ID, USER_TIMER_MINIMUM, NULL);
        }
    }

    inline VOID KillSnapAnimateTimer()
    {
        if (snap_timer_)
        {
            ::KillTimer(GetHWnd(), snap_timer_);
            snap_timer_ = NULL;
        }

        snap_sim_state_ = SnapSimulateState::SNAP_SIM_NONE;
    }

    inline VOID StartMouseCheckTimer()
    {
        if (mouse_check_timer_ == NULL)
        {
            mouse_check_timer_ = ::SetTimer(GetHWnd(), MOUSE_CHECK_TIMER_ID, 100, NULL);
        }
    }

    inline VOID KillMouseCheckTimer()
    {
        if (mouse_check_timer_)
        {
            ::KillTimer(GetHWnd(), mouse_check_timer_);
            mouse_check_timer_ = NULL;
        }
    }

    VOID UpdateDPI()
    {
        if (HDC dc = ::GetDC(NULL))
        {
            dpi_ = GetDeviceCaps(dc, LOGPIXELSX);
            snap_detect_val_ = MulDiv(16, dpi_, 96);
            snap_move_delta_ = MulDiv(kSnapMoveDelta, dpi_, 96);
            ::ReleaseDC(NULL, dc);
        }
    }

    inline BOOL DetectSnap(int x1, int x2)
    {
        return std::abs(x1 - x2) < snap_detect_val_;
    }

    BOOL GetWorkAreaRect(LPRECT prc)
    {
        ZeroMemory(prc, sizeof(prc));
        if (HMONITOR h = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi = {0};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(h, &mi))
            {
                ::CopyRect(prc, &mi.rcWork);
                return TRUE;
            }
        }
        // fallback. primary monitor's workarea
        return SystemParametersInfo(SPI_GETWORKAREA, 0, prc, 0);
    }

    BOOL AnimateSnappedWindow(BOOL revert = FALSE)
    {
        RECT rect;
        HWND window = GetHWnd();
        if (!::GetWindowRect(window, &rect))
            return FALSE;
        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return FALSE;

        const int move_delta = snap_move_delta_;
        int dx = 0, dy = 0;
        int& val = (snap_state_ == SNAP_LEFT || snap_state_ == SNAP_RIGHT) ? dx : dy;
        val = (snap_state_ == SNAP_LEFT || snap_state_ == SNAP_TOP) ? -1 * move_delta : move_delta;
        OffsetRect(&rect, revert ? -1 * dx : dx, revert ? -1 * dy : dy);

        BOOL animate_continue = TRUE;
        // ajust offset
        int ww = rect.right - rect.left;
        int wh = rect.bottom - rect.top;

        switch (snap_state_)
        {
        case SNAP_LEFT:
            if (revert)
            {
                if (rect.left > rect_work.left)
                {
                    rect.left = rect_work.left;
                    rect.right = rect.left + ww;
                    animate_continue = FALSE;
                }
            }
            else
            {
                if (rect.right - rect_work.left < kSnapHideEdgeWidth)
                {
                    rect.right = rect_work.left + kSnapHideEdgeWidth;
                    rect.left = rect.right - ww;
                    animate_continue = FALSE;
                }
            }
            break;

        case SNAP_TOP:
            if (revert)
            {
                if (rect.top > rect_work.top)
                {
                    rect.top = rect_work.top;
                    rect.bottom = rect.top + wh;
                    animate_continue = FALSE;
                }
            }
            else
            {
                if (rect.bottom - rect_work.top < kSnapHideEdgeWidth)
                {
                    rect.bottom = rect_work.top + kSnapHideEdgeWidth;
                    rect.top = rect.bottom - wh;
                    animate_continue = FALSE;
                }
            }
            break;

        case SNAP_RIGHT:
            if (revert)
            {
                if (rect.right < rect_work.right)
                {
                    rect.right = rect_work.right;
                    rect.left = rect.right - ww;
                    animate_continue = FALSE;
                }
            }
            else
            {
                if (rect_work.right - rect.left < kSnapHideEdgeWidth)
                {
                    rect.left = rect_work.right - kSnapHideEdgeWidth;
                    rect.right = rect.left + ww;
                    animate_continue = FALSE;
                }
            }
            break;

        case SNAP_BOTTOM:
            if (revert)
            {
                if (rect.bottom < rect_work.bottom)
                {
                    rect.bottom = rect_work.bottom;
                    rect.top = rect.bottom - wh;
                    animate_continue = FALSE;
                }
            }
            else
            {
                if (rect_work.bottom - rect.top < kSnapHideEdgeWidth)
                {
                    rect.top = rect_work.bottom - kSnapHideEdgeWidth;
                    rect.bottom = rect.top + wh;
                    animate_continue = FALSE;
                }
            }
            break;

        default:
            break;
        }

        ::SetWindowPos(window, HWND_TOP, rect.left, rect.top, 0, 0,
                       SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
        return animate_continue;
    }

protected:
    bool enable_snap_ = false;
    bool snap_auto_hide_ = true;
    int dpi_ = 96;
    int snap_dx_ = 0, snap_dy_ = 0;
    int snap_detect_val_ = 0;
    SNAP_STATE snap_state_ = SNAP_INVALID;
    UINT_PTR snap_timer_ = 0;
    UINT_PTR mouse_check_timer_ = 0;
    int kSnapHideEdgeWidth = 8;
    BOOL mouse_in_window_ = FALSE;
    const int kSnapMoveDelta = 45;
    int snap_move_delta_ = kSnapMoveDelta;
    SnapSimulateState snap_sim_state_ = SnapSimulateState::SNAP_SIM_HIDE;
};