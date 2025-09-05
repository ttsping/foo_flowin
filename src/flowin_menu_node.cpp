#include "pch.h"
#include "flowin_menu_node.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_vars.h"

using cfg_t = cfg_flowin_host::sp_t;

inline bool is_flowin_alive(const cfg_t& config)
{
    return config && flowin_core::get()->is_flowin_alive(config->guid);
}

#define flags_disable(cond)                                                                                            \
    {                                                                                                                  \
        if (cond)                                                                                                      \
            flags |= mainmenu_commands::flag_disabled;                                                                 \
    }

#define flags_check(cond)                                                                                              \
    {                                                                                                                  \
        if (cond)                                                                                                      \
            flags |= mainmenu_commands::flag_checked;                                                                  \
    }

#define flags_require_config()                                                                                         \
    {                                                                                                                  \
        if (config == nullptr)                                                                                         \
            flags |= mainmenu_commands::flag_disabled;                                                                 \
    }

#define flags_default_hidden()                                                                                         \
    {                                                                                                                  \
        flags |= mainmenu_commands::flag_defaulthidden;                                                                \
    }

flowin_menu_group::sp_t build_flowin_menu_nodes()
{
    static flowin_menu_group::sp_t shared_nodes;
    if (shared_nodes != nullptr)
        return shared_nodes;

    if (auto group = flowin_menu_group::new_group(0 /*not used*/))
    {
        if (auto node = group->new_node(t_menu_cmd_show_flowin, "Show", flowin_menu_show_on_flowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = flowin_core::get();
                    if (core->is_flowin_alive(config->guid))
                        core->post_message(config->guid, WM_CLOSE);
                    else
                        core->create_flowin(config->guid);
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_show_flowin_on_startup, "Show on startup",
                                        flowin_menu_show_on_flowin | flowin_menu_show_on_system_menu))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                    config->show_on_startup = !config->show_on_startup;
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->show_on_startup);
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_always_on_top, "Always on top", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                {
                    if (is_flowin_alive(config))
                        flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
                    else
                        config->always_on_top = !config->always_on_top;
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->always_on_top);
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_flowin_bring_to_top, "Bring to top", flowin_menu_show_on_main_menu))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_flowin_no_frame, "No window frame", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                {
                    if (is_flowin_alive(config))
                        flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
                    else
                        config->show_caption = !config->show_caption;
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && !config->show_caption);
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_flowin_no_frame_silent, "No window frame (slient)",
                                        flowin_menu_show_on_main_menu))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id, (LPARAM)TRUE);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!is_flowin_alive(config));
                flags_default_hidden();
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_snap_to_edge, "Snap to screen edge", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->enable_snap);
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_snap_auto_hide, "Auto hide when snapped", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->enable_autohide_when_snapped);
                flags_disable(!config || !config->enable_snap || !is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node =
                group->new_node(t_menu_cmd_flowin_reset_position, "Reset position", flowin_menu_show_on_main_menu))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_edit_mode, "Edit mode", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->edit_mode);
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node =
                group->new_node(t_menu_cmd_flowin_custom_title, "Custom title", flowin_menu_show_on_system_menu))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };
        }

        if (auto node =
                group->new_node(t_menu_cmd_flowin_transparency, "Transparency", flowin_menu_show_on_system_menu))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };
        }

        if (auto node = group->new_node(t_menu_cmd_flowin_show_info, "Info", flowin_menu_show_on_flowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    pfc::string8 msg;
                    msg << "guid: " << pfc::print_guid(config->guid);
                    popup_message_v2::g_show(core_api::get_main_window(), msg);
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_destroy_element, "Delete", flowin_menu_show_on_all))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                    flowin_core::get()->post_message(config->guid, UWM_FLOWIN_COMMAND, (WPARAM)id);
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_show_flowin_and_hide_main_window, "Show flowin and hide main window",
                                        flowin_menu_show_on_flowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = flowin_core::get();
                    if (!core->is_flowin_alive(config->guid))
                        core->create_flowin(config->guid);
                    ui_control::get()->hide();
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                flags_disable(is_flowin_alive(config));
                return flags;
            };
        }

        if (auto node = group->new_node(t_menu_cmd_close_flowin_and_activate_main_window,
                                        "Close flowin and activate main window", flowin_menu_show_on_flowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = flowin_core::get();
                    if (core->is_flowin_alive(config->guid))
                        core->post_message(config->guid, WM_CLOSE);
                    ui_control::get()->activate();
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                flags_disable(!is_flowin_alive(config));
                return flags;
            };
        }

        shared_nodes = group;
    }

    return shared_nodes;
}

flowin_menu_group_list build_flowin_menu_groups()
{
    flowin_menu_group_list groups;
    // root
    if (auto root = flowin_menu_group::new_group(flowin_menu_group_root))
    {
        if (auto node = root->new_node(t_menu_cmd_new_flowin, "New flowin"))
        {
            node->action = [](cfg_t&) { flowin_core::get()->create_flowin(); };
        }

        if (auto node = root->new_node(t_menu_cmd_show_all, "Show all"))
        {
            node->action = [](cfg_t&)
            {
                configuration::for_each(
                    [](const cfg_flowin_host::sp_t& config)
                    {
                        auto core = flowin_core::get();
                        if (core->is_flowin_alive(config->guid))
                            core->post_message(config->guid, UWM_FLOWIN_ACTIVE);
                        else
                            core->create_flowin(config->guid);
                    });
            };

            node->get_flags = [](const cfg_t&)
            {
                uint32_t flags = 0;
                const size_t flowin_count = configuration::get_count();
                flags_disable(flowin_count == 0);
                return flags;
            };
        }

        if (auto node = root->new_node(t_menu_cmd_close_all, "Close all"))
        {
            node->action = [](cfg_t&)
            {
                configuration::for_each([](const cfg_flowin_host::sp_t& config)
                                        { flowin_core::get()->post_message(config->guid, WM_CLOSE); });
            };

            node->get_flags = [](const cfg_t&)
            {
                uint32_t flags = 0;
                const size_t flowin_count = configuration::get_count();
                flags_disable(flowin_count == 0);
                return flags;
            };

            groups.push_back(root);
        }

        if (auto& shared_group = build_flowin_menu_nodes())
        {
            auto& shared_nodes = shared_group->nodes;
            // active
            if (auto active = flowin_menu_group::new_group(flowin_menu_group_active))
            {
                // identify
                if (auto node = active->new_node(t_menu_cmd_flowin_identify, "", flowin_menu_show_on_active))
                {
                    node->action = [](cfg_t&) {};
                }
                // separator
                if (auto node = active->new_node(0, "", flowin_menu_show_on_active))
                {
                    node->action = [](cfg_t&) {};
                }
                // shared nodes
                for (auto& node : shared_nodes)
                {
                    if (node->show_flags & flowin_menu_show_on_active)
                        active->nodes.push_back(node);
                }

                groups.push_back(active);
            }
            // available flowins
            configuration::for_each(
                [&groups, &shared_nodes](cfg_flowin_host::sp_t& config)
                {
                    if (auto group = flowin_menu_group::new_group(flowin_menu_group_live))
                    {
                        group->config = config;
                        for (auto& node : shared_nodes)
                        {
                            if (node->show_flags & flowin_menu_show_on_flowin)
                                group->nodes.push_back(node);
                        }

                        groups.push_back(group);
                    }
                });
        }
    }

    return groups;
}
