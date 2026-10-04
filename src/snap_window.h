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
        SnapSimNone = -1,
        SnapSimHide = 0,
        SnapSimShow = 1,
    };

protected:
    enum SnapState
    {
        SnapInvalid = -1,
        SnapNone = 0,
        SnapLeft,
        SnapTop,
        SnapRight,
        SnapBottom,
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
    bool AutoHideEnabled()
    {
        return static_cast<T*>(this)->SnapWindowAutoHideEnabled();
    }

    bool HostNeedsMouseTracking()
    {
        return static_cast<T*>(this)->SnapWindowNeedMouseTracking();
    }

    LRESULT OnMoving(UINT /*msg*/, WPARAM /*wp*/, LPARAM lp, BOOL& /*handled*/)
    {
        if (!enable_snap)
            return FALSE;
        LPRECT prc = (LPRECT)lp;
        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return FALSE;
        RECT rect = *prc;
        POINT pt;
        if (GetCursorPos(&pt))
        {
            OffsetRect(&rect, pt.x - (rect.left + snap_dx), pt.y - (rect.top + snap_dy));
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
        if (!enable_snap)
            return 1;
        snap_state = SnapNone;
        RECT rect;
        POINT pt;
        if (::GetWindowRect(GetHWnd(), &rect) && GetCursorPos(&pt))
        {
            snap_dx = pt.x - rect.left;
            snap_dy = pt.y - rect.top;
        }
        return 0;
    }

    LRESULT OnExitSizeMove(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        snap_state = CheckSnapState();
        return 0;
    }

    LRESULT OnMouseMove(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        const bool is_snap = (snap_timer != NULL) || (snap_state != SnapNone);
        if ((AutoHideEnabled() || is_snap || HostNeedsMouseTracking()) && !mouse_check_timer)
        {
            ::PostMessage(GetHWnd(), UWM_MOUSEENTER, 0, 0);
            StartMouseCheckTimer();
        }
        return 0;
    }

    LRESULT OnMouseEnter(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        mouse_in_window = TRUE;
        const auto state = CheckSnapState();
        if (AutoHideEnabled() || (state == SnapNone && state != snap_state))
        {
            if (snap_state == SnapInvalid)
            {
                snap_state = state;
            }
            else if (snap_state != SnapNone)
            {
                if (state != snap_state)
                    StartSnapAnimateTimer();
            }
        }
        static_cast<T*>(this)->SnapWindowOnHoverMouseEnter();
        return 0;
    }

    LRESULT OnMouseLeave(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        mouse_in_window = FALSE;
        if ((snap_state != SnapNone) && AutoHideEnabled())
        {
            StartSnapAnimateTimer();
        }
        static_cast<T*>(this)->SnapWindowOnHoverMouseLeave();
        return 0;
    }

    LRESULT OnTimer(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/, BOOL& /*handled*/)
    {
        UINT_PTR id = (UINT_PTR)wp;
        switch (id)
        {
        case SNAP_TIMER_ID:
            if (!AnimateSnappedWindow(mouse_in_window || (snap_sim_state == SnapSimulateState::SnapSimShow)))
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
    SnapState CheckSnapState()
    {
        RECT rect;
        if (!::GetWindowRect(GetHWnd(), &rect))
            return SnapNone;

        RECT rect_work = {0};
        if (!GetWorkAreaRect(&rect_work))
            return SnapNone;

        if (rect.left == rect_work.left)
            return SnapLeft;
        if (rect.top == rect_work.top)
            return SnapTop;
        if (rect.right == rect_work.right)
            return SnapRight;
        if (rect.bottom == rect_work.bottom)
            return SnapBottom;

        return SnapNone;
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
        switch (snap_state)
        {
        case SnapLeft:
            prc->left = rect_work.left;
            prc->right = prc->left + ww;
            break;

        case SnapTop:
            prc->top = rect_work.top;
            prc->bottom = prc->top + wh;
            break;

        case SnapRight:
            prc->right = rect_work.right;
            prc->left = prc->right - ww;
            break;

        case SnapBottom:
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
        if (snap_state != SnapNone)
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
        if (auto state = CheckSnapState(); state != SnapNone)
        {
            snap_state = state;
            snap_sim_state = SnapSimulateState::SnapSimHide;
            StartSnapAnimateTimer();
        }
    }

    VOID SimulateSnapToShow()
    {
        if (auto state = CheckSnapState(); (state == SnapNone) && (state != snap_state))
        {
            snap_sim_state = SnapSimulateState::SnapSimShow;
            StartSnapAnimateTimer();
        }
    }

private:
    inline VOID StartSnapAnimateTimer()
    {
        if (snap_timer == NULL)
        {
            snap_timer = ::SetTimer(GetHWnd(), SNAP_TIMER_ID, USER_TIMER_MINIMUM, NULL);
        }
    }

    inline VOID KillSnapAnimateTimer()
    {
        if (snap_timer)
        {
            ::KillTimer(GetHWnd(), snap_timer);
            snap_timer = NULL;
        }

        snap_sim_state = SnapSimulateState::SnapSimNone;
    }

    inline VOID StartMouseCheckTimer()
    {
        if (mouse_check_timer == NULL)
        {
            mouse_check_timer = ::SetTimer(GetHWnd(), MOUSE_CHECK_TIMER_ID, 100, NULL);
        }
    }

    inline VOID KillMouseCheckTimer()
    {
        if (mouse_check_timer)
        {
            ::KillTimer(GetHWnd(), mouse_check_timer);
            mouse_check_timer = NULL;
        }
    }

    VOID UpdateDPI()
    {
        if (HDC dc = ::GetDC(NULL))
        {
            dpi = GetDeviceCaps(dc, LOGPIXELSX);
            snap_detect_val = MulDiv(16, dpi, 96);
            snap_move_delta = MulDiv(kSnapMoveDelta, dpi, 96);
            ::ReleaseDC(NULL, dc);
        }
    }

    inline BOOL DetectSnap(int x1, int x2)
    {
        return std::abs(x1 - x2) < snap_detect_val;
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

        const int move_delta = snap_move_delta;
        int dx = 0, dy = 0;
        int& val = (snap_state == SnapLeft || snap_state == SnapRight) ? dx : dy;
        val = (snap_state == SnapLeft || snap_state == SnapTop) ? -1 * move_delta : move_delta;
        OffsetRect(&rect, revert ? -1 * dx : dx, revert ? -1 * dy : dy);

        BOOL animate_continue = TRUE;
        // ajust offset
        int ww = rect.right - rect.left;
        int wh = rect.bottom - rect.top;

        switch (snap_state)
        {
        case SnapLeft:
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

        case SnapTop:
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

        case SnapRight:
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

        case SnapBottom:
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
    bool enable_snap = false;
    int dpi = 96;
    int snap_dx = 0, snap_dy = 0;
    int snap_detect_val = 0;
    SnapState snap_state = SnapInvalid;
    UINT_PTR snap_timer = 0;
    UINT_PTR mouse_check_timer = 0;
    int kSnapHideEdgeWidth = 8;
    BOOL mouse_in_window = FALSE;
    const int kSnapMoveDelta = 45;
    int snap_move_delta = kSnapMoveDelta;
    SnapSimulateState snap_sim_state = SnapSimulateState::SnapSimHide;
};