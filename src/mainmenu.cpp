#include "pch.h"
#include "flowin_vars.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_menu_node.h"

namespace
{
using namespace Flowin;

constexpr uint32_t kMenuPathStride = 32;

class FlowinMainmenuNodeCommand : public mainmenu_node_command
{
public:
    explicit FlowinMainmenuNodeCommand(const FlowinMenuNode::Ptr& node,
                                          const CfgFlowinHost::Ptr& config = nullptr,
                                          uint32_t node_path = 0)
        : node(node), config(config), node_path(node_path)
    {
        has_dynamic_config = (config == nullptr && IsConfigRequired());
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        if (has_dynamic_config)
            LoadConfig();

        if (node->id == MenuCommands::Identify)
        {
            text = config ? config->window_title : "Unknown";
            flags = mainmenu_commands::flag_disabled;
        }
        else
        {
            text = node->text.c_str();
            flags = node->get_flags(config);
        }
    }

    GUID get_guid() override
    {
        static const GUID guid_dummy = {0x52101dd0, 0x15c2, 0x4371, {0xa4, 0x6f, 0x72, 0xd7, 0xe3, 0x3, 0x67, 0x60}};
        t_uint32 flags;
        pfc::string8 name;
        get_display(name, flags);

        GUID config_guid = guid_dummy;
        if (!has_dynamic_config && IsConfigRequired())
        {
            LoadConfig();
            if (config != nullptr)
                config_guid = config->guid;
        }

        stream_formatter_hasher_md5<> hasher;
        hasher << static_cast<t_uint32>(node_path);
        hasher << static_cast<t_uint32>(node->id);
        hasher << config_guid;

        return hasher.resultGuid();
    };

    void execute(service_ptr_t<service_base> callback) override
    {
        node->action(config);
    }

private:
    void LoadConfig()
    {
        if (config != nullptr)
            return;

        const GUID active_guid = FlowinCore::Get()->GetLatestActiveFlowin();
        if (active_guid == pfc::guid_null)
            return;

        CfgFlowin::Get()->EnumConfigurationV2(
            [&](CfgFlowinHost::Ptr& host_config)
            {
                if (host_config->guid == active_guid)
                {
                    config = host_config;
                    return true; // stop
                }

                return false;
            });
    }

    bool IsConfigRequired() const
    {
        switch (node->id)
        {
        case MenuCommands::NewFlowin:
        case MenuCommands::ShowAll:
        case MenuCommands::CloseAll:
            return false;

        default:
            break;
        }

        return true;
    }

private:
    CfgFlowinHost::Ptr config;
    FlowinMenuNode::Ptr node;
    uint32_t node_path = 0;
    bool has_dynamic_config = false;
};

class FlowinMainmenuNodeGroup : public mainmenu_node_group
{
public:
    FlowinMainmenuNodeGroup()
    {
        menu_groups = BuildFlowinMenuGroups();
        for (auto& group : menu_groups)
        {
            if (group->group == FlowinMenuGroupRoot)
            {
                for (auto& node : group->nodes)
                {
                    menu_nodes.push_back(fb2k::service_new<FlowinMainmenuNodeCommand>(node, group->config));
                }
            }
            else
            {
                menu_nodes.push_back(fb2k::service_new<FlowinMainmenuNodeGroup>(group));
            }
        }
    }

    explicit FlowinMainmenuNodeGroup(FlowinMenuGroup::Ptr& group, uint32_t node_path = 0) : menu_groups{group}
    {
        const uint32_t child_path = node_path * kMenuPathStride + static_cast<uint32_t>(group->group + 1);
        for (auto& node : group->nodes)
        {
            if (node->child_group != nullptr)
                menu_nodes.push_back(fb2k::service_new<FlowinMainmenuNodeGroup>(node->child_group, child_path));
            else if (node->id == MenuCommands::Invalid)
                menu_nodes.push_back(fb2k::service_new<mainmenu_node_separator>());
            else
                menu_nodes.push_back(fb2k::service_new<FlowinMainmenuNodeCommand>(node, group->config, node_path));
        }
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override
    {
        flags = 0;
        if (menu_groups.size() == 1)
        {
            auto& group = menu_groups.front();
            if (group->text.empty())
            {
                switch (group->group)
                {
                case FlowinMenuGroupActive:
                    text = "Active";
                    return;

                case FlowinMenuGroupLive:
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
        return menu_nodes.size();
    }

    mainmenu_node::ptr get_child(t_size index) override
    {
        return menu_nodes[index];
    }

private:
    std::vector<mainmenu_node::ptr> menu_nodes;
    FlowinMenuGroupList menu_groups;
};

class FlowinMainmenu : public mainmenu_commands_v2
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
        return fb2k::service_new<FlowinMainmenuNodeGroup>();
    }

    bool dynamic_execute(t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback) override
    {
        return __super::dynamic_execute(index, subID, callback);
    }
};

static mainmenu_commands_factory_t<FlowinMainmenu> g_flowin_mainmenu_factory;

} // namespace