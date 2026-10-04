#include "pch.h"
#include "flowin_utils.h"
#include <array>
#include <mutex>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace Utils
{

int(WINAPI* pfnGetSystemMetricsForDpi)(int, UINT) = nullptr;
UINT(WINAPI* pfnGetDpiForSystem)() = nullptr;
UINT(WINAPI* pfnGetDpiForWindow)(HWND) = nullptr;
DPI_AWARENESS_CONTEXT(WINAPI* pfnGetThreadDpiAwarenessContext)() = nullptr;
DPI_AWARENESS(WINAPI* pfnGetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT value) = nullptr;

static std::once_flag static_load_flag;

static void LoadUtilsProcedures()
{
    // clang-format off
    std::call_once(static_load_flag, [&]()
    {
        if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
        {
            std::ignore = GetProcAddress(user32, "GetSystemMetricsForDpi", pfnGetSystemMetricsForDpi);
            std::ignore = GetProcAddress(user32, "GetDpiForSystem", pfnGetDpiForSystem);
            std::ignore = GetProcAddress(user32, "GetDpiForWindow", pfnGetDpiForWindow);
            std::ignore = GetProcAddress(user32, "GetThreadDpiAwarenessContext", pfnGetThreadDpiAwarenessContext);
            std::ignore = GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext", pfnGetAwarenessFromDpiAwarenessContext);
        }
    });
    // clang-format on
}

bool IsCompositionEnabled()
{
    BOOL composition_enabled = FALSE;
    return SUCCEEDED(::DwmIsCompositionEnabled(&composition_enabled)) && composition_enabled;
}

bool IsMaximized(HWND wnd)
{
    WINDOWPLACEMENT placement;
    return ::GetWindowPlacement(wnd, &placement) && placement.showCmd == SW_MAXIMIZE;
}

int32_t GetSystemMetrics(int32_t index, uint32_t dpi)
{
    LoadUtilsProcedures();

    if (pfnGetSystemMetricsForDpi != nullptr)
        return pfnGetSystemMetricsForDpi(index, dpi);

    const int32_t rc = ::GetSystemMetrics(index);
    if (dpi == USER_DEFAULT_SCREEN_DPI)
        return rc;

    if (pfnGetThreadDpiAwarenessContext != nullptr && pfnGetAwarenessFromDpiAwarenessContext != nullptr)
    {
        const auto awareness = pfnGetAwarenessFromDpiAwarenessContext(pfnGetThreadDpiAwarenessContext());
        if (awareness == DPI_AWARENESS_UNAWARE || awareness == DPI_AWARENESS_INVALID)
        {
            const double scaleFactor = static_cast<double>(dpi) / 96.0;
            return static_cast<int>(std::round(rc * scaleFactor));
        }
    }

    return rc;
}

uint32_t CalculateCrc32(const void* data, size_t size)
{
    auto generate_table = []() constexpr
    {
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j)
            {
                // 0xEDB88320 is the reversed polynomial for IEEE 802.3
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
            }
            table[i] = crc;
        }
        return table;
    };

    static constexpr auto crc_table = generate_table();

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);

    for (size_t i = 0; i < size; ++i)
        crc = crc_table[(crc ^ ptr[i]) & 0xFF] ^ (crc >> 8);

    // Final XOR to get the actual CRC value
    return crc ^ 0xFFFFFFFF;
}

} // namespace Utils