#pragma once
#include <memory>
#include "flowin_callback.h"

class flowin_core;
using flowin_core_ptr = std::shared_ptr<flowin_core>;

class flowin_core : public cfg_flowin_callback, public std::enable_shared_from_this<flowin_core> {
  public:
    static flowin_core_ptr get();

    // cfg_flowin_callback
    void on_cfg_pre_write() override;

    void initalize();
    void finalize();
    void show_startup_flowin();
    bool is_flowin_alive(const GUID& host_guid);
    void set_latest_active_flowin(const GUID& host_guid);
    GUID get_latest_active_flowin() const;
    void set_instance_callback(ui_element_instance_callback_ptr callback) { callback_ = callback; }
    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

    ui_element_instance_ptr create_flowin_host(const GUID& inst_guid = pfc::guid_null);
    void remove_flowin_host(const GUID& host_guid, bool delete_config = false);

    BOOL post_message(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    BOOL send_message(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

    BOOL post_message(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    BOOL send_message(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

  private:
    ui_element_instance_callback_ptr callback_;
    service_list_t<ui_element_instance> flowin_hosts_;
    ui_element_popup_host::ptr dummy_element_inst_;
    GUID latest_active_flowin_guid_;
};