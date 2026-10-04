#include "pch.h"
#include "flowin_config.h"
#include "flowin_vars.h"
#include "flowin_callback.h"

CfgFlowin g_flowin_config;

CfgFlowin* CfgFlowin::Get()
{
    return &g_flowin_config;
}

enum FlowinConfigVersion
{
    Version010 = 1,
    Version011 = 3,
    Version012 = 5,
    VersionCurrent = Version012
};

void CfgFlowinHost::Reset()
{
    version = VersionCurrent;
    show_on_startup = true;
    always_on_top = false;
    dock_to_taskbar = false;
    show_caption = true;
    show_maximize_box = false;
    show_minimize_box = false;
    snap_to_main_window = false;
    move_when_press_hot_key = true;
    snap_to_edge = false;
    auto_hide_when_snapped = true;
    window_title = "Flowin";
    guid = pfc::guid_null;
    subelement_guid = pfc::guid_null;
    edit_mode = false;
    enable_transparency_active = false;
    transparency = 0;
    transparency_active = 0;
    auto_hide_when_hovered = false;
    ZeroMemory(&cfg_frameless, sizeof(cfg_frameless));
    cfg_frameless.shadowed = true;
    cfg_frameless.resizable = true;
    cfg_frameless.draggable = true;
    cfg_frameless.rounded_corner = true;

    ZeroMemory(&window_rect, sizeof(window_rect));
    ZeroMemory(reserved, sizeof(reserved));
    ZeroMemory(bool_reserved, sizeof(bool_reserved));
}

void CfgFlowinHost::SetDataRaw(stream_reader* reader, t_size size, abort_callback& abort)
{
    Reset();
    if (size < sizeof(version))
        return;

    try
    {
        t_size cfg_data_size = 0;
        reader->read_lendian_t(version, abort);
        switch (version)
        {
        case Version012:
            reader->read_object_t(auto_hide_when_hovered, abort);
            reader->read_object(bool_reserved, sizeof(bool_reserved), abort);
            [[fallthrough]];
        case Version011:
            reader->read_object_t(enable_transparency_active, abort);
            reader->read_lendian_t(transparency, abort);
            reader->read_lendian_t(transparency_active, abort);
            reader->read_object(&cfg_frameless, sizeof(cfg_frameless), abort);
            reader->read_object_t(show_in_taskbar, abort);
            reader->read_object(do_not_use, sizeof(do_not_use), abort);
            reader->read_object(reserved, sizeof(reserved), abort);
            [[fallthrough]];
        case Version010:
            reader->read_object_t(guid, abort);
            reader->read_object_t(show_on_startup, abort);
            reader->read_object_t(always_on_top, abort);
            reader->read_object_t(dock_to_taskbar, abort);
            reader->read_object_t(show_caption, abort);
            reader->read_object_t(show_minimize_box, abort);
            reader->read_object_t(show_maximize_box, abort);
            reader->read_object_t(snap_to_main_window, abort);
            reader->read_object_t(move_when_press_hot_key, abort);
            reader->read_object_t(snap_to_edge, abort);
            reader->read_object_t(auto_hide_when_snapped, abort);
            reader->read_object(&window_rect, sizeof(window_rect), abort);
            reader->read_lendian_t(move_modifiers, abort);
            reader->read_string_nullterm(window_title, abort);
            reader->read_object_t(subelement_guid, abort);
            reader->read_lendian_t(cfg_data_size, abort);
            if (cfg_data_size > 0)
                subelement_data.from_stream(reader, cfg_data_size, abort);
            break;

        default:
            break;
        }
    }
    catch (std::exception&)
    {
        Reset();
    }
}

void CfgFlowinHost::GetDataRaw(stream_writer* writer, abort_callback& abort)
{
    try
    {
        uint32_t ver = VersionCurrent;
        writer->write_lendian_t(ver, abort);
        // version 012
        writer->write_object_t(auto_hide_when_hovered, abort);
        writer->write_object(bool_reserved, sizeof(bool_reserved), abort);
        // version 011
        writer->write_object_t(enable_transparency_active, abort);
        writer->write_lendian_t(transparency, abort);
        writer->write_lendian_t(transparency_active, abort);
        writer->write_object(&cfg_frameless, sizeof(cfg_frameless), abort);
        writer->write_object_t(show_in_taskbar, abort);
        writer->write_object(do_not_use, sizeof(do_not_use), abort);
        writer->write_object(reserved, sizeof(reserved), abort);
        // version 010
        writer->write_object_t(guid, abort);
        writer->write_object_t(show_on_startup, abort);
        writer->write_object_t(always_on_top, abort);
        writer->write_object_t(dock_to_taskbar, abort);
        writer->write_object_t(show_caption, abort);
        writer->write_object_t(show_minimize_box, abort);
        writer->write_object_t(show_maximize_box, abort);
        writer->write_object_t(snap_to_main_window, abort);
        writer->write_object_t(move_when_press_hot_key, abort);
        writer->write_object_t(snap_to_edge, abort);
        writer->write_object_t(auto_hide_when_snapped, abort);
        writer->write_object(&window_rect, sizeof(window_rect), abort);
        writer->write_lendian_t(move_modifiers, abort);
        writer->write_string_nullterm(window_title, abort);
        writer->write_object_t(subelement_guid, abort);
        {
            t_size config_size = subelement_data.get_size();
            writer->write_lendian_t(config_size, abort);
            if (config_size > 0)
                writer->write(subelement_data.get_ptr(), config_size, abort);
        }
    }
    catch (std::exception&)
    {
    }
}

void CfgFlowinHost::WriteSubelement(ui_element_config::ptr data)
{
    ui_element_config_parser parser(data);
    subelement_data.from_stream(&parser.m_stream, parser.get_remaining(), fb2k::noAbort);
}

ui_element_config::ptr CfgFlowinHost::Subelement(unsigned /*id*/)
{
    return ui_element_config::g_create(subelement_guid, subelement_data.get_ptr(), subelement_data.get_size());
}

ui_element_config::ptr CfgFlowinHost::BuildConfiguration()
{
    // generate configuration that initailze ui element only
    ui_element_config_builder builder;
    builder.write_raw(&this->guid, sizeof(this->guid));
    return builder.finish(Flowin::Guids::dui_host_element);
}

GUID CfgFlowinHost::CfgGetGuid(ui_element_config::ptr data)
{
    // read host guid from ui_element_config
    PFC_ASSERT(data->get_guid() == Flowin::Guids::dui_host_element);
    GUID guid;
    ui_element_config_parser parser(data);
    parser.read_raw(&guid, sizeof(guid));
    return guid;
}

CfgFlowin::CfgFlowin() : cfg_var(g_flowin_config_guid)
{
    Reset();
}

void CfgFlowin::Reset()
{
    version = VersionCurrent;
}

void CfgFlowin::RegisterCallback(CfgFlowinCallback* cb)
{
    RETURN_VOID_IF(cb == nullptr);
    core_api::ensure_main_thread();
    if (auto it = std::find(callbacks.begin(), callbacks.end(), cb); it == callbacks.end())
        callbacks.push_back(cb);
}

void CfgFlowin::UnregisterCallback(CfgFlowinCallback* cb)
{
    RETURN_VOID_IF(cb == nullptr);
    core_api::ensure_main_thread();
    if (auto it = std::find(callbacks.begin(), callbacks.end(), cb); it != callbacks.end())
        callbacks.erase(it);
}

CfgFlowinHost::Ptr CfgFlowin::FindConfiguration(const GUID& host_guid)
{
    core_api::ensure_main_thread();
    auto& configs = host_config_list;
    auto it = std::find_if(configs.begin(), configs.end(), CfgFlowinHostComparator{host_guid});
    return it == configs.end() ? nullptr : *it;
}

CfgFlowinHost::Ptr CfgFlowin::AddOrFindConfiguration(const GUID& host_guid)
{
    core_api::ensure_main_thread();

    if (auto cfg = FindConfiguration(host_guid))
        return cfg;

    auto cfg = NewHostConfiguration();
    CoCreateGuid(&cfg->guid);
    host_config_list.push_back(cfg);
    return cfg;
}

void CfgFlowin::RemoveConfiguration(const GUID& host_guid)
{
    core_api::ensure_main_thread();
    auto& configs = host_config_list;
    auto it = std::find_if(configs.begin(), configs.end(), CfgFlowinHostComparator{host_guid});
    if (it != configs.end())
        configs.erase(it);
}

void CfgFlowin::get_data_raw(stream_writer* p_stream, abort_callback& p_abort)
{
    for (auto& cb : callbacks)
        cb->OnCfgPreWrite();

    try
    {
        uint32_t ver = VersionCurrent;
        p_stream->write_lendian_t(ver, p_abort);
        p_stream->write_object_t(show_debug_log, p_abort);
        /*
        * flowin host configuration layout
        | total number | size | data | size | data | ... |
        */
        uint32_t n, m = (uint32_t)host_config_list.size();
        // number
        p_stream->write_lendian_t(m, p_abort);
        for (n = 0; n < m; ++n)
        {
            auto cfg = host_config_list[n];
            // get data
            stream_writer_buffer_simple writer;
            cfg->GetDataRaw(&writer, p_abort);
            // size
            p_stream->write_lendian_t((uint32_t)writer.m_buffer.get_size(), p_abort);
            // data
            p_stream->write(writer.m_buffer.get_ptr(), writer.m_buffer.get_size(), p_abort);
        }
    }
    catch (...)
    {
    }
}

void CfgFlowin::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort)
{
    Reset();
    if (p_sizehint < sizeof(version))
        return;

    try
    {
        static_assert(VersionCurrent == Version012);
        p_stream->read_lendian_t(version, p_abort);
        switch (version)
        {
        case Version012:
        case Version011:
        case Version010: {
            p_stream->read_lendian_t(show_debug_log, p_abort);
            uint32_t n, m = 0;
            p_stream->read_lendian_t(m, p_abort);
            for (n = 0; n < m; ++n)
            {
                uint32_t data_size = 0;
                p_stream->read_lendian_t(data_size, p_abort);
                auto cfg = NewHostConfiguration();
                cfg->SetDataRaw(p_stream, data_size, p_abort);
                host_config_list.push_back(cfg);
            }
            break;
        }

        default:
            break;
        }
    }
    catch (...)
    {
        Reset();
    }
}
