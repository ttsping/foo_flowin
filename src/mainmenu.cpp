#include "pch.h"
#include "flowin_vars.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_menu_node.h"

namespace
{
class flowin_mainmenu_node_command : public mainmenu_node_command
{
public:
    explicit flowin_mainmenu_node_command(const flowin_menu_node::sp_t& node,
                                          const cfg_flowin_host::sp_t& config = nullptr)
        : node_(node), config_(config)
    {
        has_dynamic_config_ = (config_ == nullptr && is_config_reqired());
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        if (has_dynamic_config_)
            load_config();

        if (node_->id == t_menu_cmd_flowin_identify)
        {
            text = config_ ? config_->window_title : "Unknown";
            flags = mainmenu_commands::flag_disabled;
        }
        else
        {
            text = node_->text.c_str();
            flags = node_->get_flags(config_);
        }
    }

    GUID get_guid() override
    {
        static const GUID guid_dummy = {0x52101dd0, 0x15c2, 0x4371, {0xa4, 0x6f, 0x72, 0xd7, 0xe3, 0x3, 0x67, 0x60}};
        t_uint32 flags;
        pfc::string8 name;
        get_display(name, flags);
        if (has_dynamic_config_ || !is_config_reqired())
        {
            name += pfc::print_guid(guid_dummy);
        }
        else
        {
            load_config();
            name += pfc::print_guid(config_ ? config_->guid : guid_dummy);
        }

        return hasher_md5::get()->process_single_guid(name.get_ptr(), name.get_length());
    };

    void execute(service_ptr_t<service_base> callback) override
    {
        node_->action(config_);
    }

private:
    void load_config()
    {
        if (config_ != nullptr)
            return;

        const GUID active_guid = flowin_core::get()->get_latest_active_flowin();
        if (active_guid == pfc::guid_null)
            return;

        cfg_flowin::get()->enum_configuration_v2(
            [&](cfg_flowin_host::sp_t& config)
            {
                if (config->guid == active_guid)
                {
                    config_ = config;
                    return true; // stop
                }

                return false;
            });
    }

    bool is_config_reqired() const
    {
        switch (node_->id)
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
    cfg_flowin_host::sp_t config_;
    flowin_menu_node::sp_t node_;
    bool has_dynamic_config_ = false;
};

class flowin_mainmenu_node_group : public mainmenu_node_group
{
public:
    flowin_mainmenu_node_group()
    {
        menu_groups_ = build_flowin_menu_groups();
        for (auto& group : menu_groups_)
        {
            if (group->group == flowin_menu_group_root)
            {
                for (auto& node : group->nodes)
                {
                    menu_nodes_.push_back(fb2k::service_new<flowin_mainmenu_node_command>(node, group->config));
                }
            }
            else
            {
                menu_nodes_.push_back(fb2k::service_new<flowin_mainmenu_node_group>(group));
            }
        }
    }

    explicit flowin_mainmenu_node_group(flowin_menu_group::sp_t& group) : menu_groups_{group}
    {
        for (auto& node : group->nodes)
        {
            if (node->child_group != nullptr)
                menu_nodes_.push_back(fb2k::service_new<flowin_mainmenu_node_group>(node->child_group));
            else if (node->id == 0)
                menu_nodes_.push_back(fb2k::service_new<mainmenu_node_separator>());
            else
                menu_nodes_.push_back(fb2k::service_new<flowin_mainmenu_node_command>(node, group->config));
        }
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        flags = 0;
        if (menu_groups_.size() == 1)
        {
            auto& group = menu_groups_.front();
            if (group->text.empty())
            {
                switch (group->group)
                {
                case flowin_menu_group_active:
                    text = "Active";
                    return;

                case flowin_menu_group_live:
                    text = group->config ? group->config->window_title : "Unknown";
                    return;

                default:
                    break;
                }
            }
            else
            {
                text = group->text.c_str();
                return;
            }
        }
        // default
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
    flowin_menu_group_list menu_groups_;
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
        return false;
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
        return fb2k::service_new<flowin_mainmenu_node_group>();
    }

    bool dynamic_execute(t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback) override
    {
        return __super::dynamic_execute(index, subID, callback);
    }
};

static mainmenu_commands_factory_t<flowin_mainmenu> g_flowin_mainmenu_factory;

} // namespace