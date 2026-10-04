#include "pch.h"
#include "flowin_interface_impl.h"
#include "flowin_core.h"
#include "flowin_vars.h"

ITypeLibPtr g_typelib;
TypeInfoCache g_type_info_cache;

FlowinHostImpl::FlowinHostImpl(GUID guid) : host_guid(guid), host_window(nullptr), config(nullptr)
{
    config = CfgFlowin::Get()->FindConfiguration(host_guid);
}

FlowinHostImpl::~FlowinHostImpl()
{
}

STDMETHODIMP FlowinHostImpl::get_Left(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.left;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Top(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.top;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Width(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.right - rc.left;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Height(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.bottom - rc.top;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Show(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    *pp = TO_VARIANT_BOOL(FlowinCore::Get()->IsFlowinAlive(host_guid));
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_AlwaysOnTop(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->always_on_top);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_NoFrame(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(!config->show_caption);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_SnapToEdge(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->snap_to_edge);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_AutoHideWhenSnap(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->auto_hide_when_snapped);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Title(BSTR* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8(config->window_title));
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameResizable(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->cfg_frameless.resizable);
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameShadow(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->cfg_frameless.shadowed);
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameMovable(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    *pp = TO_VARIANT_BOOL(config->cfg_frameless.draggable);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Left(INT p)
{
    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, p, rc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Top(INT p)
{
    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, rc.left, p, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Width(INT p)
{
    RETURN_HR_IF(E_INVALIDARG, p <= 0);
    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, 0, 0, p, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Height(INT p)
{
    RETURN_HR_IF(E_INVALIDARG, p <= 0);
    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, 0, 0, rc.right - rc.left, p, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Show(VARIANT_BOOL p)
{
    if (FlowinCore::Get()->IsFlowinAlive(host_guid))
    {
        if (p == VARIANT_FALSE)
            FlowinCore::Get()->PostFlowinMessage(host_guid, WM_CLOSE);
    }
    else
    {
        if (p == VARIANT_TRUE)
        {
            if (auto inst = FlowinCore::Get()->CreateFlowin(host_guid); inst == nullptr)
                return E_FAIL;
        }
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_AlwaysOnTop(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    if (TO_VARIANT_BOOL(config->always_on_top) == p)
        return S_OK;

    if (FlowinCore::Get()->IsFlowinAlive(host_guid))
    {
        FlowinCore::Get()->PostFlowinMessage(host_guid, UWM_FLOWIN_COMMAND, (WPARAM)Flowin::MenuCommands::AlwaysOnTop);
    }
    else
    {
        config->always_on_top = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_NoFrame(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    if (TO_VARIANT_BOOL(config->show_caption) != p)
        return S_OK;

    if (FlowinCore::Get()->IsFlowinAlive(host_guid))
    {
        FlowinCore::Get()->PostFlowinMessage(host_guid, UWM_FLOWIN_COMMAND,
                                         (WPARAM)Flowin::MenuCommands::NoFrameSilent);
    }
    else
    {
        config->show_caption = p ? false : true;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_SnapToEdge(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    if (TO_VARIANT_BOOL(config->snap_to_edge) == p)
        return S_OK;

    if (FlowinCore::Get()->IsFlowinAlive(host_guid))
    {
        FlowinCore::Get()->PostFlowinMessage(host_guid, UWM_FLOWIN_COMMAND, (WPARAM)Flowin::MenuCommands::SnapToEdge);
    }
    else
    {
        config->snap_to_edge = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_AutoHideWhenSnap(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    if (TO_VARIANT_BOOL(config->auto_hide_when_snapped) == p)
        return S_OK;

    if (FlowinCore::Get()->IsFlowinAlive(host_guid))
    {
        FlowinCore::Get()->PostFlowinMessage(host_guid, UWM_FLOWIN_COMMAND, (WPARAM)Flowin::MenuCommands::AutoHideWhenSnapped);
    }
    else
    {
        config->auto_hide_when_snapped = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Title(BSTR p)
{
    RETURN_HR_IF(E_INVALIDARG, p == nullptr);
    RETURN_HR_IF(E_FAIL, config == nullptr);

    pfc::stringcvt::string_utf8_from_wide name8(p);
    if (name8.length() == 0)
        return E_INVALIDARG;

    config->window_title = name8;
    if (HWND hwnd = TryGetFlowinWindow())
        SetWindowTextW(hwnd, p);

    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameResizable(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    config->cfg_frameless.resizable = p ? true : false;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameShadow(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    config->cfg_frameless.shadowed = p ? true : false;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameMovable(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config == nullptr);
    config->cfg_frameless.draggable = p ? true : false;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::Move(INT x, INT y, INT width, INT height)
{
    HWND wnd = TryGetFlowinWindow();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);

    DWORD flags = SWP_NOACTIVATE | SWP_NOZORDER;
    if (width < 0 || height < 0)
        flags |= SWP_NOSIZE;

    SetWindowPos(wnd, nullptr, x, y, width, height, flags);
    return S_OK;
}

HWND FlowinHostImpl::TryGetFlowinWindow()
{
    if (host_window == nullptr || !IsWindow(host_window))
        host_window = FlowinCore::Get()->GetFlowinWindow(host_guid);

    return host_window;
}

FlowinControlImpl::FlowinControlImpl()
{
}

FlowinControlImpl::~FlowinControlImpl()
{
}

STDMETHODIMP FlowinControlImpl::FindByChild(UINT child_id, IFlowinHost** pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
#pragma warning(push)
#pragma warning(disable : 4312)
    GUID host_guid = FlowinCore::Get()->GetFlowinByChild(reinterpret_cast<HWND>(child_id));
#pragma warning(pop)
    if (host_guid == pfc::guid_null)
        return E_FAIL;

    *pp = new ComObjectImpl<FlowinHostImpl>(host_guid);
    return S_OK;
}

STDMETHODIMP FlowinControlImpl::FindByName(BSTR window_title, IFlowinHost** pp)
{
    RETURN_HR_IF(E_INVALIDARG, window_title == nullptr);
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    GUID host_guid = FlowinCore::Get()->GetFlowinByName(window_title);
    if (host_guid == pfc::guid_null)
        return E_FAIL;

    *pp = new ComObjectImpl<FlowinHostImpl>(host_guid);
    return S_OK;
}

STDMETHODIMP FlowinControlImpl::FindByGuid(BSTR host_guid, IFlowinHost** pp)
{
    RETURN_HR_IF(E_INVALIDARG, host_guid == nullptr);
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    GUID guid = FlowinCore::Get()->GetFlowinByGuid(host_guid);
    if (guid == pfc::guid_null)
        return E_FAIL;

    *pp = new ComObjectImpl<FlowinHostImpl>(guid);
    return S_OK;
}

FlowinControlImplFactory::FlowinControlImplFactory()
{
}

FlowinControlImplFactory::~FlowinControlImplFactory()
{
}

STDMETHODIMP FlowinControlImplFactory::CreateInstance(IUnknown* outer, REFIID riid, void** ppv)
{
    RETURN_HR_IF(E_INVALIDARG, ppv == nullptr);
    RETURN_HR_IF(CLASS_E_NOAGGREGATION, outer != nullptr);

    HRESULT hr = S_OK;
    *ppv = nullptr;

    FlowinControlImpl* impl = new ComObjectImpl<FlowinControlImpl>();
    hr = impl->QueryInterface(riid, ppv);
    impl->Release();

    return hr;
}
