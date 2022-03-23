#include "pch.h"
#include "flowin_interface_impl.h"

extern HMODULE g_module;

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
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