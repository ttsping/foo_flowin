#include "pch.h"
#include "flowin_interface_impl.h"
#include "flowin_core.h"
#include "flowin_vars.h"

ITypeLibPtr g_typelib;
type_info_cache g_type_info_cache;

FlowinHostImpl::FlowinHostImpl(GUID guid) : host_guid_(guid), host_window_(nullptr), config_(nullptr)
{
    config_ = cfg_flowin::get()->find_configuration(host_guid_);
}

FlowinHostImpl::~FlowinHostImpl()
{
}

STDMETHODIMP FlowinHostImpl::get_Left(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.left;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Top(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.top;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Width(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.right - rc.left;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Height(INT* p)
{
    RETURN_HR_IF(E_POINTER, p == nullptr);

    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    *p = rc.bottom - rc.top;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Show(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    *pp = TO_VARIANT_BOOL(flowin_core::get()->is_flowin_alive(host_guid_));
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_AlwaysOnTop(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->always_on_top);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_NoFrame(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(!config_->show_caption);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_SnapToEdge(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->enable_snap);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_AutoHideWhenSnap(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->enable_autohide_when_snapped);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::get_Title(BSTR* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8(config_->window_title));
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameResizable(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->cfg_no_frame.resizable);
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameShadow(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->cfg_no_frame.shadowed);
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::get_NoFrameMovable(VARIANT_BOOL* pp)
{
    RETURN_HR_IF(E_POINTER, pp == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    *pp = TO_VARIANT_BOOL(config_->cfg_no_frame.draggable);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Left(INT p)
{
    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, p, rc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Top(INT p)
{
    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, rc.left, p, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Width(INT p)
{
    RETURN_HR_IF(E_INVALIDARG, p <= 0);
    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, 0, 0, p, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Height(INT p)
{
    RETURN_HR_IF(E_INVALIDARG, p <= 0);
    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);
    SetWindowPos(wnd, nullptr, 0, 0, rc.right - rc.left, p, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Show(VARIANT_BOOL p)
{
    if (flowin_core::get()->is_flowin_alive(host_guid_))
    {
        if (p == VARIANT_FALSE)
            flowin_core::get()->post_message(host_guid_, WM_CLOSE);
    }
    else
    {
        if (p == VARIANT_TRUE)
        {
            if (auto inst = flowin_core::get()->create_flowin(host_guid_); inst == nullptr)
                return E_FAIL;
        }
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_AlwaysOnTop(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    if (TO_VARIANT_BOOL(config_->always_on_top) == p)
        return S_OK;

    if (flowin_core::get()->is_flowin_alive(host_guid_))
    {
        flowin_core::get()->post_message(host_guid_, UWM_FLOWIN_COMMAND, (WPARAM)t_menu_cmd_always_on_top);
    }
    else
    {
        config_->always_on_top = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_NoFrame(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    if (TO_VARIANT_BOOL(config_->show_caption) != p)
        return S_OK;

    if (flowin_core::get()->is_flowin_alive(host_guid_))
    {
        flowin_core::get()->post_message(host_guid_, UWM_FLOWIN_COMMAND, (WPARAM)t_menu_cmd_flowin_no_frame,
                                         (LPARAM)TRUE);
    }
    else
    {
        config_->show_caption = p ? false : true;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_SnapToEdge(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    if (TO_VARIANT_BOOL(config_->enable_snap) == p)
        return S_OK;

    if (flowin_core::get()->is_flowin_alive(host_guid_))
    {
        flowin_core::get()->post_message(host_guid_, UWM_FLOWIN_COMMAND, (WPARAM)t_menu_cmd_snap_to_edge);
    }
    else
    {
        config_->enable_snap = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_AutoHideWhenSnap(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    if (TO_VARIANT_BOOL(config_->enable_autohide_when_snapped) == p)
        return S_OK;

    if (flowin_core::get()->is_flowin_alive(host_guid_))
    {
        flowin_core::get()->post_message(host_guid_, UWM_FLOWIN_COMMAND, (WPARAM)t_menu_cmd_snap_auto_hide);
    }
    else
    {
        config_->enable_autohide_when_snapped = p ? true : false;
    }

    return S_OK;
}

STDMETHODIMP FlowinHostImpl::put_Title(BSTR p)
{
    RETURN_HR_IF(E_INVALIDARG, p == nullptr);
    RETURN_HR_IF(E_FAIL, config_ == nullptr);

    pfc::stringcvt::string_utf8_from_wide name8(p);
    if (name8.length() == 0)
        return E_INVALIDARG;

    config_->window_title = name8;
    if (HWND hwnd = try_get_flowin_window())
        SetWindowTextW(hwnd, p);

    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameResizable(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    config_->cfg_no_frame.resizable = p ? true : false;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameShadow(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    config_->cfg_no_frame.shadowed = p ? true : false;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) FlowinHostImpl::put_NoFrameMovable(VARIANT_BOOL p)
{
    RETURN_HR_IF(E_FAIL, config_ == nullptr);
    config_->cfg_no_frame.draggable = p ? true : false;
    return S_OK;
}

STDMETHODIMP FlowinHostImpl::Move(INT x, INT y, INT width, INT height)
{
    HWND wnd = try_get_flowin_window();
    RETURN_HR_IF(E_FAIL, wnd == nullptr);

    RECT rc{};
    GetWindowRect(wnd, &rc);

    DWORD flags = SWP_NOACTIVATE | SWP_NOZORDER;
    if (width < 0 || height < 0)
        flags |= SWP_NOSIZE;

    SetWindowPos(wnd, nullptr, x, y, width, height, flags);
    return S_OK;
}

HWND FlowinHostImpl::try_get_flowin_window()
{
    if (host_window_ == nullptr || !IsWindow(host_window_))
        host_window_ = flowin_core::get()->get_flowin_window(host_guid_);

    return host_window_;
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
    GUID host_guid = flowin_core::get()->get_flowin_by_child(reinterpret_cast<HWND>(child_id));
#pragma warning(pop)
    if (host_guid == pfc::guid_null)
        return E_FAIL;

    *pp = new com_object_impl_t<FlowinHostImpl>(host_guid);
    return S_OK;
}

STDMETHODIMP FlowinControlImpl::FindByName(BSTR window_title, IFlowinHost** pp)
{
    RETURN_HR_IF(E_INVALIDARG, window_title == nullptr);
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    GUID host_guid = flowin_core::get()->get_flowin_by_name(window_title);
    if (host_guid == pfc::guid_null)
        return E_FAIL;

    *pp = new com_object_impl_t<FlowinHostImpl>(host_guid);
    return S_OK;
}

STDMETHODIMP FlowinControlImpl::FindByGuid(BSTR host_guid, IFlowinHost** pp)
{
    RETURN_HR_IF(E_INVALIDARG, host_guid == nullptr);
    RETURN_HR_IF(E_POINTER, pp == nullptr);

    GUID guid = flowin_core::get()->get_flowin_by_guid(host_guid);
    if (guid == pfc::guid_null)
        return E_FAIL;

    *pp = new com_object_impl_t<FlowinHostImpl>(guid);
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

    FlowinControlImpl* impl = new com_object_impl_t<FlowinControlImpl>();
    hr = impl->QueryInterface(riid, ppv);
    impl->Release();

    return hr;
}
