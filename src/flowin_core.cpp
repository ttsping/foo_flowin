#include "pch.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_vars.h"

class FlowinDummyPopupHostCallback : public ui_element_popup_host_callback
{
public:
    virtual void on_resize(t_uint32 /*width*/, t_uint32 /*height*/){};
    virtual void on_close() {};
    virtual void on_destroy() {};
};

FlowinCore::Ptr FlowinCore::Get()
{
    static Ptr core;
    return core ? core : core = std::make_shared<FlowinCore>();
}

void FlowinCore::OnCfgPreWrite()
{
    for (const auto& flowin : flowin_hosts)
    {
        SendFlowinMessage(flowin->get_wnd(), UWM_FLOWIN_REFRESH_CONFIG);
    }
}

void FlowinCore::Initialize()
{
    RETURN_VOID_IF(callback.is_valid() && dummy_element_inst.is_valid());

    latest_active_flowin_guid = pfc::guid_null;
    service_ptr_t<ui_element> dummy_element;
    if (ui_element::g_find(dummy_element, g_dui_dummy_element_guid))
    {
        auto dummy_callback = new service_impl_t<FlowinDummyPopupHostCallback>();
        dummy_element_inst = ui_element_common_methods::get()->spawn_host(
            HWND_DESKTOP, dummy_element->get_default_configuration(), dummy_callback, dummy_element, WS_POPUP);
    }

    CfgFlowin::Get()->RegisterCallback(this);
}

void FlowinCore::Finalize()
{
    // During the closing process, flowin will be removed from FlowinHost.
    // Therefore, the window handles should first be collected.
    std::vector<HWND> host_wnds;
    flowin_hosts.enumerate([&host_wnds](const ui_element_instance_ptr& ptr) { host_wnds.push_back(ptr->get_wnd()); });
    for (auto& hwnd : host_wnds)
        SendFlowinMessage(hwnd, WM_CLOSE);

    flowin_hosts.remove_all();
    callback.reset();
    dummy_element_inst.reset();
    latest_active_flowin_guid = pfc::guid_null;
    CfgFlowin::Get()->UnregisterCallback(this);
}

void FlowinCore::ShowStartupFlowin()
{
    Configuration::ForEach(
        [this](CfgFlowinHost::Ptr& config)
        {
            if (config->show_on_startup)
                this->CreateFlowin(config->guid);
        });
}

void FlowinCore::RegisterFlowin(HWND hwnd, const GUID& guid)
{
    alive_flowins[hwnd] = guid;
}

void FlowinCore::UnregisterFlowin(HWND hwnd)
{
    if (auto it = alive_flowins.find(hwnd); it != alive_flowins.end())
        alive_flowins.erase(it);
}

bool FlowinCore::IsFlowinAlive(const GUID& host_guid)
{
    for (auto& [_, guid] : alive_flowins)
    {
        if (guid == host_guid)
            return true;
    }

    return false;
}

void FlowinCore::SetLatestActiveFlowin(const GUID& host_guid)
{
    latest_active_flowin_guid = host_guid;
}

GUID FlowinCore::GetLatestActiveFlowin() const
{
    return latest_active_flowin_guid;
}

void FlowinCore::Notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
    for (auto& flowin : flowin_hosts)
    {
        if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
            PostFlowinMessage(flowin->get_wnd(), UWM_FLOWIN_REPAINT);

        if (p_what == ui_element_notify_colors_changed)
            PostFlowinMessage(flowin->get_wnd(), UWM_FLOWIN_COLOR_CHANGED);

        flowin->notify(p_what, p_param1, p_param2, p_param2size);
    }
}

GUID FlowinCore::GetFlowinByChild(HWND child)
{
    for (auto& [wnd, guid] : alive_flowins)
    {
        if (IsWindowChildOf(child, wnd))
            return guid;
    }

    return pfc::guid_null;
}

GUID FlowinCore::GetFlowinByGuid(const wchar_t* guid)
{
    if (guid == nullptr)
        return pfc::guid_null;

    std::wstring mod_guid{guid};
    if (mod_guid.front() != '{')
        mod_guid.insert(0, 1, '{');

    if (mod_guid.back() != '}')
        mod_guid.push_back('}');

    GUID id{};
    if (SUCCEEDED(CLSIDFromString(mod_guid.data(), &id)))
    {
        if (auto sp = CfgFlowin::Get()->FindConfiguration(id))
            return id;
    }

    return pfc::guid_null;
}

GUID FlowinCore::GetFlowinByName(const wchar_t* name)
{
    if (name == nullptr)
        return pfc::guid_null;

    GUID id{};
    Configuration::ForEach(
        [&id, name8 = pfc::stringcvt::string_utf8_from_wide(name)](const CfgFlowinHost::Ptr& config)
        {
            if (uStringCompare(name8, config->window_title) == 0)
            {
                id = config->guid;
            }
        });

    return id;
}

ui_element_instance_ptr FlowinCore::CreateFlowin(const GUID& inst_guid /*= pfc::guid_null*/)
{
    service_ptr_t<ui_element> host;
    if (ui_element::g_find(host, g_dui_flowin_host_guid))
    {
        ui_element_config::ptr config = Configuration::AddOrFind(inst_guid)->BuildConfiguration();
        ui_element_instance_ptr ptr = host->instantiate(HWND_DESKTOP, config, callback);
        if (ptr.is_empty() || !::IsWindow(ptr->get_wnd()))
            return nullptr;
        flowin_hosts.add_item(ptr);
        ::ShowWindow(ptr->get_wnd(), SW_SHOW);
        return ptr;
    }

    return nullptr;
}

void FlowinCore::RemoveFlowin(const GUID& host_guid, bool delete_config /*= false*/)
{
    for (t_size n = 0, m = flowin_hosts.get_count(); n < m; ++n)
    {
        auto& inst = flowin_hosts[n];
        if (CfgFlowinHost::CfgGetGuid(inst->get_configuration()) == host_guid)
        {
            UnregisterFlowin(inst->get_wnd());
            flowin_hosts.remove_by_idx(n);
            break;
        }
    }

    if (delete_config)
        Configuration::Remove(host_guid);

    if (GetLatestActiveFlowin() == host_guid)
        SetLatestActiveFlowin(pfc::guid_null);
}

HWND FlowinCore::GetFlowinWindow(const GUID& host_guid)
{
    for (auto& [wnd, guid] : alive_flowins)
    {
        if (guid == host_guid)
            return wnd;
    }

    return nullptr;
}

ui_element_instance_ptr FlowinCore::GetFlowinInstance(const GUID& host_guid)
{
    for (auto& flowin : flowin_hosts)
    {
        if (Configuration::GuidFromElementConfig(flowin->get_configuration()) == host_guid)
            return flowin;
    }

    return nullptr;
}

BOOL FlowinCore::PostFlowinMessage(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/)
{
    if (!::IsWindow(wnd))
    {
        return FALSE;
    }
    return ::PostMessage(wnd, msg, wp, lp);
}

LRESULT FlowinCore::SendFlowinMessage(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/)
{
    if (wnd == nullptr || !::IsWindow(wnd))
        return 0;

    return ::SendMessage(wnd, msg, wp, lp);
}

BOOL FlowinCore::PostFlowinMessage(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp)
{
    if (HWND hwnd = GetFlowinWindow(host_guid))
    {
        return PostFlowinMessage(hwnd, msg, wp, lp);
    }

    return FALSE;
}

LRESULT FlowinCore::SendFlowinMessage(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp)
{
    if (HWND hwnd = GetFlowinWindow(host_guid))
    {
        return SendFlowinMessage(hwnd, msg, wp, lp);
    }

    return FALSE;
}
