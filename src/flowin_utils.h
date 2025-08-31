#pragma once

namespace utils
{

template <typename F, typename = std::enable_if_t<std::is_function<typename std::remove_pointer<F>::type>::value>>
bool get_proc_address(HMODULE h, const char* funcName, F& f)
{
    if (auto ptr = ::GetProcAddress(h, funcName))
    {
        f = reinterpret_cast<F>(ptr);
        return true;
    }

    f = nullptr;
    return false;
}

bool is_composition_enabled();

bool is_maximized(HWND wnd);

int32_t get_system_metrics(int32_t index, uint32_t dpi);

} // namespace flowin_utils