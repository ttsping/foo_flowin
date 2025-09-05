#pragma once
#include <vector>

class cfg_flowin_host;

enum
{
    flowin_menu_group_root = 0,
    flowin_menu_group_active,
    flowin_menu_group_live,
};

enum : uint32_t
{
    flowin_menu_show_default = 0,
    flowin_menu_show_on_root = 0,
    flowin_menu_show_on_active = 1 << 1,
    flowin_menu_show_on_flowin = 1 << 2,
    flowin_menu_show_on_system_menu = 1 << 3,
    flowin_menu_show_on_main_menu = flowin_menu_show_on_active | flowin_menu_show_on_flowin,
    flowin_menu_show_on_all = flowin_menu_show_on_main_menu | flowin_menu_show_on_system_menu,
};

static uint32_t flowin_menu_node_default_get_flags(const std::shared_ptr<cfg_flowin_host>&)
{
    return 0;
}

struct flowin_menu_node
{
    using sp_t = std::shared_ptr<flowin_menu_node>;

    uint32_t id = 0;
    uint32_t show_flags = 0;
    std::string text;
    std::function<void(std::shared_ptr<cfg_flowin_host>&)> action;
    std::function<uint32_t(const std::shared_ptr<cfg_flowin_host>&)> get_flags = flowin_menu_node_default_get_flags;

    explicit flowin_menu_node(uint32_t node_id, std::string_view caption, uint32_t flags)
        : id(node_id), text(caption), show_flags(flags)
    {
    }
};
using flowin_menu_node_list = std::vector<flowin_menu_node::sp_t>;

struct flowin_menu_group
{
    using sp_t = std::shared_ptr<flowin_menu_group>;
    int32_t group = -1;
    std::shared_ptr<cfg_flowin_host> config;
    flowin_menu_node_list nodes;

    explicit flowin_menu_group(int32_t menu_group) : group(menu_group)
    {
    }

    static inline flowin_menu_group::sp_t new_group(uint32_t menu_group)
    {
        return std::make_shared<flowin_menu_group>(menu_group);
    }

    inline flowin_menu_node::sp_t new_node(uint32_t id, std::string_view text,
                                           uint32_t flags = flowin_menu_show_default)
    {
        nodes.push_back(std::make_shared<flowin_menu_node>(id, text, flags));
        return nodes.back();
    }
};
using flowin_menu_group_list = std::vector<flowin_menu_group::sp_t>;

flowin_menu_group::sp_t build_flowin_menu_nodes();
flowin_menu_group_list build_flowin_menu_groups();