#pragma once
#include <memory>
#include <map>
#include "flowin_callback.h"

class flowin_core : public cfg_flowin_callback
{
public:
    using sp_t = std::shared_ptr<flowin_core>;

    static sp_t get();

    // cfg_flowin_callback
    void on_cfg_pre_write() override;

    void initalize();
    void finalize();

    void show_startup_flowin();

    void register_flowin(HWND hwnd, const GUID& guid);
    void unregister_flowin(HWND hwnd);

    bool is_flowin_alive(const GUID& host_guid);

    void set_latest_active_flowin(const GUID& host_guid);
    GUID get_latest_active_flowin() const;

    void set_instance_callback(ui_element_instance_callback_ptr callback)
    {
        callback_ = callback;
    }

    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

    GUID get_flowin_by_child(HWND child);
    GUID get_flowin_by_guid(const wchar_t* guid);
    GUID get_flowin_by_name(const wchar_t* name);

    ui_element_instance_ptr create_flowin(const GUID& inst_guid = pfc::guid_null);
    void remove_flowin(const GUID& host_guid, bool delete_config = false);

    HWND get_flowin_window(const GUID& host_guid);
    ui_element_instance_ptr get_flowin_instance(const GUID& host_guid);

    BOOL post_message(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    LRESULT send_message(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

    BOOL post_message(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    LRESULT send_message(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

private:
    ui_element_instance_callback_ptr callback_;
    service_list_t<ui_element_instance> flowin_hosts_;
    ui_element_popup_host::ptr dummy_element_inst_;
    GUID latest_active_flowin_guid_;
    std::map<HWND, GUID> alive_flowins_;
};