#pragma once

namespace utils
{

template <typename F, std::enable_if_t<std::is_function_v<std::remove_pointer_t<F>>, int> = 0>
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

uint32_t calculate_crc32(const void* data, size_t size);

} // namespace utils