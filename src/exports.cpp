#include "pch.h"
#include "flowin_interface_impl.h"

extern HMODULE g_module;

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }

    static wchar_t debugFile[MAX_PATH]{};
    if (debugFile[0] == '\0') {
        DWORD len = GetModuleFileNameW(g_module, debugFile, MAX_PATH);
        debugFile[len] = 0;
        PathRemoveFileSpecW(debugFile);
        PathAppendW(debugFile, L"FLOWIN_DISABLE_SCRIPT");
    }

    if (PathFileExistsW(debugFile)) {
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;
    if (rclsid == __uuidof(FlowinControlImpl)) {
        FlowinControlImplFactory* factory = new com_object_impl_t<FlowinControlImplFactory>();
        if (factory != nullptr) {
            hr = factory->QueryInterface(riid, ppv);
            factory->Release();
        }
    }

    return hr;
}