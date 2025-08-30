#include "pch.h"
#include "flowin_utils.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace flowin_utils
{

bool is_composition_enabled()
{
    BOOL composition_enabled = FALSE;
    return SUCCEEDED(::DwmIsCompositionEnabled(&composition_enabled)) && composition_enabled;
}

bool is_maximized(HWND wnd)
{
    WINDOWPLACEMENT placement;
    return ::GetWindowPlacement(wnd, &placement) && placement.showCmd == SW_MAXIMIZE;
}

} // namespace flowin_utils