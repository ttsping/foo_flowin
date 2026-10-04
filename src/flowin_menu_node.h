#pragma once
#include <vector>
#include <memory>
#include "flowin_vars.h"

class CfgFlowinHost;

enum
{
    FlowinMenuGroupRoot = 0,
    FlowinMenuGroupActive,
    FlowinMenuGroupLive,
    FlowinMenuGroupSubmenu,
};

enum : uint32_t
{
    FlowinMenuShowDefault = 0,
    FlowinMenuShowOnRoot = 0,
    FlowinMenuShowOnActive = 1 << 1,
    FlowinMenuShowOnFlowin = 1 << 2,
    FlowinMenuShowOnSystemMenu = 1 << 3,
    FlowinMenuShowShiftOnly = 1 << 4,
    FlowinMenuShowOnMainMenu = FlowinMenuShowOnActive | FlowinMenuShowOnFlowin,
    FlowinMenuShowOnAll = FlowinMenuShowOnMainMenu | FlowinMenuShowOnSystemMenu,
};

static uint32_t FlowinMenuNodeDefaultGetFlags(const std::shared_ptr<CfgFlowinHost>&)
{
    return 0;
}

static void FlowinMenuNodeDefaultAction(std::shared_ptr<CfgFlowinHost>&)
{
}

struct FlowinMenuGroup;

struct FlowinMenuNode
{
    using Ptr = std::shared_ptr<FlowinMenuNode>;

    Flowin::MenuCommands id = Flowin::MenuCommands::Invalid;
    uint32_t show_flags = 0;
    std::string text;
    std::function<void(std::shared_ptr<CfgFlowinHost>&)> action = FlowinMenuNodeDefaultAction;
    std::function<uint32_t(const std::shared_ptr<CfgFlowinHost>&)> get_flags = FlowinMenuNodeDefaultGetFlags;
    std::shared_ptr<FlowinMenuGroup> child_group;

    explicit FlowinMenuNode(Flowin::MenuCommands node_id, std::string_view caption, uint32_t flags)
        : id(node_id), text(caption), show_flags(flags)
    {
    }
};
using FlowinMenuNodeList = std::vector<FlowinMenuNode::Ptr>;

struct FlowinMenuGroup
{
    using Ptr = std::shared_ptr<FlowinMenuGroup>;
    int32_t group = -1;
    std::string text;
    std::shared_ptr<CfgFlowinHost> config;
    FlowinMenuNodeList nodes;

    explicit FlowinMenuGroup(int32_t menu_group) : group(menu_group)
    {
    }

    explicit FlowinMenuGroup(int32_t menu_group, const std::string& caption) : group(menu_group), text(caption)
    {
    }

    static inline FlowinMenuGroup::Ptr NewGroup(uint32_t menu_group, const std::string& caption = {})
    {
        return std::make_shared<FlowinMenuGroup>(menu_group, caption);
    }

    inline FlowinMenuNode::Ptr NewNode(Flowin::MenuCommands id, std::string_view text,
                                           uint32_t flags = FlowinMenuShowDefault)
    {
        nodes.push_back(std::make_shared<FlowinMenuNode>(id, text, flags));
        return nodes.back();
    }
};
using FlowinMenuGroupList = std::vector<FlowinMenuGroup::Ptr>;

FlowinMenuGroup::Ptr BuildFlowinMenuNodes();
FlowinMenuGroupList BuildFlowinMenuGroups();