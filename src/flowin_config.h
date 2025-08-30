#pragma once
#include <memory>
#include <vector>
#include "flowin_callback.h"

class cfg_flowin_host
{
public:
    DECL_SMART_PTR(cfg_flowin_host);

    cfg_flowin_host()
    {
        reset();
    }

    void reset();
    void set_data_raw(stream_reader* reader, t_size size, abort_callback& abort);
    void get_data_raw(stream_writer* writer, abort_callback& abort);
    void write_subelement(ui_element_config::ptr data);
    ui_element_config::ptr subelement(unsigned id);
    ui_element_config::ptr build_configuration();
    static GUID cfg_get_guid(ui_element_config::ptr data);

public:
    GUID guid;
    bool show_on_startup;
    bool always_on_top;
    bool dock_to_taskbar;
    bool show_caption;
    bool show_minimize_box;
    bool show_maximize_box;
    bool snap_to_main_window;
    bool move_when_press_hot_key;
    bool enable_snap;
    bool enable_autohide_when_snapped;
    uint32_t move_modifiers;
    RECT window_rect;
    pfc::string8 window_title;
    GUID subelement_guid;
    mem_block_container_impl subelement_data;
    bool enable_transparency_active;
    uint32_t transparency;
    uint32_t transparency_active;
    struct
    {
        bool resizable : 1;
        bool draggable : 1;
        bool shadowed : 1;
        uint8_t legacy_no_frame; // internal use.
        uint8_t reserved[2];
    } cfg_no_frame;
    bool show_in_taskbar;
    bool bool_dummy[3];
    uint32_t reserved[18];
    // internal use
    bool edit_mode;

private:
    uint32_t version;
};

class cfg_flowin : public cfg_var
{
public:
    static cfg_flowin* get();
    cfg_flowin();
    // cfg_var
    void get_data_raw(stream_writer* p_stream, abort_callback& p_abort);
    void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort);

    void reset();

    void register_callback(cfg_flowin_callback::wp_t cb);

    cfg_flowin_host::sp_t find_configuration(const GUID& host_guid);
    cfg_flowin_host::sp_t add_or_find_configuration(const GUID& host_guid);
    void remove_configuration(const GUID& host_guid);

    template <typename t_callback> void enum_configuration(t_callback p_callback)
    {
        for (size_t n = 0, m = host_config_list_.size(); n < m; ++n)
        {
            p_callback(host_config_list_[n]);
        }
    }

    template <typename t_callback> void enum_configuration_v2(t_callback p_callback)
    {
        for (size_t n = 0, m = host_config_list_.size(); n < m; ++n)
        {
            if (p_callback(host_config_list_[n]))
            {
                return;
            }
        }
    }

private:
    inline cfg_flowin_host::sp_t new_host_configuration()
    {
        return std::make_shared<cfg_flowin_host>();
    }

public:
    bool show_debug_log;

private:
    uint32_t version;
    std::vector<cfg_flowin_host::sp_t> host_config_list_;
    std::vector<cfg_flowin_callback::wp_t> callbacks_;
};