#include "pch.h"
#include "flowin_utils.h"
#include <mutex>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace utils
{

int(WINAPI* pfnGetSystemMetricsForDpi)(int, UINT) = nullptr;
UINT(WINAPI* pfnGetDpiForSystem)() = nullptr;
UINT(WINAPI* pfnGetDpiForWindow)(HWND) = nullptr;
DPI_AWARENESS_CONTEXT(WINAPI* pfnGetThreadDpiAwarenessContext)() = nullptr;
DPI_AWARENESS(WINAPI* pfnGetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT value) = nullptr;

static std::once_flag staticLoadFlag;

static void load_utils_procedures()
{
    // clang-format off
    std::call_once(staticLoadFlag, [&]()
    {
        if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
        {
            std::ignore = get_proc_address(user32, "GetSystemMetricsForDpi", pfnGetSystemMetricsForDpi);
            std::ignore = get_proc_address(user32, "GetDpiForSystem", pfnGetDpiForSystem);
            std::ignore = get_proc_address(user32, "GetDpiForWindow", pfnGetDpiForWindow);
            std::ignore = get_proc_address(user32, "GetThreadDpiAwarenessContext", pfnGetThreadDpiAwarenessContext);
            std::ignore = get_proc_address(user32, "GetAwarenessFromDpiAwarenessContext", pfnGetAwarenessFromDpiAwarenessContext);
        }
    });
    // clang-format on
}

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

int32_t get_system_metrics(int32_t index, uint32_t dpi)
{
    load_utils_procedures();

    if (pfnGetSystemMetricsForDpi != nullptr)
        return pfnGetSystemMetricsForDpi(index, dpi);

    const int32_t rc = GetSystemMetrics(index);
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

} // namespace flowin_utils