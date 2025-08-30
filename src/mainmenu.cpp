#include "pch.h"
#include "flowin_vars.h"
#include "flowin_core.h"
#include "flowin_config.h"

namespace
{

class flowin_config_menu_node_command : public mainmenu_node_command
{
public:
    flowin_config_menu_node_command(t_uint32 id, cfg_flowin_host::sp_t cfg = nullptr)
        : cmd_(id), config_(cfg), is_active_group_nodes_(false)
    {
        if (cfg == nullptr && is_require_config())
        {
            is_active_group_nodes_ = true;
        }
    }
    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        validate_config();
        const bool is_alive = config_ ? flowin_core::get()->is_flowin_alive(config_->guid) : false;
        const auto menu_set_check = [&](bool check = false)
        {
            if ((config_ == nullptr) && is_require_config())
            {
                flags = mainmenu_commands::flag_disabled;
                return;
            }
            flags |= (check ? mainmenu_commands::flag_checked : 0);
        };
        const auto menu_set_disable = [&](bool disable = false)
        {
            if ((config_ == nullptr) && is_require_config())
            {
                flags = mainmenu_commands::flag_disabled;
                return;
            }
            flags |= (disable ? mainmenu_commands::flag_disabled : 0);
        };

        uint32_t flowin_count = 0;
        cfg_flowin::get()->enum_configuration([&flowin_count](const cfg_flowin_host::sp_t&) { ++flowin_count; });

        flags = 0;
        switch (cmd_)
        {
        case t_menu_cmd_new_flowin:
            text = "New flowin";
            break;
        case t_menu_cmd_show_all:
            text = "Show all";
            menu_set_disable(flowin_count == 0);
            break;
        case t_menu_cmd_close_all:
            text = "Close all";
            menu_set_disable(flowin_count == 0);
            break;
        case t_menu_cmd_show_flowin:
            text = "Show";
            menu_set_check(is_alive);
            break;
        case t_menu_cmd_show_flowin_on_startup:
            text = "Show on startup";
            menu_set_check(config_ && config_->show_on_startup);
            break;
        case t_menu_cmd_always_on_top:
            text = "Always on top";
            menu_set_check(config_ && config_->always_on_top);
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_flowin_bring_to_top:
            text = "Bring to top";
            menu_set_check(false);
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_flowin_no_frame:
            text = "No window frame";
            menu_set_check(config_ && !config_->show_caption);
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_snap_to_edge:
            text = "Snap to screen edge";
            menu_set_check(config_ && config_->enable_snap);
            menu_set_disable(!is_alive || !config_ || !config_->show_caption);
            break;
        case t_menu_cmd_snap_auto_hide:
            text = "Auto hide when snapped";
            menu_set_check(config_ && config_->enable_autohide_when_snapped);
            menu_set_disable(!is_alive || !config_ || !config_->enable_snap || !config_->show_caption);
            break;
        case t_menu_cmd_flowin_reset_position:
            text = "Reset position";
            menu_set_check(false);
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_edit_mode:
            text = "Edit mode";
            menu_set_check(config_ && config_->edit_mode);
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_destroy_element:
            text = "Delete";
            menu_set_disable(!is_alive);
            break;
        case t_menu_cmd_flowin_identify:
            text = config_ ? config_->window_title : "Unknown";
            menu_set_disable(true);
            break;
        case t_menu_cmd_flowin_show_info:
            text = "Info";
            flags = mainmenu_commands::flag_defaulthidden;
            break;
        case t_menu_cmd_show_flowin_and_hide_main_window:
            text = "Show flowin and hide main window";
            flags = mainmenu_commands::flag_defaulthidden;
            menu_set_disable(is_alive);
            break;
        case t_menu_cmd_close_flowin_and_activate_main_window:
            text = "Close flowin and activate main window";
            flags = mainmenu_commands::flag_defaulthidden;
            menu_set_disable(!is_alive);
        default:
            break;
        }
    }
    void execute(service_ptr_t<service_base> callback) override
    {
        validate_config();
        switch (cmd_)
        {
        case t_menu_cmd_new_flowin:
            flowin_core::get()->create_flowin();
            break;

        case t_menu_cmd_show_all:
            cfg_flowin::get()->enum_configuration(
                [](cfg_flowin_host::sp_t config)
                {
                    if (!flowin_core::get()->is_flowin_alive(config->guid))
                    {
                        flowin_core::get()->create_flowin(config->guid);
                    }
                    else
                    {
                        flowin_core::get()->post_message(config->guid, UWM_FLOWIN_ACTIVE);
                    }
                });
            break;

        case t_menu_cmd_close_all:
            cfg_flowin::get()->enum_configuration([](cfg_flowin_host::sp_t config)
                                                  { flowin_core::get()->post_message(config->guid, WM_CLOSE); });
            break;

        case t_menu_cmd_show_flowin:
            if (config_ == nullptr)
            {
                break;
            }
            if (flowin_core::get()->is_flowin_alive(config_->guid))
            {
                flowin_core::get()->post_message(config_->guid, WM_CLOSE);
            }
            else
            {
                flowin_core::get()->create_flowin(config_->guid);
            }
            break;

        case t_menu_cmd_show_flowin_on_startup:
            if (config_ == nullptr)
            {
                break;
            }
            config_->show_on_startup = !config_->show_on_startup;
            break;

        case t_menu_cmd_always_on_top:
        case t_menu_cmd_flowin_no_frame:
        case t_menu_cmd_snap_to_edge:
        case t_menu_cmd_snap_auto_hide:
        case t_menu_cmd_edit_mode:
        case t_menu_cmd_destroy_element:
        case t_menu_cmd_flowin_reset_position:
        case t_menu_cmd_flowin_bring_to_top:
            if (config_ == nullptr)
            {
                break;
            }
            flowin_core::get()->post_message(config_->guid, UWM_FLOWIN_COMMAND, (WPARAM)cmd_);
            break;

        case t_menu_cmd_flowin_identify:
            break;

        case t_menu_cmd_flowin_show_info:
            if (config_)
            {
                pfc::string8 msg;
                msg << "guid: " << pfc::print_guid(config_->guid);
                popup_message_v2::g_show(core_api::get_main_window(), msg);
            }
            break;

        case t_menu_cmd_show_flowin_and_hide_main_window:
            if (config_ == nullptr)
            {
                break;
            }
            if (!flowin_core::get()->is_flowin_alive(config_->guid))
            {
                flowin_core::get()->create_flowin(config_->guid);
                ui_control::get()->hide();
            }
            break;

        case t_menu_cmd_close_flowin_and_activate_main_window:
            if (config_ == nullptr)
            {
                break;
            }
            if (flowin_core::get()->is_flowin_alive(config_->guid))
            {
                flowin_core::get()->post_message(config_->guid, WM_CLOSE);
                ui_control::get()->activate();
            }
            break;

        default:
            break;
        }
    };

    GUID get_guid() override
    {
        static const GUID guid_dummy = {0x52101dd0, 0x15c2, 0x4371, {0xa4, 0x6f, 0x72, 0xd7, 0xe3, 0x3, 0x67, 0x60}};
        t_uint32 flags;
        pfc::string8 name;
        get_display(name, flags);
        if (is_active_group_nodes_ || !is_require_config())
        {
            name += pfc::print_guid(guid_dummy);
        }
        else
        {
            validate_config();
            name += pfc::print_guid(config_ ? config_->guid : guid_dummy);
        }
        return hasher_md5::get()->process_single_guid(name.get_ptr(), name.get_length());
    };

    bool get_description(pfc::string_base& out) override
    {
        return false;
    }

private:
    void validate_config()
    {
        if (config_ != nullptr)
        {
            return;
        }

        GUID active_guid = flowin_core::get()->get_latest_active_flowin();
        if (active_guid == pfc::guid_null)
        {
            return;
        }
        cfg_flowin::get()->enum_configuration_v2(
            [&](cfg_flowin_host::sp_t config)
            {
                if (config->guid == active_guid)
                {
                    config_ = config;
                    return true;
                }
                return false;
            });
    }

    bool is_require_config()
    {
        switch (cmd_)
        {
        case t_menu_cmd_new_flowin:
        case t_menu_cmd_show_all:
        case t_menu_cmd_close_all:
            return false;
        default:
            break;
        }
        return true;
    }

private:
    const t_uint32 cmd_;
    cfg_flowin_host::sp_t config_;
    bool is_active_group_nodes_;
};

class active_flowin_node_group : public mainmenu_node_group
{
public:
    active_flowin_node_group()
    {
        t_uint32 menus_id[] = {
            t_menu_cmd_always_on_top, t_menu_cmd_flowin_bring_to_top, t_menu_cmd_flowin_no_frame,
            t_menu_cmd_snap_to_edge,  t_menu_cmd_snap_auto_hide,      t_menu_cmd_flowin_reset_position,
            t_menu_cmd_edit_mode,     t_menu_cmd_destroy_element,
        };
        menu_nodes_.push_back(fb2k::service_new<flowin_config_menu_node_command>(t_menu_cmd_flowin_identify));
        menu_nodes_.push_back(fb2k::service_new<mainmenu_node_separator>());
        for (t_size n = 0, m = pfc::array_size_t(menus_id); n < m; ++n)
        {
            auto node = fb2k::service_new<flowin_config_menu_node_command>(menus_id[n]);
            menu_nodes_.push_back(std::move(node));
        }
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        flags = 0;
        text = "Active";
    }

    t_size get_children_count() override
    {
        return menu_nodes_.size();
    }

    mainmenu_node::ptr get_child(t_size index) override
    {
        return menu_nodes_[index];
    }

private:
    std::vector<mainmenu_node::ptr> menu_nodes_;
};

class live_flowin_node_group : public mainmenu_node_group
{
public:
    live_flowin_node_group(cfg_flowin_host::sp_t cfg) : config_(cfg)
    {
        t_uint32 menus_id[] = {
            t_menu_cmd_show_flowin,
            t_menu_cmd_show_flowin_on_startup,
            t_menu_cmd_always_on_top,
            t_menu_cmd_flowin_bring_to_top,
            t_menu_cmd_flowin_no_frame,
            t_menu_cmd_snap_to_edge,
            t_menu_cmd_snap_auto_hide,
            t_menu_cmd_flowin_reset_position,
            t_menu_cmd_edit_mode,
            t_menu_cmd_flowin_show_info,
            t_menu_cmd_destroy_element,
            t_menu_cmd_show_flowin_and_hide_main_window,
            t_menu_cmd_close_flowin_and_activate_main_window,
        };
        for (t_size n = 0, m = pfc::array_size_t(menus_id); n < m; ++n)
        {
            auto node = fb2k::service_new<flowin_config_menu_node_command>(menus_id[n], cfg);
            menu_nodes_.push_back(std::move(node));
        }
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        flags = 0;
        text = config_->window_title;
    }

    t_size get_children_count() override
    {
        return menu_nodes_.size();
    }

    mainmenu_node::ptr get_child(t_size index) override
    {
        return menu_nodes_[index];
    }

private:
    cfg_flowin_host::sp_t config_;
    std::vector<mainmenu_node::ptr> menu_nodes_;
};

static t_uint32 fixed_menus_id[] = {t_menu_cmd_new_flowin, t_menu_cmd_show_all, t_menu_cmd_close_all};

class flowin_menu_group : public mainmenu_node_group
{
public:
    flowin_menu_group()
    {
        // fixed menus
        for (t_size n = 0, m = pfc::array_size_t(fixed_menus_id); n < m; ++n)
        {
            auto node = fb2k::service_new<flowin_config_menu_node_command>(fixed_menus_id[n]);
            menu_nodes_.push_back(std::move(node));
        }
        // separator
        menu_nodes_.push_back(fb2k::service_new<mainmenu_node_separator>());
        // active flowin
        menu_nodes_.push_back(fb2k::service_new<active_flowin_node_group>());
        cfg_flowin::get()->enum_configuration(
            [&](cfg_flowin_host::sp_t config)
            {
                auto node = fb2k::service_new<live_flowin_node_group>(config);
                menu_nodes_.push_back(std::move(node));
            });
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        flags = 0;
        text = "Flowin";
    }

    t_size get_children_count() override
    {
        return menu_nodes_.size();
    }

    mainmenu_node::ptr get_child(t_size index) override
    {
        return menu_nodes_[index];
    }

private:
    std::vector<mainmenu_node::ptr> menu_nodes_;
};

class flowin_mainmenu : public mainmenu_commands_v2
{
public:
    t_uint32 get_command_count() override
    {
        return 1;
    }
    GUID get_command(t_uint32 p_index) override
    {
        return g_flowin_mainmenu_group_guid;
    }
    void get_name(t_uint32 p_index, pfc::string_base& p_out) override
    {
        p_out = "Flowin Menu";
    }
    bool get_description(t_uint32 p_index, pfc::string_base& p_out) override
    {
        p_out = "Flowin menus.";
        return true;
    }
    GUID get_parent() override
    {
        return mainmenu_groups::view;
    }
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override
    {
    }
    bool is_command_dynamic(t_uint32 index) override
    {
        return true;
    }
    mainmenu_node::ptr dynamic_instantiate(t_uint32 index) override
    {
        return fb2k::service_new<flowin_menu_group>();
    }
    bool dynamic_execute(t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback) override
    {
        return __super::dynamic_execute(index, subID, callback);
    }
};

static mainmenu_commands_factory_t<flowin_mainmenu> g_flowin_mainmenu_factory;

} // namespace