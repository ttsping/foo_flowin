#include "pch.h"
#include "flowin_config.h"
#include "flowin_vars.h"
#include "flowin_callback.h"

cfg_flowin g_flowin_config;

cfg_flowin* cfg_flowin::get()
{
    return &g_flowin_config;
}

enum t_flowin_config_version
{
    t_version_010 = 1,
    t_version_011 = 3,
    t_version_current = t_version_011
};

void cfg_flowin_host::reset()
{
    version = t_version_current;
    show_on_startup = true;
    always_on_top = false;
    dock_to_taskbar = false;
    show_caption = true;
    show_maximize_box = false;
    show_minimize_box = false;
    snap_to_main_window = false;
    move_when_press_hot_key = true;
    enable_snap = false;
    enable_autohide_when_snapped = true;
    window_title = "Flowin";
    guid = pfc::guid_null;
    subelement_guid = pfc::guid_null;
    edit_mode = false;
    enable_transparency_active = false;
    transparency = 0;
    transparency_active = 0;
    ZeroMemory(&cfg_no_frame, sizeof(cfg_no_frame));
    cfg_no_frame.shadowed = true;
    cfg_no_frame.resizable = true;
    cfg_no_frame.draggable = true;

    ZeroMemory(&window_rect, sizeof(window_rect));
    ZeroMemory(&reserved, sizeof(reserved));
}

void cfg_flowin_host::set_data_raw(stream_reader* reader, t_size size, abort_callback& abort)
{
    reset();
    if (size < sizeof(version))
        return;

    try
    {
        t_size cfg_data_size = 0;
        reader->read_lendian_t(version, abort);
        switch (version)
        {
        case t_version_011:
            reader->read_object_t(enable_transparency_active, abort);
            reader->read_lendian_t(transparency, abort);
            reader->read_lendian_t(transparency_active, abort);
            reader->read_object(&cfg_no_frame, sizeof(cfg_no_frame), abort);
            reader->read_object_t(show_in_taskbar, abort);
            reader->read_object(bool_dummy, sizeof(bool_dummy), abort);
            reader->read_object(reserved, sizeof(reserved), abort);
            [[fallthrough]];
        case t_version_010:
            reader->read_object_t(guid, abort);
            reader->read_object_t(show_on_startup, abort);
            reader->read_object_t(always_on_top, abort);
            reader->read_object_t(dock_to_taskbar, abort);
            reader->read_object_t(show_caption, abort);
            reader->read_object_t(show_minimize_box, abort);
            reader->read_object_t(show_maximize_box, abort);
            reader->read_object_t(snap_to_main_window, abort);
            reader->read_object_t(move_when_press_hot_key, abort);
            reader->read_object_t(enable_snap, abort);
            reader->read_object_t(enable_autohide_when_snapped, abort);
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
        reset();
    }
}

void cfg_flowin_host::get_data_raw(stream_writer* writer, abort_callback& abort)
{
    try
    {
        uint32_t ver = t_version_current;
        writer->write_lendian_t(ver, abort);
        // version 011
        writer->write_object_t(enable_transparency_active, abort);
        writer->write_lendian_t(transparency, abort);
        writer->write_lendian_t(transparency_active, abort);
        writer->write_object(&cfg_no_frame, sizeof(cfg_no_frame), abort);
        writer->write_object_t(show_in_taskbar, abort);
        writer->write_object(bool_dummy, sizeof(bool_dummy), abort);
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
        writer->write_object_t(enable_snap, abort);
        writer->write_object_t(enable_autohide_when_snapped, abort);
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

void cfg_flowin_host::write_subelement(ui_element_config::ptr data)
{
    ui_element_config_parser parser(data);
    subelement_data.from_stream(&parser.m_stream, parser.get_remaining(), fb2k::noAbort);
}

ui_element_config::ptr cfg_flowin_host::subelement(unsigned /*id*/)
{
    return ui_element_config::g_create(subelement_guid, subelement_data.get_ptr(), subelement_data.get_size());
}

ui_element_config::ptr cfg_flowin_host::build_configuration()
{
    // generate configuration that initailze ui element only
    ui_element_config_builder builder;
    builder.write_raw(&this->guid, sizeof(this->guid));
    return builder.finish(flowin::guids::dui_host_element);
}

GUID cfg_flowin_host::cfg_get_guid(ui_element_config::ptr data)
{
    // read host guid from ui_element_config
    PFC_ASSERT(data->get_guid() == flowin::guids::dui_host_element);
    GUID guid;
    ui_element_config_parser parser(data);
    parser.read_raw(&guid, sizeof(guid));
    return guid;
}

cfg_flowin::cfg_flowin() : cfg_var(g_flowin_config_guid)
{
    reset();
}

void cfg_flowin::reset()
{
    version = t_version_current;
}

void cfg_flowin::register_callback(cfg_flowin_callback* cb)
{
    RETURN_VOID_IF(cb == nullptr);
    core_api::ensure_main_thread();
    if (auto it = std::find(callbacks_.begin(), callbacks_.end(), cb); it == callbacks_.end())
        callbacks_.push_back(cb);
}

void cfg_flowin::unregister_callback(cfg_flowin_callback* cb)
{
    RETURN_VOID_IF(cb == nullptr);
    core_api::ensure_main_thread();
    if (auto it = std::find(callbacks_.begin(), callbacks_.end(), cb); it != callbacks_.end())
        callbacks_.erase(it);
}

cfg_flowin_host::sp_t cfg_flowin::find_configuration(const GUID& host_guid)
{
    core_api::ensure_main_thread();
    auto& configs = host_config_list_;
    auto it = std::find_if(configs.begin(), configs.end(), cfg_flowin_host_comparator{host_guid});
    return it == configs.end() ? nullptr : *it;
}

cfg_flowin_host::sp_t cfg_flowin::add_or_find_configuration(const GUID& host_guid)
{
    core_api::ensure_main_thread();

    if (auto cfg = find_configuration(host_guid))
        return cfg;

    auto cfg = new_host_configuration();
    CoCreateGuid(&cfg->guid);
    host_config_list_.push_back(cfg);
    return cfg;
}

void cfg_flowin::remove_configuration(const GUID& host_guid)
{
    core_api::ensure_main_thread();
    auto& configs = host_config_list_;
    auto it = std::find_if(configs.begin(), configs.end(), cfg_flowin_host_comparator{host_guid});
    if (it != configs.end())
        configs.erase(it);
}

void cfg_flowin::get_data_raw(stream_writer* p_stream, abort_callback& p_abort)
{
    for (auto& cb : callbacks_)
        cb->on_cfg_pre_write();

    try
    {
        uint32_t ver = t_version_current;
        p_stream->write_lendian_t(ver, p_abort);
        p_stream->write_object_t(show_debug_log, p_abort);
        /*
        * flowin host configuration layout
        | total number | size | data | size | data | ... |
        */
        uint32_t n, m = (uint32_t)host_config_list_.size();
        // number
        p_stream->write_lendian_t(m, p_abort);
        for (n = 0; n < m; ++n)
        {
            auto cfg = host_config_list_[n];
            // get data
            stream_writer_buffer_simple writer;
            cfg->get_data_raw(&writer, p_abort);
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

void cfg_flowin::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort)
{
    reset();
    if (p_sizehint < sizeof(version))
        return;

    try
    {
        p_stream->read_lendian_t(version, p_abort);
        switch (version)
        {
        case t_version_011:
        case t_version_010: {
            p_stream->read_lendian_t(show_debug_log, p_abort);
            uint32_t n, m = 0;
            p_stream->read_lendian_t(m, p_abort);
            for (n = 0; n < m; ++n)
            {
                uint32_t data_size = 0;
                p_stream->read_lendian_t(data_size, p_abort);
                auto cfg = new_host_configuration();
                cfg->set_data_raw(p_stream, data_size, p_abort);
                host_config_list_.push_back(cfg);
            }
            break;
        }

        default:
            break;
        }
    }
    catch (...)
    {
        reset();
    }
}
