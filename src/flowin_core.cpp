#include "pch.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_vars.h"

class flowin_dummy_popup_host_callback : public ui_element_popup_host_callback {
  public:
    virtual void on_resize(t_uint32 /*width*/, t_uint32 /*height*/){};
    virtual void on_close(){};
    virtual void on_destroy(){};
};

flowin_core_ptr flowin_core::get() {
    static flowin_core_ptr core_;
    if (!core_)
        core_ = std::make_shared<flowin_core>();
    return core_;
}

void flowin_core::on_cfg_pre_write() {
    flowin_hosts_.enumerate([this](const ui_element_instance_ptr& ptr) { this->send_message(ptr->get_wnd(), UWM_FLOWIN_REFRESH_CONFIG); });
}

void flowin_core::initalize() {
    if (callback_.is_valid() && dummy_element_inst_.is_valid()) {
        return;
    }
    latest_active_flowin_guid_ = pfc::guid_null;
    service_ptr_t<ui_element> dummy_element;
    if (ui_element::g_find(dummy_element, g_dui_dummy_element_guid)) {
        auto callback = new service_impl_t<flowin_dummy_popup_host_callback>();
        dummy_element_inst_ =
            ui_element_common_methods::get()->spawn_host(HWND_DESKTOP, dummy_element->get_default_configuration(), callback, dummy_element, WS_POPUP);
    }
    cfg_flowin::get()->register_callback(shared_from_this());
}

void flowin_core::finalize() {
    flowin_hosts_.enumerate([this](const ui_element_instance_ptr& ptr) { this->send_message(ptr->get_wnd(), WM_CLOSE); });
    flowin_hosts_.remove_all();
    callback_.reset();
    dummy_element_inst_.reset();
    latest_active_flowin_guid_ = pfc::guid_null;
}

void flowin_core::show_startup_flowin() {
    cfg_flowin::get()->enum_configuration([this](cfg_flowin_host::sp_t host_config) {
        if (host_config->show_on_startup) {
            this->create_flowin_host(host_config->guid);
        }
    });
}

bool flowin_core::is_flowin_alive(const GUID& host_guid) {
    for (t_size n = 0, m = flowin_hosts_.get_count(); n < m; ++n) {
        auto& inst = flowin_hosts_[n];
        if (cfg_flowin_host::cfg_get_guid(inst->get_configuration()) == host_guid) {
            return !!::IsWindow(inst->get_wnd());
        }
    }
    return false;
}

void flowin_core::set_latest_active_flowin(const GUID& host_guid) { latest_active_flowin_guid_ = host_guid; }

GUID flowin_core::get_latest_active_flowin() const { return latest_active_flowin_guid_; }

void flowin_core::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) {
    t_size n, m = flowin_hosts_.get_count();
    for (n = 0; n < m; ++n) {
        auto& inst = flowin_hosts_[n];
        inst->notify(p_what, p_param1, p_param2, p_param2size);
    }
}

ui_element_instance_ptr flowin_core::create_flowin_host(const GUID& inst_guid /*= pfc::guid_null*/) {
    service_ptr_t<ui_element> host;
    if (ui_element::g_find(host, g_dui_flowin_host_guid)) {
        ui_element_config::ptr config = cfg_flowin::get()->add_or_find_configuration(inst_guid)->build_configuration();
        ui_element_instance_ptr ptr = host->instantiate(HWND_DESKTOP, config, callback_);
        if (ptr.is_empty() || !::IsWindow(ptr->get_wnd()))
            return nullptr;
        flowin_hosts_.add_item(ptr);
        ::ShowWindow(ptr->get_wnd(), SW_SHOW);
        return ptr;
    }
    return nullptr;
}

void flowin_core::remove_flowin_host(const GUID& host_guid, bool delete_config /*= false*/) {
    for (t_size n = 0, m = flowin_hosts_.get_count(); n < m; ++n) {
        auto& inst = flowin_hosts_[n];
        if (cfg_flowin_host::cfg_get_guid(inst->get_configuration()) == host_guid) {
            flowin_hosts_.remove_by_idx(n);
            break;
        }
    }

    if (delete_config) {
        cfg_flowin::get()->remove_configuration(host_guid);
    }

    if (get_latest_active_flowin() == host_guid) {
        set_latest_active_flowin(pfc::guid_null);
    }
}

BOOL flowin_core::post_message(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/) {
    if (!::IsWindow(wnd)) {
        return FALSE;
    }
    return ::PostMessage(wnd, msg, wp, lp);
}

BOOL flowin_core::send_message(HWND wnd, UINT msg, WPARAM wp /*= 0*/, LPARAM lp /*= 0*/) {
    if (!::IsWindow(wnd)) {
        return FALSE;
    }
    return ::SendMessage(wnd, msg, wp, lp);
}

BOOL flowin_core::post_message(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp) {
    for (t_size n = 0, m = flowin_hosts_.get_count(); n < m; ++n) {
        auto& inst = flowin_hosts_[n];
        if (cfg_flowin_host::cfg_get_guid(inst->get_configuration()) == host_guid) {
            return post_message(inst->get_wnd(), msg, wp, lp);
        }
    }
    return FALSE;
}

BOOL flowin_core::send_message(const GUID& host_guid, UINT msg, WPARAM wp, LPARAM lp) {
    for (t_size n = 0, m = flowin_hosts_.get_count(); n < m; ++n) {
        auto& inst = flowin_hosts_[n];
        if (cfg_flowin_host::cfg_get_guid(inst->get_configuration()) == host_guid) {
            return send_message(inst->get_wnd(), msg, wp, lp);
        }
    }
    return FALSE;
}
