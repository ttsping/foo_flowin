#pragma once
#include <memory>
#include <vector>

class CfgFlowinCallback;

class CfgFlowinHost
{
public:
    using Ptr = std::shared_ptr<CfgFlowinHost>;

    CfgFlowinHost()
    {
        Reset();
    }

    void Reset();
    void SetDataRaw(stream_reader* reader, t_size size, abort_callback& abort);
    void GetDataRaw(stream_writer* writer, abort_callback& abort);
    void WriteSubelement(ui_element_config::ptr data);
    ui_element_config::ptr Subelement(unsigned id);
    ui_element_config::ptr BuildConfiguration();
    static GUID CfgGetGuid(ui_element_config::ptr data);

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
    bool snap_to_edge;
    bool auto_hide_when_snapped;
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
    } cfg_frameless;

    static_assert(sizeof(cfg_frameless) == sizeof(uint8_t) * 4, "unexpected no-frame configuration size");

    bool show_in_taskbar;
    bool auto_hide_when_hovered;

    bool bool_reserved[32];

    bool do_not_use[3];
    uint32_t reserved[18];
    // internal use
    bool edit_mode;

private:
    uint32_t version;
};

class CfgFlowinHostComparator
{
private:
    GUID target_guid;

public:
    explicit CfgFlowinHostComparator(const GUID& target) : target_guid(target)
    {
    }

    bool operator()(const CfgFlowinHost& cfg) const
    {
        return cfg.guid == target_guid;
    }

    bool operator()(const CfgFlowinHost::Ptr& cfg) const
    {
        return cfg->guid == target_guid;
    }
};

class CfgFlowin : public cfg_var
{
public:
    static CfgFlowin* Get();
    CfgFlowin();
    // cfg_var
    void get_data_raw(stream_writer* p_stream, abort_callback& p_abort);
    void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort);

    void Reset();

    void RegisterCallback(CfgFlowinCallback* cb);
    void UnregisterCallback(CfgFlowinCallback* cb);

    CfgFlowinHost::Ptr FindConfiguration(const GUID& host_guid);
    CfgFlowinHost::Ptr AddOrFindConfiguration(const GUID& host_guid);
    void RemoveConfiguration(const GUID& host_guid);

    size_t GetConfigurationCount() const
    {
        return host_config_list.size();
    }

    template <typename t_callback> void EnumConfiguration(t_callback p_callback)
    {
        for (size_t n = 0, m = host_config_list.size(); n < m; ++n)
        {
            p_callback(host_config_list[n]);
        }
    }

    template <typename t_callback> void EnumConfigurationV2(t_callback p_callback)
    {
        for (size_t n = 0, m = host_config_list.size(); n < m; ++n)
        {
            if (p_callback(host_config_list[n]))
            {
                return;
            }
        }
    }

private:
    inline CfgFlowinHost::Ptr NewHostConfiguration()
    {
        return std::make_shared<CfgFlowinHost>();
    }

public:
    bool show_debug_log = false;

private:
    uint32_t version;
    std::vector<CfgFlowinHost::Ptr> host_config_list;
    std::vector<CfgFlowinCallback*> callbacks;
};

namespace Configuration
{
inline auto Find(const GUID& host_guid)
{
    return CfgFlowin::Get()->FindConfiguration(host_guid);
}

inline auto AddOrFind(const GUID& host_guid)
{
    return CfgFlowin::Get()->AddOrFindConfiguration(host_guid);
}

inline void Remove(const GUID& host_guid)
{
    CfgFlowin::Get()->RemoveConfiguration(host_guid);
}

inline GUID GuidFromElementConfig(ui_element_config::ptr& data)
{
    return CfgFlowinHost::CfgGetGuid(data);
}

template <typename t_callback> inline void ForEach(t_callback&& p_callback)
{
    CfgFlowin::Get()->EnumConfiguration(std::move(p_callback));
}

inline size_t GetCount() 
{
    return CfgFlowin::Get()->GetConfigurationCount();
}

} // namespace Configuration