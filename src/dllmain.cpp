// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <comdef.h>

extern ITypeLibPtr g_typelib;
HMODULE g_module = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            g_module = hModule;
            DisableThreadLibraryCalls(hModule);
            (VOID)OleInitialize(NULL);
            wchar_t module_path[MAX_PATH] = {};
            DWORD len = GetModuleFileNameW(hModule, module_path, MAX_PATH);
            module_path[len] = 0;
            (VOID)LoadTypeLibEx(module_path, REGKIND_NONE, &g_typelib);
        } break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

namespace {
DECLARE_COMPONENT_VERSION("Flowin", "0.2.0 beta", "ohyeah");
}