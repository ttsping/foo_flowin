#pragma once

namespace flowin_utils {

bool add_or_remove_window_frame(HWND wnd, bool remove = true);

bool has_window_frame(HWND wnd);

bool is_composition_enabled();

bool is_maximized(HWND wnd);

}  // namespace flowin_utils