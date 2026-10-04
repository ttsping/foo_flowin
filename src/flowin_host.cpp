#include "pch.h"
#include <shobjidl.h>
#include <comdef.h>
#include <dwmapi.h>
#include <mutex>
#include "helpers/ui_element_helpers.h"
#include "helpers/atl-misc.h"
#include "libPPUI/win32_utility.h"
#include "snap_window.h"
#include "flowin_vars.h"
#include "flowin_config.h"
#include "flowin_core.h"
#include "flowin_menu_node.h"
#include "flowin_utils.h"
#include "ui/ui_custom_title.h"
#include "ui/ui_transparency_settings.h"
#include "ui/ui_no_frame.h"
#include "resource.h"

#pragma comment(lib, "dwmapi.lib")

// COM smart pointer typedefs for file dialogs
_COM_SMARTPTR_TYPEDEF(IFileSaveDialog, __uuidof(IFileSaveDialog));
_COM_SMARTPTR_TYPEDEF(IFileOpenDialog, __uuidof(IFileOpenDialog));
_COM_SMARTPTR_TYPEDEF(IShellItem, __uuidof(IShellItem));

// Flowin config file header
#pragma pack(push, 1)
struct fwcfg_header_t
{
    uint32_t magic;     // Magic number: "FGCF" = 0x46474346
    uint32_t data_size; // Size of config data
    uint32_t data_crc;  // CRC32 of config data
};
#pragma pack(pop)

static constexpr uint32_t FWCFG_MAGIC = 0x46435746; // "FWCF" = Flowin Window Config File
static constexpr size_t FWCFG_HEADER_SIZE = sizeof(fwcfg_header_t);

using namespace Flowin;

// clang-format off
typedef CWinTraits<WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME, 0> CFlowinTraits;
// clang-format on

class FlowinHost : public ui_element_helpers::ui_element_instance_host_base,
                    public CWindowImpl<FlowinHost, CWindow, CFlowinTraits>,
                    public CSnapWindow<FlowinHost>,
                    public message_filter_impl_base
{
    class FlowinUiElementInstanceCallbackImpl : public ui_element_instance_callback_v3
    {
    public:
        FlowinUiElementInstanceCallbackImpl(class FlowinHost* host, ui_element_instance_callback_ptr callback)
            : host(host), callback(callback)
        {
        }

        void on_min_max_info_change() override
        {
        }

        void on_alt_pressed(bool p_state) override
        {
        }

        bool query_color(const GUID& p_what, t_ui_color& p_out) override
        {
            if (callback.is_valid())
                return callback->query_color(p_what, p_out);
            return false;
        }

        bool request_activation(service_ptr_t<class ui_element_instance> p_item) override
        {
            return true;
        }

        bool is_edit_mode_enabled() override
        {
            return host ? host->is_edit_mode_enabled() : false;
        }

        void request_replace(service_ptr_t<class ui_element_instance> p_item) override
        {
            if (callback.is_valid())
                callback->request_replace(p_item);
        }

        t_ui_font query_font_ex(const GUID& p_what) override
        {
            return callback.is_valid() ? callback->query_font_ex(p_what) : nullptr;
        }

        bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) override
        {
            return host ? host->host_is_child_visible(0) : false;
        }

        t_size notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2,
                      t_size param2size) override
        {
            return host ? host->host_notify(source, what, param1, param2, param2size) : 0;
        }

    private:
        FlowinHost* host;
        ui_element_instance_callback_ptr callback;
    };

public:
    DECLARE_WND_CLASS(TEXT("{EA622005-1140-4EF1-B64D-4A215DB3526A}"));

    BEGIN_MSG_MAP_EX(FlowinHost)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_ACTIVATE(OnActive)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_MBUTTONDOWN(OnMButtonDown)
        MSG_WM_MBUTTONUP(OnMButtonUp)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_INITMENUPOPUP(OnInitMenuPopup)
        MSG_WM_SYSCOMMAND(OnSysCommand)
        MSG_WM_SHOWWINDOW(OnShowWindow)
        MSG_WM_SIZE(OnSize)
        // MSG_WM_NCCALCSIZE(OnNcCalcSize)
        MSG_WM_NCHITTEST(OnNcHitTest)
        MSG_WM_NCACTIVATE(OnNcActive)
        MSG_WM_SETCURSOR(OnSetCursor)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_DESTROY(OnDestroy)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_COMMAND, OnFlowinCommand)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_REFRESH_CONFIG, OnRefreshConfig)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_ACTIVE, OnActiveFlowin)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_UPDATE_TRANSPARENCY, OnUpdateTransparency)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_REPAINT, OnRepaint)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_COLOR_CHANGED, OnUiColorChanged)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_CONTEXT_MENU, OnFlowinContextMenu)
        CHAIN_MSG_MAP(ui_element_instance_host_base)
        CHAIN_MSG_MAP(CSnapWindow<FlowinHost>)
    END_MSG_MAP()

    FlowinHost(ui_element_config::ptr p_config, ui_element_instance_callback_ptr p_callback)
        : ui_element_instance_host_base(p_callback), dummy_config(p_config), host_config(nullptr)
    {
        callback = new service_impl_t<FlowinUiElementInstanceCallbackImpl>(this, p_callback);
        set_configuration(p_config);
    }

    virtual ~FlowinHost()
    {
    }

    static GUID g_get_guid()
    {
        return g_dui_flowin_host_guid;
    }

    static GUID g_get_subclass()
    {
        return ui_element_subclass_utility;
    }

    static void g_get_name(pfc::string_base& out)
    {
        out = "Flowin";
    }

    static ui_element_config::ptr g_get_default_configuration()
    {
        return ui_element_config::g_create_empty(g_get_guid());
    }

    static const char* g_get_description()
    {
        return "";
    }

    const int32_t kWindowFrameX = ::GetSystemMetrics(SM_CXSIZEFRAME);
    const int32_t kWindowFrameY = ::GetSystemMetrics(SM_CYSIZEFRAME);

    bool HasChild() const
    {
        return element_inst.is_valid() && ::IsWindow(element_inst->get_wnd());
    }

    inline POINT GetBorderMetrics()
    {
        using namespace Utils;
        const auto dpi = static_cast<uint32_t>(QueryScreenDPIEx(*this).cx);
        const int32_t cx = GetSystemMetrics(SM_CXFRAME, dpi) + GetSystemMetrics(SM_CXPADDEDBORDER, dpi);
        const int32_t cy = GetSystemMetrics(SM_CYFRAME, dpi) + GetSystemMetrics(SM_CXPADDEDBORDER, dpi);
        return POINT{cx, cy};
    }

    inline CRect GetRectForNonSizing()
    {
        CRect rect;
        GetWindowRect(&rect);
        const auto border = GetBorderMetrics();
        rect.InflateRect(-border.x, -border.y);
        return rect;
    }

    bool pretranslate_message(MSG* p_msg) override
    {
        if (!host_config)
            return false;

        if (!IsChild(p_msg->hwnd))
            return false;

        bool forward_message = false;
        switch (p_msg->message)
        {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
            if (host_config->snap_to_edge || host_config->enable_transparency_active || host_config->auto_hide_when_hovered)
                forward_message = true;

            if (is_perform_drag)
                forward_message = true;
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (IsCfgNoFrame())
                forward_message = true;
            break;

        case WM_RBUTTONDOWN:
            interrupt_context_menu = false;

            if (!IsCfgNoFrame())
                break;

            if (auto modifiers = (uint32_t)p_msg->wParam; ((modifiers & (MK_CONTROL | MK_SHIFT)) != 0))
            {
                interrupt_context_menu = true;
                return true;
            }
            break;

        case WM_RBUTTONUP:
            if (interrupt_context_menu)
            {
                PostMessage(UWM_FLOWIN_CONTEXT_MENU);
                return true;
            }
            break;

        default:
            break;
        }

        if (forward_message)
            SendMessage(p_msg->message, p_msg->wParam, p_msg->lParam);

        switch (p_msg->message)
        {
        case WM_MOUSEMOVE:
            OnMouseMoveHook(p_msg);
            break;

        case WM_LBUTTONDOWN:
            OnLButtonDownHook(p_msg);
            break;

        default:
            break;
        }

        return false;
    }

    HWND get_wnd() override
    {
        return *this;
    }

    bool IsCfgNoFrame() const
    {
        return !host_config->show_caption;
    }

    auto& GetCfgNoFrame() const
    {
        return host_config->cfg_frameless;
    }

    bool UseLegacyNoFrame() const
    {
        static int32_t s_is_win11 = -1;
        if (s_is_win11 == -1)
        {
            // run once
            s_is_win11 = 0;
            // not future-proof
            typedef NTSTATUS(WINAPI * RtlGetVersionPtr)(LPOSVERSIONINFOEXW);
            if (RtlGetVersionPtr fun = (RtlGetVersionPtr)GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion"))
            {
                OSVERSIONINFOEXW ovi{};
                ovi.dwOSVersionInfoSize = sizeof(ovi);
                fun(&ovi);
                s_is_win11 = (ovi.dwMajorVersion >= 10 && ovi.dwBuildNumber >= 22000) ? 1 : 0;
            }
        }

        bool ret = s_is_win11 != 1 || !Utils::IsCompositionEnabled();
        if (host_config)
            host_config->cfg_frameless.legacy_no_frame = ret ? 1 : 0;

        return ret;
    }

    void ShowOrHideOnTaskbar(bool show)
    {
        _COM_SMARTPTR_TYPEDEF(ITaskbarList, __uuidof(ITaskbarList));

        try
        {
            CoInitializeScope scope;
            ITaskbarListPtr taskbar;
            if (SUCCEEDED(taskbar.CreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER)))
            {
                HWND wnd = get_wnd();
                std::ignore = show ? taskbar->AddTab(wnd) : taskbar->DeleteTab(wnd);
            }
        }
        catch (std::exception&)
        {
        }
    }

    bool IsActive() const
    {
        return GetActiveWindow() == m_hWnd;
    }

    void set_configuration(ui_element_config::ptr config) override
    {
        const auto guid = Configuration::GuidFromElementConfig(config);
        host_config = Configuration::AddOrFind(guid);
    }

    ui_element_config::ptr get_configuration() override
    {
        return dummy_config;
    }

    void host_replace_element(unsigned /*p_id*/, ui_element_config::ptr cfg) override
    {
        const GUID& new_guid = cfg->get_guid();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, new_guid))
        {
            element_inst = element->instantiate(*this, cfg, callback);
            if (!HasChild())
                return;
            // resize host window
            PostMessage(WM_SIZE);
            // refresh element config
            if (host_config->subelement_guid != new_guid)
            {
                host_config->subelement_guid = new_guid;
                element->get_name(host_config->window_title);
                uSetWindowText(*this, host_config->window_title);
            }

            host_config->WriteSubelement(cfg);
            // initial notify
            element_inst->notify(ui_element_notify_visibility_changed, (t_size)true, nullptr, 0);
        }
    }

    void host_replace_element(unsigned p_id, const GUID& p_newguid) override
    {
        element_inst.reset();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, p_newguid))
        {
            ui_element_config::ptr cfg;
            if (p_newguid == host_config->subelement_guid)
            {
                // restore child element
                cfg = host_config->Subelement(0);
            }
            else
            {
                cfg = element->get_default_configuration();
                if (element->get_subclass() == ui_element_subclass_containers)
                    host_config->edit_mode = true;
            }

            host_replace_element(p_id, cfg);
        }
    }

    ui_element_instance_ptr host_get_child(t_size /*which*/) override
    {
        return element_inst;
    }

    t_size host_get_children_count() override
    {
        return HasChild() ? 1 : 0;
    }

    void host_bring_to_front(t_size /*which*/) override
    {
        BringWindowToTop();
    }

    void host_replace_child(t_size which) override
    {
        callback->request_replace(host_get_child(which));
    }

    bool host_is_child_visible(t_size which) override
    {
        return HasChild() && ::IsWindowVisible(element_inst->get_wnd());
    }

    void initialize_window(HWND parent)
    {
        CSize dpi = QueryScreenDPIEx(*this);
        if (dpi.cx <= 0 || dpi.cy <= 0)
            dpi = CSize(USER_DEFAULT_SCREEN_DPI, USER_DEFAULT_SCREEN_DPI);

        const int32_t width = MulDiv(680, dpi.cx, USER_DEFAULT_SCREEN_DPI);
        const int32_t height = MulDiv(460, dpi.cy, USER_DEFAULT_SCREEN_DPI);
        WIN32_OP_D(Create(parent, CRect(0, 0, width, height)));
    }

    bool is_edit_mode_enabled()
    {
        return host_config->edit_mode;
    }

    t_size host_notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2,
                       t_size param2size)
    {
        return 0;
    }

private:
    void NotifyCommand(MenuCommands command)
    {
        PostMessage(UWM_FLOWIN_COMMAND, static_cast<uint32_t>(command));
    }

    bool IsTransparencyEnabled()
    {
        return (host_config->transparency > 0) ||
               (host_config->enable_transparency_active && host_config->transparency_active > 0);
    }

    void CalcIntermediateTransparency(int32_t target_transparency)
    {
        const int32_t delta = 12;
        if (transparency_intermediate < target_transparency)
        {
            transparency_intermediate = min(target_transparency, transparency_intermediate + delta);
        }
        else if (transparency_intermediate > target_transparency)
        {
            transparency_intermediate = max(target_transparency, transparency_intermediate - delta);
        }
    }

    void UpdateTransparency(int transparency = -1)
    {
        if (host_config->enable_transparency_active && host_config->transparency != host_config->transparency_active)
        {
            if (transparency_intermediate == -1)
            {
                transparency_intermediate =
                    IsActive() ? host_config->transparency : host_config->transparency_active;
            }
            else if (transparency_timer == NULL)
            {
                transparency_timer = SetTimer(kTransparencyTimerID, 20);
                return;
            }
        }

        if (transparency_timer && transparency == -1)
            return;

        PostMessage(UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)transparency);
    }

    void ApplyHoverHideAlpha(BYTE alpha)
    {
        ModifyStyleEx(0, WS_EX_LAYERED);
        SetLayeredWindowAttributes(*this, 0, alpha, LWA_ALPHA);
    }

    void StartHoverHideAnimation(bool hiding)
    {
        if (!hiding)
        {
            SimulateHoverHide(false);
        }

        is_hover_hiding = hiding;
        if (hover_hide_timer == NULL)
        {
            hover_hide_timer = SetTimer(kHoverHideTimerID, 20);
        }
    }

    void OnHoverMouseEnter()
    {
        if (!host_config->auto_hide_when_hovered)
            return;

        StartHoverHideAnimation(true);
    }

    void OnHoverMouseLeave()
    {
        if (!host_config->auto_hide_when_hovered)
            return;

        StartHoverHideAnimation(false);
    }

public:
    void SnapWindowOnHoverMouseEnter()
    {
        OnHoverMouseEnter();
    }

    void SnapWindowOnHoverMouseLeave()
    {
        if (!is_hover_hiding || hover_hide_alpha > 0)
        {
            OnHoverMouseLeave();
        }
    }

    bool SnapWindowAutoHideEnabled()
    {
        return host_config && host_config->auto_hide_when_snapped;
    }

    bool SnapWindowNeedMouseTracking()
    {
        return host_config && host_config->auto_hide_when_hovered;
    }

    void SetAlwaysOnTop(bool on_top)
    {
        SetWindowPos(on_top ? HWND_TOPMOST : HWND_NOTOPMOST, CRect{0}, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    void BringWindowToTop()
    {
        NotifyCommand(MenuCommands::BringToTop);
    }

    void ShowNoFrameShadow(bool show)
    {
        if (Utils::IsCompositionEnabled())
        {
            static const MARGINS extend_margins[2]{{0, 0, 0, 0}, {1, 1, 1, 1}};
            ::DwmExtendFrameIntoClientArea(get_wnd(), &extend_margins[show ? 1 : 0]);
        }
    }

    void EnableRoundedCorner(bool enable)
    {
        const DWORD policy = enable ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
        std::ignore = DwmSetWindowAttribute(get_wnd(), DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    }

    void InsertMenuEntry(HMENU menu, MenuCommands id, LPCWSTR caption, bool enabled = true, bool checked = false)
    {
        MENUITEMINFOW mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (ULONG_PTR)this;
        if (id != MenuCommands::Invalid)
        {
            mii.fMask |= MIIM_ID | MIIM_STRING | MIIM_STATE;
            mii.wID = static_cast<UINT>(id);
            mii.fState = (enabled ? MFS_ENABLED : MFS_DISABLED) | (checked ? MFS_CHECKED : MFS_UNCHECKED);
            mii.dwTypeData = const_cast<LPWSTR>(caption);
        }
        else
        {
            mii.fType = MFT_SEPARATOR;
        }

        InsertMenuItemW(menu, SC_CLOSE, FALSE, &mii);
    }

    void CleanupSystemMenu()
    {
        HMENU menu = GetSystemMenu(FALSE);
        do
        {
            int32_t n, m = GetMenuItemCount(menu);
            for (n = 0; n < m; ++n)
            {
                MENUITEMINFOW mii = {0};
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_DATA | MIIM_SUBMENU;
                GetMenuItemInfoW(menu, n, TRUE, &mii);
                if (mii.dwItemData == (ULONG_PTR)this)
                {
                    if (mii.hSubMenu)
                        DestroyMenu(mii.hSubMenu);
                    DeleteMenu(menu, n, MF_BYPOSITION);
                    break;
                }
            }

            if (n == m)
                break;
        } while (true);
    }

    void BuildContextMenu(HMENU menu, bool sys_menu = true)
    {
        // Check if Shift key is pressed
        const bool shift_pressed = IsKeyPressed(VK_SHIFT);

        if (menu_nodes.empty())
        {
            if (auto group_nodes = BuildFlowinMenuNodes())
            {
                for (auto& node : group_nodes->nodes)
                {
                    if (node->show_flags & FlowinMenuShowOnSystemMenu)
                        menu_nodes.push_back(node);
                }
            }
        }

        InsertContextMenuNodes(menu, menu_nodes, shift_pressed);

        if (sys_menu)
            InsertMenuEntry(menu, MenuCommands::Invalid, nullptr);
    }

    void InsertContextMenuNodes(HMENU menu, const FlowinMenuNodeList& nodes, bool shift_pressed)
    {
        for (auto& node : nodes)
        {
            if (!(node->show_flags & FlowinMenuShowOnSystemMenu))
                continue;

            // Skip shift-only menu items if shift is not pressed
            if ((node->show_flags & FlowinMenuShowShiftOnly) && !shift_pressed)
                continue;

            // A node with a child group is a submenu
            if (node->child_group != nullptr)
            {
                InsertContextSubmenu(menu, node, shift_pressed);
                continue;
            }

            pfc::stringcvt::string_wide_from_utf8 caption(node->text.c_str());
            const uint32_t flags = node->get_flags(host_config);
            const bool enabled = !(flags & mainmenu_commands::flag_disabled);
            const bool checked = flags & mainmenu_commands::flag_checked;
            InsertMenuEntry(menu, node->id, caption, enabled, checked);
        }
    }

    void InsertContextSubmenu(HMENU menu, const FlowinMenuNode::Ptr& node, bool shift_pressed)
    {
        HMENU sub_menu = CreatePopupMenu();
        if (sub_menu == nullptr)
            return;

        InsertContextMenuNodes(sub_menu, node->child_group->nodes, shift_pressed);

        // An empty submenu would only be a dead end, keep it out of the menu
        if (GetMenuItemCount(sub_menu) == 0)
        {
            DestroyMenu(sub_menu);
            return;
        }

        pfc::stringcvt::string_wide_from_utf8 caption(node->child_group->text.c_str());
        MENUITEMINFOW mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_STRING | MIIM_SUBMENU | MIIM_STATE;
        mii.dwItemData = (ULONG_PTR)this;
        mii.dwTypeData = const_cast<LPWSTR>(static_cast<LPCWSTR>(caption));
        mii.fState = MFS_ENABLED;
        mii.hSubMenu = sub_menu;
        InsertMenuItemW(menu, SC_CLOSE, FALSE, &mii);
    }

    void ExecuteContextMenu(MenuCommands cmd, int param = 0)
    {
        switch (cmd)
        {
        case MenuCommands::ShowOnStartup:
            host_config->show_on_startup = !host_config->show_on_startup;
            break;

        case MenuCommands::AlwaysOnTop:
            host_config->always_on_top = !host_config->always_on_top;
            SetAlwaysOnTop(host_config->always_on_top);
            break;

        case MenuCommands::NoFrame:
        case MenuCommands::NoFrameSilent:
            if (!host_config->show_caption)
            {
                host_config->show_caption = true;
                ConfigureWindowStyle();
            }
            else
            {
                bool apply_command = true;
                if ((cmd != MenuCommands::NoFrameSilent) && (param == 0))
                {
                    CNoFrameSettingsDialog dlg(host_config);
                    if (dlg.DoModal(*this) != IDOK)
                        apply_command = false;
                }

                if (apply_command)
                {
                    host_config->show_caption = false;
                    ConfigureWindowStyle();
                }
            }
            break;

        case MenuCommands::ShowOnTaskbar:
            host_config->show_in_taskbar = !host_config->show_in_taskbar;
            ShowOrHideOnTaskbar(host_config->show_in_taskbar);
            ConfigureWindowStyle();
            break;

        case MenuCommands::SnapToEdge:
            host_config->snap_to_edge = !host_config->snap_to_edge;
            enable_snap = host_config->snap_to_edge;
            if (!enable_snap)
                RestoreFromSnapHidden();
            break;

        case MenuCommands::AutoHideWhenSnapped:
            host_config->auto_hide_when_snapped = !host_config->auto_hide_when_snapped;
            if (!host_config->auto_hide_when_snapped)
                RestoreFromSnapHidden();
            break;

        case MenuCommands::EditMode:
            host_config->edit_mode = !host_config->edit_mode;
            if (HasChild())
                element_inst->notify(ui_element_notify_edit_mode_changed, 0, nullptr, 0);
            break;

        case MenuCommands::DestroyFlowin: {
            pfc::string8 element_name;
            uGetWindowText(*this, element_name);
            pfc::string_formatter msg;
            msg << " You are about to delete \"" << element_name
                << "\".\n This action cannot be undone.  Do you want to continue?";
            if (uMessageBox(*this, msg, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
                fb2k::inMainThread([this]() { FlowinCore::Get()->RemoveFlowin(this->host_config->guid, true); });
            break;
        }

        case MenuCommands::CustomTitle: {
            CCustomTitleDialog dlg(host_config->window_title);
            if (IDOK == dlg.DoModal(*this))
            {
                if (host_config->window_title.is_empty())
                {
                    if (HasChild())
                        ui_element::g_get_name(host_config->window_title, element_inst->get_guid());
                }

                ::uSetWindowText(*this, host_config->window_title);
            }

            break;
        }

        case MenuCommands::Transparency: {
            CTransparencySetDialog dlg(m_hWnd, host_config);
            dlg.DoModal(*this);
            UpdateTransparency();
            break;
        }

        case MenuCommands::ResetPosition: {
            CenterWindow(core_api::get_main_window());
            BringWindowToTop();
            break;
        }

        case MenuCommands::BringToTop: {
            RestoreFromSnapHidden();
            SetAlwaysOnTop(!host_config->always_on_top);
            SetAlwaysOnTop(host_config->always_on_top);
            break;
        }

        case MenuCommands::SnapHide:
            if (host_config->auto_hide_when_snapped)
                break;
            SimulateSnapToHide();
            break;

        case MenuCommands::SnapShow:
            if (host_config->auto_hide_when_snapped)
                break;
            SimulateSnapToShow();
            break;

        case MenuCommands::AutoHideWhenHovered:
            host_config->auto_hide_when_hovered = !host_config->auto_hide_when_hovered;
            if (host_config->auto_hide_when_hovered)
            {
                pfc::string8 window_title;
                uGetWindowText(*this, window_title);
                pfc::string_formatter msg;
                msg << "\"Auto-hide when hovered\" is now enabled for \"" << window_title << "\".\n\n"
                    << "Note: When the mouse enters this window, it will become invisible.\n"
                    << "You won't be able to interact with the panel until the mouse leaves.\n\n"
                    << "To disable: Main Menu -> View -> Flowin -> " << window_title << " -> Auto-hide when hovered";
                uMessageBox(*this, msg, "Auto-hide when hovered", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                KillTimer(kHoverHideTimerID);
                KillTimer(kHoverHideCheckTimerID);
                hover_hide_timer = NULL;
                is_hover_hiding = false;
                SimulateHoverHide(false, true);
            }
            break;

        case MenuCommands::ExportConfig:
            ExportConfigToFile();
            break;

        case MenuCommands::ImportConfig:
            ImportConfigFromFile();
            break;

        default:
            break;
        }
    }

    void AdjustRectToPrimaryMonitor(LPRECT rect, BOOL center = FALSE)
    {
        LONG ww = rect->right - rect->left;
        LONG wh = rect->bottom - rect->top;
        HMONITOR mon = MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST);
        WIN32_OP_D(mon != NULL);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        WIN32_OP_D(GetMonitorInfo(mon, &mi));
        const RECT& rc = mi.rcWork;
        if (center)
        {
            rect->left = rc.left + (rc.right - rc.left - ww) / 2;
            rect->top = rc.top + (rc.bottom - rc.top - wh) / 2;
            rect->right = rect->left + ww;
            rect->bottom = rect->top + wh;
        }
        else
        {
            rect->left = max(rc.left, min(rc.right - ww, rect->left));
            rect->top = max(rc.top, min(rc.bottom - wh, rect->top));
            rect->right = rect->left + ww;
            rect->bottom = rect->top + wh;
        }
    }

    void AdjustWindowPosition(LPRECT rect = nullptr, BOOL center = FALSE)
    {
        RECT rc_window = {};
        auto rc = rect ? rect : &rc_window;
        if (rect == nullptr)
            WIN32_OP_D(GetWindowRect(rc));
        AdjustRectToPrimaryMonitor(rc, center);
        WIN32_OP_D(SetWindowPos(nullptr, rc, SWP_NOZORDER | SWP_NOACTIVATE));
    }

    void AdjustMaximizedClientRect(LPRECT rect)
    {
        if (Utils::IsMaximized(get_wnd()))
        {
            HMONITOR mon = MonitorFromWindow(get_wnd(), MONITOR_DEFAULTTONEAREST);
            WIN32_OP_D(mon != NULL);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            WIN32_OP_D(GetMonitorInfo(mon, &mi));
            *rect = mi.rcWork;
        }
    }

    void ConfigureWindowStyle()
    {
        // frame
        const DWORD rel_style = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
        if (IsCfgNoFrame())
        {
            // window style
            if (UseLegacyNoFrame())
            {
                ModifyStyle(rel_style, 0);
            }
            else
            {
                // ModifyStyle(0, rel_style);
                ModifyStyle(rel_style, 0);
                ShowNoFrameShadow(GetCfgNoFrame().shadowed);
                EnableRoundedCorner(GetCfgNoFrame().rounded_corner);
            }
            // HACK
            // TODO snap in no frame mode not fully supported
            kSnapHideEdgeWidth = 2;
        }
        else
        {
            ModifyStyle(!host_config->show_in_taskbar ? (WS_MAXIMIZEBOX | WS_MINIMIZEBOX) : 0,
                        rel_style | (host_config->show_in_taskbar ? (WS_MAXIMIZEBOX | WS_MINIMIZEBOX) : 0));
            ShowNoFrameShadow(false);
            EnableRoundedCorner(true);
            kSnapHideEdgeWidth = 8;
        }

        // notify changes
        SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }

private:
    int OnCreate(LPCREATESTRUCT lpcs)
    {
        SetIcon(ui_control::get()->get_main_icon());

        FlowinCore::Get()->RegisterFlowin(m_hWnd, host_config->guid);

        ui_config_manager::ptr api;
        if (ui_config_manager::tryGet(api) && api->is_dark_mode())
        {
            dark_mode_hooks.AddDialog(m_hWnd);
            dark_mode_hooks.SetDark(true);
        }

        (VOID) GetSystemMenu(FALSE);
        (VOID) UseLegacyNoFrame();

        if (IsRectEmpty(&host_config->window_rect))
            CenterWindow(core_api::get_main_window());
        else
            AdjustWindowPosition(&host_config->window_rect);

        if (host_config->window_title.is_empty())
            g_get_name(host_config->window_title);

        ::uSetWindowText(*this, host_config->window_title);

        if (host_config->subelement_guid != pfc::guid_null)
            host_replace_element(0, host_config->subelement_guid);

        SetAlwaysOnTop(host_config->always_on_top);
        ConfigureWindowStyle();

        // snap config
        enable_snap = host_config->snap_to_edge;

        if (!host_config->show_in_taskbar)
            ShowOrHideOnTaskbar(false);

        if (IsTransparencyEnabled())
            OnUpdateTransparency(0, (WPARAM)host_config->transparency, 0);

        if (!host_config->always_on_top)
            BringWindowToTop();

        if (host_config->auto_hide_when_hovered)
        {
            POINT pt;
            GetCursorPos(&pt);
            RECT rect;
            GetWindowRect(&rect);

            if (PtInRect(&rect, pt))
            {
                SimulateHoverHide(true, true);
                is_hover_hiding = true;
                hover_hide_timer = SetTimer(kHoverHideCheckTimerID, 100);
            }
        }

        return TRUE;
    }

    void OnClose()
    {
        ShowWindow(SW_HIDE);
        SendMessage(UWM_FLOWIN_REFRESH_CONFIG);
        SetMsgHandled(FALSE);
    }

    void OnDestroy()
    {
        FlowinCore::Get()->RemoveFlowin(host_config->guid);
        host_config.reset();
        SetMsgHandled(FALSE);
    }

    void OnPaint(CDCHandle /*dc*/)
    {
        CPaintDC dc(*this);
        t_ui_color text_color;
        if (!callback->query_color(ui_color_text, text_color))
            text_color = GetSysColor(COLOR_BTNTEXT);

        dc.SetTextColor(text_color);
        dc.SetBkMode(TRANSPARENT);
        SelectObjectScope scope(dc, (HGDIOBJ)callback->query_font_ex(ui_font_default));
        CRect rc;
        GetClientRect(&rc);
        dc.DrawText(_T("Click to add new element."), -1, &rc, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    BOOL OnEraseBkgnd(CDCHandle dc)
    {
        CRect rc;
        GetClientRect(&rc);
        CBrush brush;
        t_ui_color background_color;
        if (!callback->query_color(ui_color_background, background_color))
            background_color = GetSysColor(COLOR_BTNFACE);

        brush.CreateSolidBrush(background_color);
        dc.FillRect(&rc, brush);
        return TRUE;
    }

    void OnActive(UINT state, BOOL /*minimized*/, CWindow /*wnd_other*/)
    {
        if (IsTransparencyEnabled())
        {
            static bool first_time_active = true;
            if (first_time_active)
            {
                PostMessage(UWM_FLOWIN_UPDATE_TRANSPARENCY, -1);
                first_time_active = false;
            }
            else
            {
                UpdateTransparency();
            }
        }

        FlowinCore::Get()->SetLatestActiveFlowin(host_config->guid);
    }

    void OnLButtonUp(UINT flags, CPoint point)
    {
        if (!HasChild())
        {
            replace_dialog(*this, 0, pfc::guid_null);
        }
    }

    void OnMButtonDown(UINT flags, CPoint point)
    {
        GetCursorPos(&drag_point);
        if (IsCfgNoFrame() && GetCfgNoFrame().draggable)
        {
            SetCapture();
            SendMessage(WM_ENTERSIZEMOVE);
            is_perform_drag = true;
        }
    }

    void OnMButtonUp(UINT flags, CPoint point)
    {
        if (is_perform_drag)
        {
            ReleaseCapture();
            SendMessage(WM_EXITSIZEMOVE);
            is_perform_drag = false;
        }
    }

    void OnMouseMove(UINT flags, CPoint point)
    {
        if ((flags & MK_MBUTTON) && is_perform_drag)
        {
            POINT pt{};
            GetCursorPos(&pt);
            RECT rect;
            WIN32_OP_D(GetWindowRect(&rect));
            OffsetRect(&rect, pt.x - drag_point.x, pt.y - drag_point.y);
            SendMessage(WM_MOVING, 0, (LPARAM)&rect);
            MoveWindow(&rect);
            GetCursorPos(&pt);
            drag_point = pt;
            return;
        }

        SetMsgHandled(FALSE);
    }

    void OnMouseMoveHook(LPMSG msg)
    {
        if (!IsCfgNoFrame())
            return;

        GUITHREADINFO thread_info = {};
        thread_info.cbSize = sizeof(thread_info);
        if (GetGUIThreadInfo(GetCurrentThreadId(), &thread_info))
        {
            if (thread_info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE))
                return;

            const DWORD msg_pos = GetMessagePos();
            const POINT pt = {GET_X_LPARAM(msg_pos), GET_Y_LPARAM(msg_pos)};
            const CRect rect_for_non_sizing = GetRectForNonSizing();
            if (rect_for_non_sizing.PtInRect(pt))
                return;

            const int32_t hittest = (int32_t)SendMessage(WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
            if (hittest != HTCLIENT)
            {
                SendMessage(WM_SETCURSOR, (WPARAM)get_wnd(), MAKELPARAM(hittest, WM_MOUSEMOVE));
                msg->message = WM_NULL;
            }
        }
    }

    void OnLButtonDownHook(LPMSG msg)
    {
        if (!IsCfgNoFrame())
            return;

        auto HitTestToWMSZ = [](int32_t hittest) -> int32_t
        {
            switch (hittest)
            {
            case HTLEFT:
                return WMSZ_LEFT;
            case HTTOP:
                return WMSZ_TOP;
            case HTRIGHT:
                return WMSZ_RIGHT;
            case HTBOTTOM:
                return WMSZ_BOTTOM;
            case HTTOPLEFT:
                return WMSZ_TOPLEFT;
            case HTTOPRIGHT:
                return WMSZ_TOPRIGHT;
            case HTBOTTOMLEFT:
                return WMSZ_BOTTOMLEFT;
            case HTBOTTOMRIGHT:
                return WMSZ_BOTTOMRIGHT;
            default:
                break;
            }
            return 0;
        };

        GUITHREADINFO threadInfo = {};
        threadInfo.cbSize = sizeof(threadInfo);
        if (GetGUIThreadInfo(GetCurrentThreadId(), &threadInfo))
        {
            if (threadInfo.flags & (GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE))
                return;

            const DWORD messagePos = GetMessagePos();
            const POINT pt = {GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos)};

            {
                // simulate resizing
                const CRect rect_for_non_sizing = GetRectForNonSizing();
                if (!rect_for_non_sizing.PtInRect(pt))
                {
                    if (threadInfo.flags & (GUI_INMOVESIZE))
                        return;

                    const int32_t hittest = (int32_t)SendMessage(WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
                    if (hittest != HTCLIENT)
                    {
                        SendMessage(WM_SETCURSOR, (WPARAM)get_wnd(), MAKELPARAM(hittest, WM_MOUSEMOVE));
                        SendMessage(WM_SYSCOMMAND, SC_SIZE | HitTestToWMSZ(hittest), MAKELPARAM(pt.x, pt.y));
                        msg->message = WM_NULL;
                        return;
                    }
                }
            }
        }
    }

    void OnContextMenu(CWindow wnd, CPoint point)
    {
        RECT rect_client;
        GetClientRect(&rect_client);
        ClientToScreen(&rect_client);
        SetMsgHandled(FALSE);
        if (!PtInRect(&rect_client, point))
            return;

        if (!callback->is_edit_mode_enabled() && HasChild())
            return;

        auto inst = HasChild()
                        ? element_inst
                        : ui_element_helpers::instantiate_dummy(*this, ui_element_config::g_create_empty(), callback);
        standard_edit_context_menu(MAKELPARAM(point.x, point.y), inst, 0, *this);
        SetMsgHandled(TRUE);
    }

    void OnInitMenuPopup(CMenuHandle menu, UINT idx, BOOL sys_menu)
    {
        if (sys_menu && menu.m_hMenu == GetSystemMenu(FALSE))
        {
            CleanupSystemMenu();
            BuildContextMenu(menu);
        }
    }

    void OnSysCommand(UINT id, CPoint /*point*/)
    {
        ExecuteContextMenu(static_cast<MenuCommands>(id));
        SetMsgHandled(FALSE);
    }

    void OnShowWindow(BOOL show, UINT /*status*/)
    {
        if (HasChild())
        {
            element_inst->notify(ui_element_notify_visibility_changed, (t_size) !!show, nullptr, 0);
        }
    }

    void OnSize(UINT type, CSize size)
    {
        if (HasChild())
        {
            CRect rc;
            GetClientRect(&rc);
            ::SetWindowPos(element_inst->get_wnd(), HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(),
                           SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
        }
    }

    LRESULT OnNcCalcSize(BOOL calc_valid_rect, LPARAM param)
    {
        if (!IsCfgNoFrame())
        {
            SetMsgHandled(FALSE);
        }
        else if (!calc_valid_rect || UseLegacyNoFrame())
        {
            SetMsgHandled(FALSE);
        }
        else
        {
            LPNCCALCSIZE_PARAMS lpnccs_params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(param);
            AdjustMaximizedClientRect(lpnccs_params->rgrc);
        }

        return 0;
    }

    UINT OnNcHitTest(CPoint point)
    {
        if (!IsCfgNoFrame())
        {
            SetMsgHandled(FALSE);
            return 0;
        }

        UINT res = HTCLIENT;
        if (!GetCfgNoFrame().resizable)
            return res;

        RECT rect{};
        WIN32_OP_D(GetWindowRect(&rect));
        const auto border = GetBorderMetrics();

        enum EdgeMask
        {
            Left = 0b0001,
            Right = 0b0010,
            Top = 0b0100,
            Bottom = 0b1000,
        };

        const auto result = Left * (point.x < (rect.left + border.x)) | Right * (point.x >= (rect.right - border.x)) |
                            Top * (point.y < (rect.top + border.y)) | Bottom * (point.y >= (rect.bottom - border.y));
        switch (result)
        {
        case Left:
            return HTLEFT;
        case Right:
            return HTRIGHT;
        case Top:
            return HTTOP;
        case Bottom:
            return HTBOTTOM;
        case Top | Left:
            return HTTOPLEFT;
        case Top | Right:
            return HTTOPRIGHT;
        case Bottom | Left:
            return HTBOTTOMLEFT;
        case Bottom | Right:
            return HTBOTTOMRIGHT;
        default:
            break;
        }

        return res;
    }

    BOOL OnNcActive(BOOL active)
    {
        if (IsCfgNoFrame() && !Utils::IsCompositionEnabled())
            return TRUE;

        SetMsgHandled(FALSE);
        return FALSE;
    }

    BOOL OnSetCursor(CWindow /*wnd*/, UINT hittest, UINT message)
    {
        if (!IsCfgNoFrame() /* || !UseLegacyNoFrame()*/)
        {
            SetMsgHandled(FALSE);
            return FALSE;
        }

        if (hittest == HTCLIENT)
        {
            SetMsgHandled(FALSE);
            return FALSE;
        }

        if (hittest == HTTOP || hittest == HTBOTTOM)
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        else if (hittest == HTLEFT || hittest == HTRIGHT)
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        else if (hittest == HTTOPLEFT || hittest == HTBOTTOMRIGHT)
            SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
        else if (hittest == HTTOPRIGHT || hittest == HTBOTTOMLEFT)
            SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
        else
            SetCursor(LoadCursor(nullptr, IDC_ARROW));

        return TRUE;
    }

    void OnTimer(UINT_PTR id)
    {
        if (id == kTransparencyTimerID)
        {
            uint32_t target_transparency = IsActive() ? host_config->transparency_active : host_config->transparency;
            CalcIntermediateTransparency(target_transparency);
            UpdateTransparency(transparency_intermediate);
            if (transparency_intermediate == target_transparency)
            {
                KillTimer(id);
                transparency_timer = NULL;
            }

            return;
        }

        if (id == kHoverHideTimerID)
        {
            const int32_t max_alpha = (int32_t)(255.0 - host_config->transparency * 255.0 / 100);
            const int32_t delta = 51;
            const int32_t target_alpha = is_hover_hiding ? 0 : max_alpha;

            if (hover_hide_alpha < target_alpha)
            {
                hover_hide_alpha = min(target_alpha, hover_hide_alpha + delta);
            }
            else if (hover_hide_alpha > target_alpha)
            {
                hover_hide_alpha = max(target_alpha, hover_hide_alpha - delta);
            }

            ApplyHoverHideAlpha((BYTE)hover_hide_alpha);

            if (hover_hide_alpha == target_alpha)
            {
                if (is_hover_hiding && hover_hide_alpha == 0)
                {
                    SimulateHoverHide(true);
                    KillTimer(kHoverHideTimerID);
                    hover_hide_timer = SetTimer(kHoverHideCheckTimerID, 100);
                }
                else
                {
                    KillTimer(id);
                    hover_hide_timer = NULL;
                }
            }

            return;
        }

        if (id == kHoverHideCheckTimerID)
        {
            if (is_hover_hiding && hover_hide_alpha == 0)
            {
                POINT pt;
                GetCursorPos(&pt);
                RECT rect;
                GetWindowRect(&rect);

                if (!PtInRect(&rect, pt))
                {
                    KillTimer(id);
                    hover_hide_timer = NULL;
                    StartHoverHideAnimation(false);
                }
            }
            else
            {
                KillTimer(id);
                hover_hide_timer = NULL;
            }

            return;
        }

        SetMsgHandled(FALSE);
    }

    LRESULT OnFlowinCommand(UINT /*msg*/, WPARAM wp, LPARAM lp)
    {
        ExecuteContextMenu((MenuCommands)wp, (int)lp);
        return TRUE;
    }

    LRESULT OnRefreshConfig(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        GetSnapWindowRect(&host_config->window_rect);
        if (HasChild())
            host_config->WriteSubelement(element_inst->get_configuration());

        return TRUE;
    }

    LRESULT OnActiveFlowin(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        BringWindowToTop();
        return TRUE;
    }

    LRESULT OnUpdateTransparency(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/)
    {
        ModifyStyleEx(0, WS_EX_LAYERED);
        int transparency = (int)wp;
        BYTE alpha = 0;
        if (transparency >= 0)
            alpha = (BYTE)(255.0 - transparency * 255.0 / 100);
        else if (host_config->enable_transparency_active && GetActiveWindow() == m_hWnd)
            alpha = (BYTE)(255.0 - host_config->transparency_active * 255.0 / 100);
        else
            alpha = (BYTE)(255.0 - host_config->transparency * 255.0 / 100);

        SetLayeredWindowAttributes(*this, 0, alpha, LWA_ALPHA);
        return TRUE;
    }

    LRESULT OnRepaint(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        InvalidateRect(nullptr);
        return 0;
    }

    LRESULT OnUiColorChanged(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        ui_config_manager::ptr api;
        if (ui_config_manager::tryGet(api))
            dark_mode_hooks.SetDark(api->is_dark_mode());

        return 0;
    }

    LRESULT OnFlowinContextMenu(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        if (HMENU menu = CreatePopupMenu())
        {
            POINT pt = {};
            GetCursorPos(&pt);
            BuildContextMenu(menu, false);
            const int32_t cmd = TrackPopupMenu(menu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
            ExecuteContextMenu(static_cast<Flowin::MenuCommands>(cmd));
        }
        return 0;
    }

    void SimulateHoverHide(bool hide, bool overrride_alpha = false)
    {
        if (hide)
        {
            if (overrride_alpha)
            {
                hover_hide_alpha = 0;
                ApplyHoverHideAlpha(0);
            }

            ModifyStyleEx(0, WS_EX_TRANSPARENT);
            if (host_config->show_in_taskbar)
                ShowOrHideOnTaskbar(false);
        }
        else
        {
            if (overrride_alpha)
            {
                const BYTE max_alpha = (BYTE)(255.0 - host_config->transparency * 255.0 / 100);
                hover_hide_alpha = max_alpha;
                ApplyHoverHideAlpha(max_alpha);
            }

            ModifyStyleEx(WS_EX_TRANSPARENT, 0);
            if (host_config->show_in_taskbar)
                ShowOrHideOnTaskbar(true);
        }
    }

    // Remove invalid characters from filename
    static pfc::string8 SanitizeFilename(const char* filename)
    {
        pfc::string8 result;
        const char* invalid_chars = "\\/:*?\"<>|";
        for (t_size i = 0; filename[i] != '\0'; ++i)
        {
            bool is_invalid = false;
            for (int j = 0; invalid_chars[j] != '\0'; ++j)
            {
                if (filename[i] == invalid_chars[j])
                {
                    is_invalid = true;
                    break;
                }
            }
            if (!is_invalid)
                result.add_char(filename[i]);
        }
        return result;
    }

    void ExportConfigToFile()
    {
        // Get default filename from window title
        pfc::string8 default_name = SanitizeFilename(host_config->window_title);
        if (default_name.is_empty())
            default_name = "flowin";

        pfc::stringcvt::string_wide_from_utf8 default_name_wide(default_name);

        // Create file save dialog using COM
        IFileSaveDialogPtr file_dialog;
        if (SUCCEEDED(file_dialog.CreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER)))
        {
            // Set file types
            COMDLG_FILTERSPEC file_types[] = {{L"Flowin Config Files", L"*.fwcfg"}, {L"All Files", L"*.*"}};
            file_dialog->SetFileTypes(ARRAYSIZE(file_types), file_types);
            file_dialog->SetDefaultExtension(L"fwcfg");
            file_dialog->SetFileName(default_name_wide);
            file_dialog->SetOptions(FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);

            if (SUCCEEDED(file_dialog->Show(*this)))
            {
                IShellItemPtr result_item;
                if (SUCCEEDED(file_dialog->GetResult(&result_item)))
                {
                    LPWSTR file_path = nullptr;
                    if (SUCCEEDED(result_item->GetDisplayName(SIGDN_FILESYSPATH, &file_path)))
                    {
                        try
                        {
                            // Refresh config
                            if (IsWindow())
                                SendMessage(UWM_FLOWIN_REFRESH_CONFIG);

                            // Serialize config data
                            stream_writer_buffer_simple writer;
                            host_config->GetDataRaw(&writer, fb2k::noAbort);

                            // Write to file
                            HANDLE file_handle = CreateFileW(file_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                                             FILE_ATTRIBUTE_NORMAL, nullptr);
                            if (file_handle != INVALID_HANDLE_VALUE)
                            {
                                // Build header
                                fwcfg_header_t header;
                                header.magic = FWCFG_MAGIC;
                                header.data_size = (uint32_t)writer.m_buffer.get_size();
                                header.data_crc = Utils::CalculateCrc32(writer.m_buffer.get_ptr(), writer.m_buffer.get_size());

                                // Write header
                                DWORD bytes_written = 0;
                                BOOL rc = WriteFile(file_handle, &header, FWCFG_HEADER_SIZE, &bytes_written, nullptr);
                                DWORD err = GetLastError();

                                // Write data
                                if (rc && bytes_written == FWCFG_HEADER_SIZE)
                                {
                                    rc = WriteFile(file_handle, writer.m_buffer.get_ptr(),
                                                   (DWORD)writer.m_buffer.get_size(), &bytes_written, nullptr);
                                    if (!rc)
                                        err = GetLastError();
                                }

                                CloseHandle(file_handle);

                                if (!rc || bytes_written != writer.m_buffer.get_size())
                                {
                                    pfc::string8_fast win_err;
                                    pfc::string8_fast msg;
                                    msg << "Failed to write configuration file.";
                                    if (pfc::winFormatSystemErrorMessage(win_err, err))
                                        msg << "\r\nError: " << win_err;
                                    uMessageBox(*this, msg, "Export Error", MB_OK | MB_ICONERROR);
                                }
                            }
                            else
                            {
                                uMessageBox(*this, "Failed to save configuration file.", "Export Error",
                                            MB_OK | MB_ICONERROR);
                            }
                        }
                        catch (std::exception&)
                        {
                            uMessageBox(*this, "Failed to export configuration.", "Export Error", MB_OK | MB_ICONERROR);
                        }

                        CoTaskMemFree(file_path);
                    }
                }
            }
        }
    }

    void ImportConfigFromFile()
    {
        // Confirm before import
        if (uMessageBox(*this,
                        "Importing configuration will replace the current flowin window.\n\nDo you want to continue?",
                        "Import Configuration", MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return;
        }

        // Create file open dialog using COM
        IFileOpenDialogPtr file_dialog;
        if (SUCCEEDED(file_dialog.CreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER)))
        {
            // Set file types
            COMDLG_FILTERSPEC file_types[] = {{L"Flowin Config Files", L"*.fwcfg"}, {L"All Files", L"*.*"}};
            file_dialog->SetFileTypes(ARRAYSIZE(file_types), file_types);
            file_dialog->SetDefaultExtension(L"fwcfg");
            file_dialog->SetOptions(FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);

            if (SUCCEEDED(file_dialog->Show(*this)))
            {
                IShellItemPtr result_item;
                if (SUCCEEDED(file_dialog->GetResult(&result_item)))
                {
                    LPWSTR file_path = nullptr;
                    if (SUCCEEDED(result_item->GetDisplayName(SIGDN_FILESYSPATH, &file_path)))
                    {
                        try
                        {
                            // Read file content
                            HANDLE file_handle = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                            if (file_handle == INVALID_HANDLE_VALUE)
                            {
                                uMessageBox(*this, "Failed to open configuration file.", "Import Error",
                                            MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }

                            DWORD file_size = GetFileSize(file_handle, nullptr);
                            if (file_size == INVALID_FILE_SIZE || file_size < FWCFG_HEADER_SIZE)
                            {
                                CloseHandle(file_handle);
                                uMessageBox(*this, "Invalid configuration file: file too small.", "Import Error", MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }

                            // Read entire file
                            pfc::array_t<uint8_t> buffer;
                            buffer.set_size(file_size);
                            DWORD bytes_read = 0;
                            if (!ReadFile(file_handle, buffer.get_ptr(), file_size, &bytes_read, nullptr) ||
                                bytes_read != file_size)
                            {
                                CloseHandle(file_handle);
                                uMessageBox(*this, "Failed to read configuration file.", "Import Error",
                                            MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }
                            CloseHandle(file_handle);

                            // Parse and validate header
                            const fwcfg_header_t* header = reinterpret_cast<const fwcfg_header_t*>(buffer.get_ptr());
                            if (header->magic != FWCFG_MAGIC)
                            {
                                uMessageBox(*this, "Invalid configuration file: wrong file format.", "Import Error",
                                            MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }

                            if (header->data_size != file_size - FWCFG_HEADER_SIZE)
                            {
                                uMessageBox(*this, "Invalid configuration file: data size mismatch.", "Import Error",
                                            MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }

                            // Verify CRC
                            const uint32_t calculated_crc = Utils::CalculateCrc32(buffer.get_ptr() + FWCFG_HEADER_SIZE, header->data_size);
                            if (header->data_crc != calculated_crc)
                            {
                                uMessageBox(*this, "Invalid configuration file: CRC check failed.", "Import Error",
                                            MB_OK | MB_ICONERROR);
                                CoTaskMemFree(file_path);
                                return;
                            }

                            // Store the current GUID before removal
                            GUID old_guid = host_config->guid;

                            // Generate new GUID for imported config
                            GUID new_guid;
                            CoCreateGuid(&new_guid);

                            // Create new config and load data from file
                            auto new_config = CfgFlowin::Get()->AddOrFindConfiguration(new_guid);
                            if (new_config)
                            {
                                stream_reader_memblock_ref reader(buffer.get_ptr() + FWCFG_HEADER_SIZE, header->data_size);
                                new_config->SetDataRaw(&reader, header->data_size, fb2k::noAbort);
                                // Restore the GUID we generated
                                new_config->guid = new_guid;
                            }

                            // Save reference to FlowinCore
                            auto core = FlowinCore::Get();

                            // Remove current flowin (window + config)
                            core->RemoveFlowin(old_guid, true);

                            // Create new flowin window
                            core->CreateFlowin(new_guid);
                        }
                        catch (std::exception&)
                        {
                            uMessageBox(*this, "Failed to import configuration.", "Import Error", MB_OK | MB_ICONERROR);
                        }
                        CoTaskMemFree(file_path);
                    }
                }
            }
        }
    }

private:
    // fix me. not standard impl
    ui_element_config::ptr dummy_config;
    ui_element_instance_ptr element_inst;
    FlowinUiElementInstanceCallbackImpl::ptr callback;
    CfgFlowinHost::Ptr host_config;
    bool is_perform_drag = false;
    POINT drag_point;
    const int kTransparencyTimerID = 0x1001;
    const int kHoverHideTimerID = 0x1003;
    const int kHoverHideCheckTimerID = 0x1004;
    UINT_PTR transparency_timer = 0;
    UINT_PTR hover_hide_timer = 0;
    int32_t transparency_intermediate = -1;
    int32_t hover_hide_alpha = 255;
    bool is_hover_hiding = false;
    DarkMode::CHooks dark_mode_hooks;
    std::vector<FlowinMenuNode::Ptr> menu_nodes;
    bool interrupt_context_menu = false;
};

class UiElementFlowinHostImpl : public ui_element_impl<FlowinHost>
{
public:
    bool is_user_addable()
    {
        return false;
    }
};

namespace
{
static service_factory_single_t<UiElementFlowinHostImpl> g_ui_element_dummy_impl_factory;
} // namespace
