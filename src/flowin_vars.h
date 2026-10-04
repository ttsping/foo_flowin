#pragma once

namespace Flowin
{
namespace Guids
{
constexpr GUID dui_dummy_element = {0xe149bb0a, 0xdba2, 0x41aa, {0xa5, 0x30, 0x28, 0x68, 0x41, 0xda, 0x5e, 0xd}};
constexpr GUID dui_host_element = {0xf3e1270d, 0xd824, 0x4756, {0xa4, 0x80, 0xdf, 0x6c, 0x4a, 0x26, 0x26, 0x5e}};
constexpr GUID main_menu_group = {0xca75bb5, 0xdb54, 0x43a1, {0x85, 0x1a, 0x47, 0xa8, 0x3e, 0x4d, 0xcf, 0x8e}};
constexpr GUID main_config = {0x546661da, 0x5967, 0x45d7, {0x83, 0x6e, 0x4e, 0xdd, 0x14, 0x2b, 0xb7, 0x2f}};
} // namespace Guids

enum class MenuCommands
{
    Invalid = 0,
    Separator = 0,
    NewFlowin = 0x1002,
    ShowAll,
    CloseAll,
    AlwaysOnTop,
    SnapToEdge,
    AutoHideWhenSnapped,
    SnapHide,
    SnapShow,
    EditMode,
    DestroyFlowin,
    Show,
    ShowOnStartup,
    CustomTitle,
    NoFrame,
    NoFrameSilent,
    ShowOnTaskbar,
    PseudoTransparent,
    Transparency,
    ResetPosition,
    BringToTop,
    ShowAndHideMainWindow,
    CloseAndActivateMainWindow,
    Identify,
    ShowInfo,
    AutoHideWhenHovered,
    ExportConfig,
    ImportConfig,
};

} // namespace Flowin

// {E149BB0A-DBA2-41AA-A530-286841DA5E0D}
static const GUID g_dui_dummy_element_guid = {
    0xe149bb0a, 0xdba2, 0x41aa, {0xa5, 0x30, 0x28, 0x68, 0x41, 0xda, 0x5e, 0xd}};

// {F3E1270D-D824-4756-A480-DF6C4A26265E}
static const GUID g_dui_flowin_host_guid = {
    0xf3e1270d, 0xd824, 0x4756, {0xa4, 0x80, 0xdf, 0x6c, 0x4a, 0x26, 0x26, 0x5e}};

// {0CA75BB5-DB54-43A1-851A-47A83E4DCF8E}
static const GUID g_flowin_mainmenu_group_guid = {
    0xca75bb5, 0xdb54, 0x43a1, {0x85, 0x1a, 0x47, 0xa8, 0x3e, 0x4d, 0xcf, 0x8e}};

// {546661DA-5967-45D7-836E-4EDD142BB72F}
static const GUID g_flowin_config_guid = {0x546661da, 0x5967, 0x45d7, {0x83, 0x6e, 0x4e, 0xdd, 0x14, 0x2b, 0xb7, 0x2f}};

enum FlowinUserMessage
{
    UWM_FLOWIN_COMMAND = WM_USER + 0x1002,
    UWM_FLOWIN_REFRESH_CONFIG,
    UWM_FLOWIN_ACTIVE,
    UWM_FLOWIN_UPDATE_TRANSPARENCY,
    UWM_FLOWIN_REPAINT,
    UWM_FLOWIN_COLOR_CHANGED,
    UWM_FLOWIN_CONTEXT_MENU,
};