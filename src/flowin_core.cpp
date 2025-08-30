#include "pch.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_vars.h"

class flowin_dummy_popup_host_callback : public ui_element_popup_host_callback
{
public:
    virtual void on_resize(t_uint32 /*width*/, t_uint32 /*height*/){};
    virtual void on_close() {};
    virtual void on_destroy() {};
};

flowin_core::sp_t flowin_core::get()
{
    static sp_t core_;
    return core_ ? core_ : core_ = std::make_shared<flowin_core>();
}

void flowin_core::on_cfg_pre_write()
{
    for (const auto& flowin : flowin_hosts_)
    {
        send_message(flowin->get_wnd(), UWM_FLOWIN_REFRESH_CONFIG);
    }
}

void flowin_core::initalize()
{
    RETURN_VOID_IF(callback_.is_valid() && dummy_element_inst_.is_valid());

    latest_active_flowin_guid_ = pfc::guid_null;
    service_ptr_t<ui_element> dummy_element;
    if (ui_element::g_find(dummy_element, g_dui_dummy_element_guid))
    {
        auto callback = new service_impl_t<flowin_dummy_popup_host_callback>();
        dummy_element_inst_ = ui_element_common_methods::get()->spawn_host(
            HWND_DESKTOP, dummy_element->get_default_configuration(), callback, dummy_element, WS_POPUP);
    }

    cfg_flowin::get()->register_callback(this);
}

void flowin_core::finalize()
{
    // During the closing process, flowin will be removed from flowin_host.
    // Therefore, the window handles should first be collected.
    std::vector<HWND> host_wnds;
    flowin_hosts_.enumerate([&host_wnds](const ui_element_instance_ptr& ptr) { host_wnds.push_back(ptr->get_wnd()); });
    for (auto& hwnd : host_wnds)
        send_message(hwnd, WM_CLOSE);

    flowin_hosts_.remove_all();
    callback_.reset();
    dummy_element_inst_.reset();
    latest_active_flowin_guid_ = pfc::guid_null;
    cfg_flowin::get()->unregister_callback(this);
}

void flowin_core::show_startup_flowin()
{
    configuration::for_each(
        [this](cfg_flowin_host::sp_t& config)
        {
            if (config->show_on_startup)
                this->create_flowin(config->guid);
        });
}

void flowin_core::register_flowin(HWND hwnd, const GUID& guid)
{
    alive_flowins_[hwnd] = guid;
}

void flowin_core::unregister_flowin(HWND hwnd)
{
    if (auto it = alive_flowins_.find(hwnd); it != alive_flowins_.end())
        alive_flowins_.erase(it);
}

bool flowin_core::is_flowin_alive(const GUID& host_guid)
{
    for (auto& [_, guid] : alive_flowins_)
    {
        if (guid == host_guid)
            return true;
    }

    return false;
}

void flowin_core::set_latest_active_flowin(const GUID& host_guid)
{
    latest_active_flowin_guid_ = host_guid;
}

GUID flowin_core::get_latest_active_flowin() const
{
    return latest_active_flowin_guid_;
}

void flowin_core::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
    for (auto& flowin : flowin_hosts_)
    {
        if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
            post_message(flowin->get_wnd(), UWM_FLOWIN_REPAINT);

        if (p_what == ui_element_notify_colors_changed)
            post_message(flowin->get_wnd(), UWM_FLOWIN_COLOR_CHANGED);

        flowin->notify(p_what, p_param1, p_param2, p_param2size);
    }
}

GUID flowin_core::get_flowin_by_child(HWND child)
{
    for (auto& [wnd, guid] : alive_flowins_)
    {
        if (IsWindowChildOf(child, wnd))
            return guid;
    }

    return pfc::guid_null;
}

GUID flowin_core::get_flowin_by_guid(const wchar_t* guid)
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
        if (auto sp = cfg_flowin::get()->find_configuration(id))
            return id;
    }

    return pfc::guid_null;
}

GUID flowin_core::get_flowin_by_name(const wchar_t* name)
{
    if (name == nullptr)
        return pfc::guid_null;

    GUID id{};
    configuration::for_each(
        [&id, name8 = pfc::stringcvt::string_utf8_from_wide(name)](const cfg_flowin_host::sp_t& config)
        {
            if (uStringCompare(name8, config->window_title) == 0)
            {
                id = config->guid;
            }
        });

    return id;
}

ui_element_instance_ptr flowin_core::create_flowin(const GUID& inst_guid /*= pfc::guid_null*/)
{
    service_ptr_t<ui_element> host;
    if (ui_element::g_find(host, g_dui_flowin_host_guid))
    {
        ui_element_config::ptr config = configuration::add_or_find(inst_guid)->build_configuration();
        ui_element_instance_ptr ptr = host->instantiate(HWND_DESKTOP, config, callback_);
        if (ptr.is_empty() || !::IsWindow(ptr->get_wnd()))
            return nullptr;
        flowin_hosts_.add_item(ptr);
        ::ShowWindow(ptr->get_wnd(), SW_SHOW);
        return ptr;
    }

    return nullptr;
}

void flowin_core::remove_flowin(const GUID& host_guid, bool delete_config /*= false*/)
{
    for (t_size n = 0, m = flowin_hosts_.get_count(); n < m; ++n)
    {
        auto& inst = flowin_hosts_[n];
        if (cfg_flowin_host::cfg_get_guid(inst->get_configuration()) == host_guid)
        {
            unregister_flowin(inst->get_wnd());
            flowin_hosts_.remove_by_idx(n);
            break;
        }
    }

    if (delete_config)
        configuration::remove(host_guid);

    if (get_latest_active_flowin() == host_guid)
        set_latest_active_flowin(pfc::guid_null);
}

HWND flowin_core::get_flowin_window(const GUID& host_guid)
{
    for (auto& [wnd, guid] : alive_flowins_)
    {
        if (guid == host_guid)
            return wnd;
    }

    return nullptr;
}

ui_element_instance_ptr flowin_core::get_flowin_instance(const GUID& host_guid)
{
    for (auto& flowin : flowin_hosts_)
    {
        if (configuration::guid_from_element_config(flowin->get_configuration()) == host_guid)
            return flowin;
    }

    return nullptr;
}

BOOL flowin_core::post_message(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/)
{
    if (!::IsWindow(wnd))
    {
        return FALSE;
    }
    return ::PostMessage(wnd, msg, wp, lp);
}

LRESULT flowin_core::send_message(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/)
{
    if (wnd == nullptr || !::IsWindow(wnd))
        return 0;

    return ::SendMessage(wnd, msg, wp, lp);
}

BOOL flowin_core::post_message(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp)
{
    if (HWND hwnd = get_flowin_window(host_guid))
    {
        return post_message(hwnd, msg, wp, lp);
    }

    return FALSE;
}

LRESULT flowin_core::send_message(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp)
{
    if (HWND hwnd = get_flowin_window(host_guid))
    {
        return send_message(hwnd, msg, wp, lp);
    }

    return FALSE;
}
