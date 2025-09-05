#pragma once
#include <memory>
#include <vector>

class cfg_flowin_callback;

class cfg_flowin_host
{
public:
    using sp_t = std::shared_ptr<cfg_flowin_host>;

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
        bool rounded_corner : 1;
        uint8_t legacy_no_frame; // internal use.
        uint8_t reserved[2];
    } cfg_no_frame;

    static_assert(sizeof(cfg_no_frame) == sizeof(uint8_t) * 4, "unexpected no-frame configuration size");

    bool show_in_taskbar;
    bool bool_dummy[3];
    uint32_t reserved[18];
    // internal use
    bool edit_mode;

private:
    uint32_t version;
};

class cfg_flowin_host_comparator
{
private:
    GUID target_guid;

public:
    explicit cfg_flowin_host_comparator(const GUID& target) : target_guid(target)
    {
    }

    bool operator()(const cfg_flowin_host& cfg) const
    {
        return cfg.guid == target_guid;
    }

    bool operator()(const cfg_flowin_host::sp_t& cfg) const
    {
        return cfg->guid == target_guid;
    }
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

    void register_callback(cfg_flowin_callback* cb);
    void unregister_callback(cfg_flowin_callback* cb);

    cfg_flowin_host::sp_t find_configuration(const GUID& host_guid);
    cfg_flowin_host::sp_t add_or_find_configuration(const GUID& host_guid);
    void remove_configuration(const GUID& host_guid);

    size_t get_configuration_count() const
    {
        return host_config_list_.size();
    }

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
    bool show_debug_log = false;

private:
    uint32_t version;
    std::vector<cfg_flowin_host::sp_t> host_config_list_;
    std::vector<cfg_flowin_callback*> callbacks_;
};

namespace configuration
{
inline auto find(const GUID& host_guid)
{
    return cfg_flowin::get()->find_configuration(host_guid);
}

inline auto add_or_find(const GUID& host_guid)
{
    return cfg_flowin::get()->add_or_find_configuration(host_guid);
}

inline void remove(const GUID& host_guid)
{
    cfg_flowin::get()->remove_configuration(host_guid);
}

inline GUID guid_from_element_config(ui_element_config::ptr& data)
{
    return cfg_flowin_host::cfg_get_guid(data);
}

template <typename t_callback> inline void for_each(t_callback&& p_callback)
{
    cfg_flowin::get()->enum_configuration(std::move(p_callback));
}

inline size_t get_count() 
{
    return cfg_flowin::get()->get_configuration_count();
}

} // namespace configuration