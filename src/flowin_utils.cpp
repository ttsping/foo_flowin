#include "pch.h"
#include "flowin_utils.h"

namespace flowin_utils {

bool add_or_remove_window_frame(HWND wnd, bool remove /*= true*/) {
    LONG style = GetWindowLong(wnd, GWL_STYLE);
    LONG style_check = WS_CAPTION | WS_THICKFRAME;
    style = remove ? style & ~style_check : style | style_check;
    SetWindowLong(wnd, GWL_STYLE, style);
    return !!SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool has_window_frame(HWND wnd) {
    LONG style = GetWindowLong(wnd, GWL_STYLE);
    return ((style & WS_CAPTION) != 0) || ((style & WS_THICKFRAME) != 0);
}

}  // namespace flowin_utils