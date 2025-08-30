#include "pch.h"
#include <shobjidl.h>
#include <comdef.h>
#include <dwmapi.h>
#include <mutex>
#include "snap_window.h"
#include "flowin_vars.h"
#include "flowin_config.h"
#include "flowin_core.h"
#include "helpers/ui_element_helpers.h"
#include "helpers/atl-misc.h"
#include "libPPUI/win32_utility.h"
#include "flowin_utils.h"
#include "ui/ui_custom_title.h"
#include "ui/ui_transparency_settings.h"
#include "ui/ui_no_frame.h"
#include "resource.h"

#pragma comment(lib, "dwmapi.lib")

// clang-format off
typedef CWinTraits<WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME, 0> CFlowinTraits;
// clang-format on

class flowin_host : public ui_element_helpers::ui_element_instance_host_base,
                    public CWindowImpl<flowin_host, CWindow, CFlowinTraits>,
                    public CSnapWindow<flowin_host>,
                    public message_filter_impl_base
{
    class flowin_ui_element_instance_callback_impl : public ui_element_instance_callback_v3
    {
    public:
        flowin_ui_element_instance_callback_impl(class flowin_host* host, ui_element_instance_callback_ptr callback)
            : host_(host), callback_(callback)
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
            if (callback_.is_valid())
                return callback_->query_color(p_what, p_out);
            return false;
        }

        bool request_activation(service_ptr_t<class ui_element_instance> p_item) override
        {
            return true;
        }

        bool is_edit_mode_enabled() override
        {
            return host_ ? host_->is_edit_mode_enabled() : false;
        }

        void request_replace(service_ptr_t<class ui_element_instance> p_item) override
        {
            if (callback_.is_valid())
                callback_->request_replace(p_item);
        }

        t_ui_font query_font_ex(const GUID& p_what) override
        {
            return callback_.is_valid() ? callback_->query_font_ex(p_what) : nullptr;
        }

        bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) override
        {
            return host_ ? host_->host_is_child_visible(0) : false;
        }

        t_size notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2,
                      t_size param2size) override
        {
            return host_ ? host_->host_notify(source, what, param1, param2, param2size) : 0;
        }

    private:
        flowin_host* host_;
        ui_element_instance_callback_ptr callback_;
    };

public:
    DECLARE_WND_CLASS(TEXT("{EA622005-1140-4EF1-B64D-4A215DB3526A}"));

    BEGIN_MSG_MAP_EX(flowin_host)
        MSG_WM_CREATE(on_create)
        MSG_WM_PAINT(on_paint)
        MSG_WM_ERASEBKGND(on_erase_bkgnd)
        MSG_WM_ACTIVATE(on_active)
        MSG_WM_LBUTTONUP(on_lbutton_up)
        MSG_WM_MBUTTONDOWN(on_mbutton_down)
        MSG_WM_MBUTTONUP(on_mbutton_up)
        MSG_WM_MOUSEMOVE(on_mouse_move)
        MSG_WM_CONTEXTMENU(on_context_menu)
        MSG_WM_INITMENUPOPUP(on_init_menu_popup)
        MSG_WM_SYSCOMMAND(on_sys_command)
        MSG_WM_SHOWWINDOW(on_show_window)
        MSG_WM_SIZE(on_size)
        MSG_WM_NCCALCSIZE(on_nc_calc_size)
        MSG_WM_NCHITTEST(on_nc_hittest)
        MSG_WM_NCACTIVATE(on_nc_active)
        MSG_WM_SETCURSOR(on_set_cursor)
        MSG_WM_TIMER(on_timer)
        MSG_WM_CLOSE(on_close)
        MSG_WM_DESTROY(on_destroy)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_COMMAND, on_flowin_command)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_REFRESH_CONFIG, on_refresh_config)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_ACTIVE, on_active_flowin)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_UPDATE_TRANSPARENCY, on_update_transparency)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_REPAINT, on_repaint)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_COLOR_CHANGED, on_ui_color_changed)
        CHAIN_MSG_MAP(ui_element_instance_host_base)
        CHAIN_MSG_MAP(CSnapWindow<flowin_host>)
    END_MSG_MAP()

    flowin_host(ui_element_config::ptr p_config, ui_element_instance_callback_ptr p_callback)
        : ui_element_instance_host_base(p_callback), dummy_config_(p_config), host_config_(nullptr)
    {
        callback_ = new service_impl_t<flowin_ui_element_instance_callback_impl>(this, p_callback);
        set_configuration(p_config);
    }

    virtual ~flowin_host()
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

    bool has_child() const
    {
        return element_inst_.is_valid() && ::IsWindow(element_inst_->get_wnd());
    }

    bool pretranslate_message(MSG* p_msg)
    {
        if (!host_config_)
            return false;

        if (!IsChild(p_msg->hwnd))
            return false;

        bool forward_message = false;
        switch (p_msg->message)
        {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
            if (host_config_->enable_snap || host_config_->enable_transparency_active)
                forward_message = true;

            if (is_perform_drag_)
                forward_message = true;
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (is_cfg_no_frame())
                forward_message = true;
            break;

        default:
            break;
        }

        if (forward_message)
            SendMessage(p_msg->message, p_msg->wParam, p_msg->lParam);

        return false;
    }

    HWND get_wnd() override
    {
        return *this;
    }

    bool is_cfg_no_frame() const
    {
        return !host_config_->show_caption;
    }

    auto& get_cfg_no_frame() const
    {
        return host_config_->cfg_no_frame;
    }

    bool use_legacy_no_frame() const
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

        bool ret = s_is_win11 != 1 || !flowin_utils::is_composition_enabled();
        if (host_config_)
            host_config_->cfg_no_frame.legacy_no_frame = ret ? 1 : 0;

        return ret;
    }

    void show_or_hide_on_taskbar(bool show)
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

    bool is_active() const
    {
        return GetActiveWindow() == m_hWnd;
    }

    void set_configuration(ui_element_config::ptr config) override
    {
        const auto guid = configuration::guid_from_element_config(config);
        host_config_ = configuration::add_or_find(guid);
    }

    ui_element_config::ptr get_configuration() override
    {
        return dummy_config_;
    }

    void host_replace_element(unsigned /*p_id*/, ui_element_config::ptr cfg) override
    {
        const GUID& new_guid = cfg->get_guid();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, new_guid))
        {
            element_inst_ = element->instantiate(*this, cfg, callback_);
            if (!has_child())
                return;
            // resize host window
            PostMessage(WM_SIZE);
            // refresh element config
            if (host_config_->subelement_guid != new_guid)
            {
                host_config_->subelement_guid = new_guid;
                element->get_name(host_config_->window_title);
                uSetWindowText(*this, host_config_->window_title);
            }

            host_config_->write_subelement(cfg);
        }
    }

    void host_replace_element(unsigned p_id, const GUID& p_newguid) override
    {
        element_inst_.reset();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, p_newguid))
        {
            ui_element_config::ptr cfg;
            if (p_newguid == host_config_->subelement_guid)
            {
                // restore child element
                cfg = host_config_->subelement(0);
            }
            else
            {
                cfg = element->get_default_configuration();
                if (element->get_subclass() == ui_element_subclass_containers)
                    host_config_->edit_mode = true;
            }

            host_replace_element(p_id, cfg);
        }
    }

    ui_element_instance_ptr host_get_child(t_size /*which*/) override
    {
        return element_inst_;
    }

    t_size host_get_children_count() override
    {
        return has_child() ? 1 : 0;
    }

    void host_bring_to_front(t_size /*which*/) override
    {
        bring_window_to_top();
    }

    void host_replace_child(t_size which) override
    {
        callback_->request_replace(host_get_child(which));
    }

    bool host_is_child_visible(t_size which) override
    {
        return has_child() && ::IsWindowVisible(element_inst_->get_wnd());
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
        return host_config_->edit_mode;
    }

    t_size host_notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2,
                       t_size param2size)
    {
        return 0;
    }

private:
    bool is_transparency_enabled()
    {
        return (host_config_->transparency > 0) ||
               (host_config_->enable_transparency_active && host_config_->transparency_active > 0);
    }

    void calc_intermediate_transparency(int32_t target_transparency)
    {
        const int32_t delta = 12;
        if (tranparency_intermediate_ < target_transparency)
        {
            tranparency_intermediate_ = min(target_transparency, tranparency_intermediate_ + delta);
        }
        else if (tranparency_intermediate_ > target_transparency)
        {
            tranparency_intermediate_ = max(target_transparency, tranparency_intermediate_ - delta);
        }
    }

    void update_transparency(int transparency = -1)
    {
        if (host_config_->enable_transparency_active && host_config_->transparency != host_config_->transparency_active)
        {
            if (tranparency_intermediate_ == -1)
            {
                tranparency_intermediate_ =
                    is_active() ? host_config_->transparency : host_config_->transparency_active;
            }
            else if (transparency_timer_ == NULL)
            {
                transparency_timer_ = SetTimer(kTransparencyTimerID, 20);
                return;
            }
        }

        if (transparency_timer_ && transparency == -1)
            return;

        PostMessage(UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)transparency);
    }

    void set_always_on_top(bool on_top)
    {
        SetWindowPos(on_top ? HWND_TOPMOST : HWND_NOTOPMOST, CRect{0}, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    void bring_window_to_top()
    {
        PostMessage(UWM_FLOWIN_COMMAND, t_menu_cmd_flowin_bring_to_top);
    }

    void show_no_frame_shadow(bool show)
    {
        if (flowin_utils::is_composition_enabled())
        {
            static const MARGINS extend_margins[2]{{0, 0, 0, 0}, {1, 1, 1, 1}};
            ::DwmExtendFrameIntoClientArea(get_wnd(), &extend_margins[show ? 1 : 0]);
        }
    }

    void insert_menu(HMENU menu, UINT id, LPCWSTR caption, bool enabled = true, bool checked = false)
    {
        MENUITEMINFOW mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (ULONG_PTR)this;
        if (id)
        {
            mii.fMask |= MIIM_ID | MIIM_STRING | MIIM_STATE;
            mii.wID = id;
            mii.fState = (enabled ? MFS_ENABLED : MFS_DISABLED) | (checked ? MFS_CHECKED : MFS_UNCHECKED);
            mii.dwTypeData = const_cast<LPWSTR>(caption);
        }
        else
        {
            mii.fType = MFT_SEPARATOR;
        }

        InsertMenuItemW(menu, SC_CLOSE, FALSE, &mii);
    }

    void cleanup_system_menu()
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

    void build_context_menu(HMENU menu, bool sys_menu = true)
    {
        insert_menu(menu, t_menu_cmd_show_flowin_on_startup, L"Show on startup", true, host_config_->show_on_startup);
        insert_menu(menu, t_menu_cmd_always_on_top, L"Always on top", true, host_config_->always_on_top);
        insert_menu(menu, t_menu_cmd_flowin_no_frame, L"No window frame", true, !host_config_->show_caption);
        insert_menu(menu, t_menu_cmd_show_in_taskbar, L"Add to the taskbar", true, host_config_->show_in_taskbar);
        insert_menu(menu, t_menu_cmd_snap_to_edge, L"Snap to screen edge", true, host_config_->enable_snap);
        insert_menu(menu, t_menu_cmd_snap_auto_hide, L"Auto hide when snapped", host_config_->enable_snap,
                    host_config_->enable_autohide_when_snapped);
        insert_menu(menu, t_menu_cmd_edit_mode, L"Edit mode", true, host_config_->edit_mode);
        insert_menu(menu, t_menu_cmd_flowin_custom_title, L"Custom title");
        insert_menu(menu, t_menu_cmd_flowin_transparency, L"Transparency");
        insert_menu(menu, t_menu_cmd_destroy_element, L"Delete");

        if (sys_menu)
            insert_menu(menu, 0, nullptr);
    }

    void execute_context_menu(int cmd, int param = 0)
    {
        switch (cmd)
        {
        case t_menu_cmd_show_flowin_on_startup:
            host_config_->show_on_startup = !host_config_->show_on_startup;
            break;

        case t_menu_cmd_always_on_top:
            host_config_->always_on_top = !host_config_->always_on_top;
            set_always_on_top(host_config_->always_on_top);
            break;

        case t_menu_cmd_flowin_no_frame:
            if (!host_config_->show_caption)
            {
                host_config_->show_caption = true;
                configure_window_style();
            }
            else
            {
                bool apply_command = true;
                if (!param)
                {
                    CNoFrameSettingsDialog dlg(host_config_);
                    if (dlg.DoModal(*this) != IDOK)
                        apply_command = false;
                }

                if (apply_command)
                {
                    host_config_->show_caption = false;
                    configure_window_style();
                }
            }
            break;

        case t_menu_cmd_show_in_taskbar:
            host_config_->show_in_taskbar = !host_config_->show_in_taskbar;
            show_or_hide_on_taskbar(host_config_->show_in_taskbar);
            configure_window_style();
            break;

        case t_menu_cmd_snap_to_edge:
            host_config_->enable_snap = !host_config_->enable_snap;
            enable_window_snap_ = host_config_->enable_snap;
            if (!enable_window_snap_)
                RestoreFromSnapHidden();
            break;

        case t_menu_cmd_snap_auto_hide:
            host_config_->enable_autohide_when_snapped = !host_config_->enable_autohide_when_snapped;
            enable_window_snap_auto_hide_ = host_config_->enable_autohide_when_snapped;
            if (!enable_window_snap_auto_hide_)
                RestoreFromSnapHidden();
            break;

        case t_menu_cmd_edit_mode:
            host_config_->edit_mode = !host_config_->edit_mode;
            if (has_child())
                element_inst_->notify(ui_element_notify_edit_mode_changed, 0, nullptr, 0);
            break;

        case t_menu_cmd_destroy_element: {
            pfc::string8 element_name;
            uGetWindowText(*this, element_name);
            pfc::string_formatter msg;
            msg << " You are about to delete \"" << uGetWindowText(*this).c_str()
                << "\".\n This action cannot be undone.  Do you want to continue?";
            if (uMessageBox(*this, msg, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
                fb2k::inMainThread([this]() { flowin_core::get()->remove_flowin(this->host_config_->guid, true); });
            break;
        }

        case t_menu_cmd_flowin_custom_title: {
            CCustomTitleDialog dlg(host_config_->window_title);
            if (IDOK == dlg.DoModal(*this))
            {
                if (host_config_->window_title.is_empty())
                {
                    if (has_child())
                        ui_element::g_get_name(host_config_->window_title, element_inst_->get_guid());
                }

                ::uSetWindowText(*this, host_config_->window_title);
            }

            break;
        }

        case t_menu_cmd_flowin_transparency: {
            CTransparencySetDialog dlg(m_hWnd, host_config_);
            dlg.DoModal(*this);
            update_transparency();
            break;
        }

        case t_menu_cmd_flowin_reset_position: {
            CenterWindow(core_api::get_main_window());
            BringWindowToTop();
            break;
        }

        case t_menu_cmd_flowin_bring_to_top: {
            RestoreFromSnapHidden();
            set_always_on_top(!host_config_->always_on_top);
            set_always_on_top(host_config_->always_on_top);
            break;
        }

        default:
            break;
        }
    }

    void ajust_rect_to_primary_monitor(LPRECT rect, BOOL center = FALSE)
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

    void ajust_window_position(LPRECT rect = nullptr, BOOL center = FALSE)
    {
        RECT rc_window = {};
        auto rc = rect ? rect : &rc_window;
        if (rect == nullptr)
            WIN32_OP_D(GetWindowRect(rc));
        ajust_rect_to_primary_monitor(rc, center);
        WIN32_OP_D(SetWindowPos(nullptr, rc, SWP_NOZORDER | SWP_NOACTIVATE));
    }

    void adjust_maximized_client_rect(LPRECT rect)
    {
        if (flowin_utils::is_maximized(get_wnd()))
        {
            HMONITOR mon = MonitorFromWindow(get_wnd(), MONITOR_DEFAULTTONEAREST);
            WIN32_OP_D(mon != NULL);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            WIN32_OP_D(GetMonitorInfo(mon, &mi));
            *rect = mi.rcWork;
        }
    }

    void configure_window_style()
    {
        // frame
        const DWORD rel_style = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
        if (is_cfg_no_frame())
        {
            // window style
            if (use_legacy_no_frame())
            {
                ModifyStyle(rel_style, 0);
            }
            else
            {
                ModifyStyle(0, rel_style);
                show_no_frame_shadow(get_cfg_no_frame().shadowed);
            }
            // HACK
            // TODO snap in no frame mode not fully supported
            kSnapHideEdgeWidth = 2;
        }
        else
        {
            ModifyStyle(!host_config_->show_in_taskbar ? (WS_MAXIMIZEBOX | WS_MINIMIZEBOX) : 0,
                        rel_style | (host_config_->show_in_taskbar ? (WS_MAXIMIZEBOX | WS_MINIMIZEBOX) : 0));
            show_no_frame_shadow(false);
            kSnapHideEdgeWidth = 8;
        }

        // notify changes
        SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }

private:
    int on_create(LPCREATESTRUCT lpcs)
    {
        SetIcon(ui_control::get()->get_main_icon());

        flowin_core::get()->register_flowin(m_hWnd, host_config_->guid);

        ui_config_manager::ptr api;
        if (ui_config_manager::tryGet(api) && api->is_dark_mode())
        {
            dark_mode_hooks_.AddDialog(m_hWnd);
            dark_mode_hooks_.SetDark(true);
        }

        (VOID) GetSystemMenu(FALSE);
        (VOID) use_legacy_no_frame();

        if (IsRectEmpty(&host_config_->window_rect))
            CenterWindow(core_api::get_main_window());
        else
            ajust_window_position(&host_config_->window_rect);

        if (host_config_->window_title.is_empty())
            g_get_name(host_config_->window_title);

        ::uSetWindowText(*this, host_config_->window_title);

        if (host_config_->subelement_guid != pfc::guid_null)
            host_replace_element(0, host_config_->subelement_guid);

        set_always_on_top(host_config_->always_on_top);
        configure_window_style();

        // snap config
        enable_window_snap_ = host_config_->enable_snap;
        enable_window_snap_auto_hide_ = host_config_->enable_autohide_when_snapped;

        if (!host_config_->show_in_taskbar)
            show_or_hide_on_taskbar(false);

        if (is_transparency_enabled())
            update_transparency();

        if (!host_config_->always_on_top)
            bring_window_to_top();

        return TRUE;
    }

    void on_close()
    {
        ShowWindow(SW_HIDE);
        SendMessage(UWM_FLOWIN_REFRESH_CONFIG);
        SetMsgHandled(FALSE);
    }

    void on_destroy()
    {
        flowin_core::get()->remove_flowin(host_config_->guid);
        host_config_.reset();
        SetMsgHandled(FALSE);
    }

    void on_paint(CDCHandle /*dc*/)
    {
        CPaintDC dc(*this);
        t_ui_color text_color;
        if (!callback_->query_color(ui_color_text, text_color))
            text_color = GetSysColor(COLOR_BTNTEXT);

        dc.SetTextColor(text_color);
        dc.SetBkMode(TRANSPARENT);
        SelectObjectScope scope(dc, (HGDIOBJ)callback_->query_font_ex(ui_font_default));
        CRect rc;
        GetClientRect(&rc);
        dc.DrawText(_T("Click to add new element."), -1, &rc, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    BOOL on_erase_bkgnd(CDCHandle dc)
    {
        CRect rc;
        GetClientRect(&rc);
        CBrush brush;
        t_ui_color background_color;
        if (!callback_->query_color(ui_color_background, background_color))
            background_color = GetSysColor(COLOR_BTNFACE);

        brush.CreateSolidBrush(background_color);
        dc.FillRect(&rc, brush);
        return TRUE;
    }

    void on_active(UINT state, BOOL /*minimized*/, CWindow /*wnd_other*/)
    {
        if (is_transparency_enabled())
        {
            static bool first_time_active = true;
            if (first_time_active)
            {
                PostMessage(UWM_FLOWIN_UPDATE_TRANSPARENCY, -1);
                first_time_active = false;
            }
            else
            {
                update_transparency();
            }
        }

        flowin_core::get()->set_latest_active_flowin(host_config_->guid);
    }

    void on_lbutton_up(UINT flags, CPoint point)
    {
        if (!has_child())
        {
            replace_dialog(*this, 0, pfc::guid_null);
        }
    }

    void on_mbutton_down(UINT flags, CPoint point)
    {
        GetCursorPos(&drag_point_);
        if (is_cfg_no_frame() && get_cfg_no_frame().draggable)
        {
            SetCapture();
            is_perform_drag_ = true;
        }
    }

    void on_mbutton_up(UINT flags, CPoint point)
    {
        if (is_perform_drag_)
        {
            ReleaseCapture();
            is_perform_drag_ = false;
        }
    }

    void on_mouse_move(UINT flags, CPoint point)
    {
        if ((flags & MK_MBUTTON) && is_perform_drag_)
        {
            POINT pt{};
            GetCursorPos(&pt);
            RECT rect;
            WIN32_OP_D(GetWindowRect(&rect));
            OffsetRect(&rect, pt.x - drag_point_.x, pt.y - drag_point_.y);
            MoveWindow(&rect);
            drag_point_ = pt;
            return;
        }

        SetMsgHandled(FALSE);
    }

    void on_context_menu(CWindow wnd, CPoint point)
    {
        RECT rect_client;
        GetClientRect(&rect_client);
        ClientToScreen(&rect_client);
        SetMsgHandled(FALSE);
        if (!PtInRect(&rect_client, point))
            return;

        if (!callback_->is_edit_mode_enabled() && has_child())
            return;

        auto inst = has_child()
                        ? element_inst_
                        : ui_element_helpers::instantiate_dummy(*this, ui_element_config::g_create_empty(), callback_);
        standard_edit_context_menu(MAKELPARAM(point.x, point.y), inst, 0, *this);
        SetMsgHandled(TRUE);
    }

    void on_init_menu_popup(CMenuHandle menu, UINT idx, BOOL sys_menu)
    {
        if (sys_menu && menu.m_hMenu == GetSystemMenu(FALSE))
        {
            cleanup_system_menu();
            build_context_menu(menu);
        }
    }

    void on_sys_command(UINT id, CPoint /*point*/)
    {
        execute_context_menu(id);
        SetMsgHandled(FALSE);
    }

    void on_show_window(BOOL show, UINT /*status*/)
    {
        if (has_child())
        {
            element_inst_->notify(ui_element_notify_visibility_changed, (t_size) !!show, nullptr, 0);
        }
    }

    void on_size(UINT type, CSize size)
    {
        if (has_child())
        {
            CRect rc;
            GetClientRect(&rc);
            if (is_cfg_no_frame() && get_cfg_no_frame().resizable)
            {
                InflateRect(&rc, -2, -2);
            }

            ::SetWindowPos(element_inst_->get_wnd(), HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(),
                           SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
        }
    }

    LRESULT on_nc_calc_size(BOOL calc_valid_rect, LPARAM param)
    {
        if (!is_cfg_no_frame())
        {
            SetMsgHandled(FALSE);
        }
        else if (!calc_valid_rect || use_legacy_no_frame())
        {
            SetMsgHandled(FALSE);
        }
        else
        {
            LPNCCALCSIZE_PARAMS lpnccs_params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(param);
            adjust_maximized_client_rect(lpnccs_params->rgrc);
        }

        return 0;
    }

    UINT on_nc_hittest(CPoint point)
    {
        if (!is_cfg_no_frame())
        {
            SetMsgHandled(FALSE);
            return 0;
        }

        UINT res = HTCLIENT;

        if (!get_cfg_no_frame().resizable)
            return res;

        const int cx = kWindowFrameX;
        const int cy = kWindowFrameY;

        RECT rect_window{};
        WIN32_OP_D(GetWindowRect(&rect_window));

        // left & right
        if (point.x <= rect_window.left + cx)
            res = HTLEFT;
        else if (point.x >= (rect_window.right - cx))
            res = HTRIGHT;

        // top & bottom
        if (point.y <= rect_window.top + cy)
        {
            switch (res)
            {
            case HTLEFT:
                res = HTTOPLEFT;
                break;

            case HTRIGHT:
                res = HTTOPRIGHT;
                break;

            default:
                res = HTTOP;
                break;
            }
        }
        else if (point.y >= (rect_window.bottom - cy))
        {
            switch (res)
            {
            case HTLEFT:
                res = HTBOTTOMLEFT;
                break;

            case HTRIGHT:
                res = HTBOTTOMRIGHT;
                break;

            default:
                res = HTBOTTOM;
                break;
            }
        }

        return res;
    }

    BOOL on_nc_active(BOOL active)
    {
        if (is_cfg_no_frame() && !flowin_utils::is_composition_enabled())
            return TRUE;

        SetMsgHandled(FALSE);
        return FALSE;
    }

    BOOL on_set_cursor(CWindow /*wnd*/, UINT hittest, UINT message)
    {
        if (!is_cfg_no_frame() || !use_legacy_no_frame())
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

    void on_timer(UINT_PTR id)
    {
        if (id == kTransparencyTimerID)
        {
            uint32_t target_transparency = is_active() ? host_config_->transparency_active : host_config_->transparency;
            calc_intermediate_transparency(target_transparency);
            update_transparency(tranparency_intermediate_);
            if (tranparency_intermediate_ == target_transparency)
            {
                KillTimer(id);
                transparency_timer_ = NULL;
            }

            return;
        }

        SetMsgHandled(FALSE);
    }

    LRESULT on_flowin_command(UINT /*msg*/, WPARAM wp, LPARAM lp)
    {
        execute_context_menu((t_uint32)wp, (int)lp);
        return TRUE;
    }

    LRESULT on_refresh_config(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        GetSnapWindowRect(&host_config_->window_rect);
        if (has_child())
            host_config_->write_subelement(element_inst_->get_configuration());

        return TRUE;
    }

    LRESULT on_active_flowin(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        BringWindowToTop();
        return TRUE;
    }

    LRESULT on_update_transparency(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/)
    {
        ModifyStyleEx(0, WS_EX_LAYERED);
        int transparency = (int)wp;
        BYTE alpha = 0;
        if (transparency >= 0)
            alpha = (BYTE)(255.0 - transparency * 255.0 / 100);
        else if (host_config_->enable_transparency_active && GetActiveWindow() == m_hWnd)
            alpha = (BYTE)(255.0 - host_config_->transparency_active * 255.0 / 100);
        else
            alpha = (BYTE)(255.0 - host_config_->transparency * 255.0 / 100);

        SetLayeredWindowAttributes(*this, 0, alpha, LWA_ALPHA);
        return TRUE;
    }

    LRESULT on_repaint(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        InvalidateRect(nullptr);
        return 0;
    }

    LRESULT on_ui_color_changed(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/)
    {
        ui_config_manager::ptr api;
        if (ui_config_manager::tryGet(api))
            dark_mode_hooks_.SetDark(api->is_dark_mode());

        return 0;
    }

private:
    // fix me. not standard impl
    ui_element_config::ptr dummy_config_;
    ui_element_instance_ptr element_inst_;
    flowin_ui_element_instance_callback_impl::ptr callback_;
    cfg_flowin_host::sp_t host_config_;
    bool is_perform_drag_ = false;
    POINT drag_point_;
    const int kTransparencyTimerID = 0x1001;
    UINT_PTR transparency_timer_ = 0;
    int32_t tranparency_intermediate_ = -1;
    DarkMode::CHooks dark_mode_hooks_;
};

class ui_element_flowin_host_impl : public ui_element_impl<flowin_host>
{
public:
    bool is_user_addable()
    {
        return false;
    }
};

namespace
{
static service_factory_single_t<ui_element_flowin_host_impl> g_ui_element_dummy_impl_factory;
} // namespace
