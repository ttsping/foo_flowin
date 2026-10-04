#pragma once

namespace Utils
{

template <typename F, std::enable_if_t<std::is_function_v<std::remove_pointer_t<F>>, int> = 0>
bool GetProcAddress(HMODULE h, const char* funcName, F& f)
{
    if (auto ptr = ::GetProcAddress(h, funcName))
    {
        f = reinterpret_cast<F>(ptr);
        return true;
    }

    f = nullptr;
    return false;
}

bool IsCompositionEnabled();

bool IsMaximized(HWND wnd);

int32_t GetSystemMetrics(int32_t index, uint32_t dpi);

uint32_t CalculateCrc32(const void* data, size_t size);

} // namespace Utils