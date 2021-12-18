#include "pch.h"
#include "flowin_utils.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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

bool is_composition_enabled() {
    BOOL composition_enabled = FALSE;
    return SUCCEEDED(::DwmIsCompositionEnabled(&composition_enabled)) && composition_enabled;
}

bool is_maximized(HWND wnd) {
    WINDOWPLACEMENT placement;
    return ::GetWindowPlacement(wnd, &placement) && placement.showCmd == SW_MAXIMIZE;
}

}  // namespace flowin_utils