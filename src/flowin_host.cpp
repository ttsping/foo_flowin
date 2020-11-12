#include "pch.h"
#include <shobjidl.h>
#include <comdef.h>
#include "snap_window.h"
#include "flowin_vars.h"
#include "flowin_config.h"
#include "flowin_core.h"
#include "helpers/ui_element_helpers.h"
#include "helpers/atl-misc.h"
#include "libPPUI/win32_utility.h"
#include "flowin_utils.h"
#include "resource.h"

_COM_SMARTPTR_TYPEDEF(ITaskbarList, __uuidof(ITaskbarList));

namespace {

class CCustomTitleDialog : public CDialogImpl<CCustomTitleDialog> {
  public:
    enum { IDD = IDD_HOST_CUSTOM_TITLE };

    CCustomTitleDialog(pfc::string8& title) : title_(title) {}

    BEGIN_MSG_MAP_EX(CCustomTitleDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

  private:
    BOOL OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/) {
        CenterWindow(GetParent());
        ::uSetDlgItemText(*this, IDC_EDIT_CUSTOM_TITLE, title_);
        return TRUE;
    }

    void OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/) {
        if (id == IDOK) {
            ::uGetDlgItemText(*this, IDC_EDIT_CUSTOM_TITLE, title_);
        }
        EndDialog(id);
    }

  private:
    pfc::string8& title_;
};

class CTransparencySetDialog : public CDialogImpl<CTransparencySetDialog> {
  public:
    enum { IDD = IDD_TRANSPARENCY };

    CTransparencySetDialog(HWND wnd, cfg_flowin_host::sp_t host_cfg) : window_(wnd), cfg_(host_cfg) {}

    BEGIN_MSG_MAP_EX(CTransparencySetDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
        COMMAND_ID_HANDLER_EX(IDC_CHK_TRANSPARENCY_ACTIVE, OnEnableHoverTransparency)
        MSG_WM_HSCROLL(OnHScroll)
    END_MSG_MAP()

  private:
    BOOL OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/) {
        CenterWindow(GetParent());
        track_ctrl_.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY));
        track_ctrl_.SetRange(0, 100);
        track_ctrl_.SetPos((int)cfg_->transparency);

        uButton_SetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE, cfg_->enable_transparency_active);

        track_hover_ctrl_.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY_HOVER));
        track_hover_ctrl_.SetRange(0, 100);
        track_hover_ctrl_.SetPos((int)cfg_->transparency_active);
        if (!cfg_->enable_transparency_active) {
            track_hover_ctrl_.EnableWindow(FALSE);
        }
        return TRUE;
    }

    void OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/) { EndDialog(id); }

    void OnEnableHoverTransparency(UINT /*code*/, int /*id*/, CWindow /*ctrl*/) {
        cfg_->enable_transparency_active = uButton_GetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE);
        track_hover_ctrl_.EnableWindow(cfg_->enable_transparency_active);
    }

    void OnHScroll(UINT code, UINT /*pos*/, CTrackBarCtrl ctrl) {
        if (code == TB_THUMBTRACK) {
            int transparency = -1;
            cfg_->transparency = track_ctrl_.GetPos();
            cfg_->transparency_active = track_hover_ctrl_.GetPos();
            transparency = ctrl.GetDlgCtrlID() == IDC_SLIDER_TRANSPARENCY ? (int)cfg_->transparency : (int)cfg_->transparency_active;
            ::PostMessage(window_, UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)transparency, 0);
        }
    }

  private:
    cfg_flowin_host::sp_t cfg_;
    HWND window_;
    CTrackBarCtrl track_ctrl_;
    CTrackBarCtrl track_hover_ctrl_;
};

typedef CWinTraits<WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME, 0> CFlowinTraits;

class flowin_host : public ui_element_helpers::ui_element_instance_host_base,
                    public CWindowImpl<flowin_host, CWindow, CFlowinTraits>,
                    public CSnapWindow<flowin_host>,
                    public message_filter_impl_base {
    class flowin_ui_element_instance_callback_impl : public ui_element_instance_callback_v3 {
      public:
        flowin_ui_element_instance_callback_impl(class flowin_host* host, ui_element_instance_callback_ptr callback) : host_(host), callback_(callback) {}
        void on_min_max_info_change() {}
        virtual void on_alt_pressed(bool p_state) {}
        bool query_color(const GUID& p_what, t_ui_color& p_out) {
            if (callback_.is_valid())
                return callback_->query_color(p_what, p_out);
            return false;
        }
        bool request_activation(service_ptr_t<class ui_element_instance> p_item) { return true; }
        bool is_edit_mode_enabled() {
            if (host_)
                return host_->is_edit_mode_enabled();
            return false;
        }
        void request_replace(service_ptr_t<class ui_element_instance> p_item) {
            if (callback_.is_valid())
                callback_->request_replace(p_item);
        }
        t_ui_font query_font() { return query_font_ex(ui_font_default); }
        t_ui_font query_font_ex(const GUID& p_what) {
            if (callback_.is_valid())
                return callback_->query_font_ex(p_what);
            ;
            return NULL;
        }
        bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) {
            if (host_)
                return host_->host_is_child_visible(0);
            return false;
        }
        t_size notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2, t_size param2size) {
            if (host_)
                return host_->host_notify(source, what, param1, param2, param2size);
            return 0;
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
        MSG_WM_CONTEXTMENU(on_context_menu)
        MSG_WM_INITMENUPOPUP(on_init_menu_popup)
        MSG_WM_SYSCOMMAND(on_sys_command)
        MSG_WM_SHOWWINDOW(on_show_window)
        MSG_WM_SIZE(on_size)
        MSG_WM_CLOSE(on_close)
        MSG_WM_DESTROY(on_destroy)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_COMMAND, on_flowin_command)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_REFRESH_CONFIG, on_refresh_config)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_ACTIVE, on_active_flowin)
        MESSAGE_HANDLER_EX(UWM_FLOWIN_UPDATE_TRANSPARENCY, on_update_transparency)
        CHAIN_MSG_MAP(ui_element_instance_host_base)
        CHAIN_MSG_MAP(CSnapWindow<flowin_host>)
    END_MSG_MAP()

    flowin_host(ui_element_config::ptr p_config, ui_element_instance_callback_ptr p_callback)
        : ui_element_instance_host_base(p_callback), dummy_config_(p_config), host_config_(nullptr) {
        callback_ = new service_impl_t<flowin_ui_element_instance_callback_impl>(this, p_callback);
        set_configuration(p_config);
    }
    static GUID g_get_guid() { return g_dui_flowin_host_guid; }
    static GUID g_get_subclass() { return ui_element_subclass_utility; }
    static void g_get_name(pfc::string_base& out) { out = "Flowin"; }
    static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return ""; }

    bool has_child() { return element_inst_.is_valid() && ::IsWindow(element_inst_->get_wnd()); }

    bool pretranslate_message(MSG* p_msg) {
        if (p_msg->message == WM_MOUSEMOVE || p_msg->message == WM_NCMOUSEMOVE) {
            if (host_config_ && (host_config_->enable_snap || host_config_->enable_transparency_active) && IsChild(p_msg->hwnd)) {
                SendMessage(p_msg->message, p_msg->wParam, p_msg->lParam);
            }
        }
        return false;
    }

    HWND get_wnd() override { return *this; }

    void set_configuration(ui_element_config::ptr config) override {
        host_config_ = cfg_flowin::get()->add_or_find_configuration(cfg_flowin_host::cfg_get_guid(config));
    }

    ui_element_config::ptr get_configuration() override { return dummy_config_; }

    void host_replace_element(unsigned /*p_id*/, ui_element_config::ptr cfg) override {
        const GUID& new_guid = cfg->get_guid();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, new_guid)) {
            element_inst_ = element->instantiate(*this, cfg, callback_);
            if (!has_child()) {
                return;
            }
            // resize host window
            PostMessage(WM_SIZE);
            // refresh element config
            if (host_config_->subelement_guid != new_guid) {
                host_config_->subelement_guid = new_guid;
                element->get_name(host_config_->window_title);
                uSetWindowText(*this, host_config_->window_title);
            }
            host_config_->write_subelement(cfg);
        }
    }

    void host_replace_element(unsigned p_id, const GUID& p_newguid) override {
        element_inst_.reset();
        service_ptr_t<ui_element> element;
        if (ui_element::g_find(element, p_newguid)) {
            ui_element_config::ptr cfg;
            if (p_newguid == host_config_->subelement_guid) {  // restore child element
                cfg = host_config_->subelement(0);
            } else {
                cfg = element->get_default_configuration();
                if (element->get_subclass() == ui_element_subclass_containers) {
                    host_config_->edit_mode = true;
                }
            }
            host_replace_element(p_id, cfg);
        }
    }

    ui_element_instance_ptr host_get_child(t_size /*which*/) override { return element_inst_; }

    t_size host_get_children_count() override { return has_child() ? 1 : 0; }

    void host_bring_to_front(t_size which) override {
        if (has_child()) {
            WIN32_OP_D(::BringWindowToTop(element_inst_->get_wnd()));
        }
    }

    void host_replace_child(t_size which) override { callback_->request_replace(host_get_child(which)); }

    bool host_is_child_visible(t_size which) override { return has_child() && ::IsWindowVisible(element_inst_->get_wnd()); }

    void initialize_window(HWND parent) {
        CSize dpi = QueryScreenDPIEx(*this);
        if (dpi.cx <= 0 || dpi.cy <= 0) {
            dpi = CSize(96, 96);
        }
        int width = MulDiv(680, dpi.cx, 96);
        int height = MulDiv(460, dpi.cy, 96);
        WIN32_OP_D(Create(parent, CRect(0, 0, width, height)));
    }

    bool is_edit_mode_enabled() { return host_config_->edit_mode; }

    t_size host_notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2, t_size param2size) { return 0; }

  private:
    bool is_transparency_enabled() {
        return (host_config_->transparency > 0) || (host_config_->enable_transparency_active && host_config_->transparency_active > 0);
    }

    void update_transparency() {
        PostMessage(UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)-1);
    }

    void set_always_on_top(bool on_top) { SetWindowPos(on_top ? HWND_TOPMOST : HWND_NOTOPMOST, CRect{ 0 }, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE); }

    void insert_menu(HMENU menu, UINT id, LPCWSTR caption, bool enabled = true, bool checked = false) {
        MENUITEMINFOW mii = { 0 };
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (ULONG_PTR)this;
        if (id) {
            mii.fMask |= MIIM_ID | MIIM_STRING | MIIM_STATE;
            mii.wID = id;
            mii.fState = (enabled ? MFS_ENABLED : MFS_DISABLED) | (checked ? MFS_CHECKED : MFS_UNCHECKED);
            mii.dwTypeData = const_cast<LPWSTR>(caption);
        } else {
            mii.fType = MFT_SEPARATOR;
        }
        InsertMenuItemW(menu, SC_CLOSE, FALSE, &mii);
    }

    void cleanup_system_menu() {
        HMENU menu = GetSystemMenu(FALSE);
        do {
            int n, m = GetMenuItemCount(menu);
            for (n = 0; n < m; ++n) {
                MENUITEMINFOW mii = { 0 };
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_DATA | MIIM_SUBMENU;
                GetMenuItemInfoW(menu, n, TRUE, &mii);
                if (mii.dwItemData == (ULONG_PTR)this) {
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

    void build_context_menu(HMENU menu, bool sys_menu = true) {
        insert_menu(menu, t_menu_cmd_always_on_top, L"Always on top", true, host_config_->always_on_top);
        insert_menu(menu, t_menu_cmd_flowin_no_frame, L"No frame", true, !host_config_->show_caption);
        insert_menu(menu, t_menu_cmd_snap_to_edge, L"Snap to screen edge", true, host_config_->enable_snap);
        insert_menu(menu, t_menu_cmd_snap_auto_hide, L"Auto hide when snapped", host_config_->enable_snap, host_config_->enable_autohide_when_snapped);
        insert_menu(menu, t_menu_cmd_edit_mode, L"Edit mode", true, host_config_->edit_mode);
        insert_menu(menu, t_menu_cmd_flowin_custom_title, L"Custom Title");
        insert_menu(menu, t_menu_cmd_flowin_transparency, L"Transparency");
        insert_menu(menu, t_menu_cmd_destroy_element, L"Delete");
        if (sys_menu) {
            insert_menu(menu, 0, nullptr);
        }
    }

    void execute_context_menu(int cmd) {
        switch (cmd) {
            case t_menu_cmd_always_on_top:
                host_config_->always_on_top = !host_config_->always_on_top;
                set_always_on_top(host_config_->always_on_top);
                break;

            case t_menu_cmd_flowin_no_frame:
                host_config_->show_caption = !host_config_->show_caption;
                flowin_utils::add_or_remove_window_frame(*this, !host_config_->show_caption);
                break;

            case t_menu_cmd_snap_to_edge:
                host_config_->enable_snap = !host_config_->enable_snap;
                enable_window_snap_ = host_config_->enable_snap;
                if (!enable_window_snap_) {
                    RestoreFromSnapHidden();
                }
                break;

            case t_menu_cmd_snap_auto_hide:
                host_config_->enable_autohide_when_snapped = !host_config_->enable_autohide_when_snapped;
                enable_window_snap_auto_hide_ = host_config_->enable_autohide_when_snapped;
                if (!enable_window_snap_auto_hide_) {
                    RestoreFromSnapHidden();
                }
                break;

            case t_menu_cmd_edit_mode:
                host_config_->edit_mode = !host_config_->edit_mode;
                if (has_child()) {
                    element_inst_->notify(ui_element_notify_edit_mode_changed, 0, nullptr, 0);
                }
                break;

            case t_menu_cmd_destroy_element: {
                pfc::string8 element_name;
                uGetWindowText(*this, element_name);
                pfc::string_formatter msg;
                msg << " Delete \"" << uGetWindowText(*this).c_str() << "\"?\n You cannot restore it when deleted.";
                if (uMessageBox(*this, msg, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK) {
                    fb2k::inMainThread([this]() { flowin_core::get()->remove_flowin_host(this->host_config_->guid, true); });
                }
            } break;

            case t_menu_cmd_flowin_custom_title: {
                CCustomTitleDialog dlg(host_config_->window_title);
                if (IDOK == dlg.DoModal(*this)) {
                    ::uSetWindowText(*this, host_config_->window_title);
                }
            } break;

            case t_menu_cmd_flowin_transparency: {
                CTransparencySetDialog dlg(m_hWnd, host_config_);
                dlg.DoModal(*this);
                update_transparency();
            } break;

            default:
                break;
        }
    }

    void ajust_rect_to_primary_monitor(LPRECT rect, BOOL center = FALSE) {
        LONG ww = rect->right - rect->left;
        LONG wh = rect->bottom - rect->top;
        HMONITOR mon = MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST);
        WIN32_OP_D(mon != NULL);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        WIN32_OP_D(GetMonitorInfo(mon, &mi));
        const RECT& rc = mi.rcWork;
        if (center) {
            rect->left = rc.left + (rc.right - rc.left - ww) / 2;
            rect->top = rc.top + (rc.bottom - rc.top - wh) / 2;
            rect->right = rect->left + ww;
            rect->bottom = rect->top + wh;
        } else {
            rect->left = max(rc.left, min(rc.right - ww, rect->left));
            rect->top = max(rc.top, min(rc.bottom - wh, rect->top));
            rect->right = rect->left + ww;
            rect->bottom = rect->top + wh;
        }
    }

    void ajust_window_position(LPRECT rect = NULL, BOOL center = FALSE) {
        RECT rc;
        if (rect) {
            WIN32_OP_D(CopyRect(&rc, rect));
        } else {
            WIN32_OP_D(GetWindowRect(&rc));
        }
        ajust_rect_to_primary_monitor(&rc, center);
        WIN32_OP_D(SetWindowPos(NULL, rect, SWP_NOZORDER | SWP_NOACTIVATE));
    }

  private:
    int on_create(LPCREATESTRUCT lpcs) {
        (VOID) GetSystemMenu(FALSE);

        if (IsRectEmpty(&host_config_->window_rect)) {
            CenterWindow(core_api::get_main_window());
        } else {
            ajust_window_position(&host_config_->window_rect);
        }
        SetIcon(ui_control::get()->get_main_icon());

        if (host_config_->window_title.is_empty()) {
            g_get_name(host_config_->window_title);
        }

        ::uSetWindowText(*this, host_config_->window_title);

        if (host_config_->subelement_guid != pfc::guid_null) {
            host_replace_element(0, host_config_->subelement_guid);
        }

        set_always_on_top(host_config_->always_on_top);
        flowin_utils::add_or_remove_window_frame(*this, !host_config_->show_caption);

        // snap config
        enable_window_snap_ = host_config_->enable_snap;
        enable_window_snap_auto_hide_ = host_config_->enable_autohide_when_snapped;

        // TODO configurable?
        // remove icon from taskbar
        try {
            CoInitializeScope scope;
            ITaskbarListPtr taskbar;
            if (SUCCEEDED(taskbar.CreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER))) {
                taskbar->DeleteTab(*this);
            }
        } catch (std::exception&) {
        }

        if (is_transparency_enabled()) {
            update_transparency();
        }

        return TRUE;
    }

    void on_close() {
        ShowWindow(SW_HIDE);
        SendMessage(UWM_FLOWIN_REFRESH_CONFIG);
        SetMsgHandled(FALSE);
    }

    void on_destroy() {
        flowin_core::get()->remove_flowin_host(host_config_->guid);
        host_config_.reset();
        SetMsgHandled(FALSE);
    }

    void on_paint(CDCHandle /*dc*/) {
        CPaintDC dc(*this);
        dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
        dc.SetBkMode(TRANSPARENT);
        SelectObjectScope scope(dc, (HGDIOBJ)callback_->query_font_ex(ui_font_default));
        CRect rc;
        GetClientRect(&rc);
        dc.DrawText(_T("Click to add new element."), -1, &rc, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    BOOL on_erase_bkgnd(CDCHandle dc) {
        CRect rc;
        GetClientRect(&rc);
        CBrush brush;
        brush.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        dc.FillRect(&rc, brush);
        return TRUE;
    }

    void on_active(UINT state, BOOL /*minimized*/, CWindow /*wnd_other*/) {
        if (is_transparency_enabled()) {
            update_transparency();
        }
    }

    void on_lbutton_up(UINT flags, CPoint point) {
        if (!has_child()) {
            replace_dialog(*this, 0, pfc::guid_null);
        }
    }

    void on_context_menu(CWindow wnd, CPoint point) {
        RECT rect_client;
        GetClientRect(&rect_client);
        ClientToScreen(&rect_client);
        SetMsgHandled(FALSE);
        if (!PtInRect(&rect_client, point)) {
            return;
        }
        if (!callback_->is_edit_mode_enabled() && has_child()) {
            return;
        }
        auto inst = has_child() ? element_inst_ : ui_element_helpers::instantiate_dummy(*this, ui_element_config::g_create_empty(), callback_);
        standard_edit_context_menu(MAKELPARAM(point.x, point.y), inst, 0, *this);
        SetMsgHandled(TRUE);
    }

    void on_init_menu_popup(CMenuHandle menu, UINT idx, BOOL sys_menu) {
        if (sys_menu && menu.m_hMenu == GetSystemMenu(FALSE)) {
            cleanup_system_menu();
            build_context_menu(menu);
        }
    }

    void on_sys_command(UINT id, CPoint /*point*/) {
        execute_context_menu(id);
        SetMsgHandled(FALSE);
    }

    void on_show_window(BOOL show, UINT /*status*/) {
        if (has_child()) {
            element_inst_->notify(ui_element_notify_visibility_changed, (t_size)!!show, nullptr, 0);
        }
    }

    void on_size(UINT type, CSize size) {
        if (has_child()) {
            CRect rc;
            GetClientRect(&rc);
            ::SetWindowPos(element_inst_->get_wnd(), HWND_TOP, 0, 0, rc.Width(), rc.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
        }
    }

    LRESULT on_flowin_command(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/) {
        execute_context_menu((t_uint32)wp);
        return TRUE;
    }

    LRESULT on_refresh_config(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/) {
        GetSnapWindowRect(&host_config_->window_rect);
        if (has_child()) {
            host_config_->write_subelement(element_inst_->get_configuration());
        }
        return TRUE;
    }

    LRESULT on_active_flowin(UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/) {
        BringWindowToTop();
        return TRUE;
    }

    LRESULT on_update_transparency(UINT /*msg*/, WPARAM wp, LPARAM /*lp*/) {
        ModifyStyleEx(0, WS_EX_LAYERED);
        int transparency = (int)wp;
        BYTE alpha = 0;
        if (transparency >= 0) {
            alpha = (BYTE)(255.0 - transparency * 255.0 / 100);
        } else if (host_config_->enable_transparency_active && GetActiveWindow() == m_hWnd) {
            alpha = (BYTE)(255.0 - host_config_->transparency_active * 255.0 / 100);
        } else {
            alpha = (BYTE)(255.0 - host_config_->transparency * 255.0 / 100);
        }
        SetLayeredWindowAttributes(*this, 0, alpha, LWA_ALPHA);
        return TRUE;
    }

  private:
    // fix me. not standard impl
    ui_element_config::ptr dummy_config_;
    ui_element_instance_ptr element_inst_;
    flowin_ui_element_instance_callback_impl::ptr callback_;
    cfg_flowin_host::sp_t host_config_;
};

class ui_element_flowin_host_impl : public ui_element_impl<flowin_host> {
  public:
    bool is_user_addable() { return false; }
};

static service_factory_single_t<ui_element_flowin_host_impl> g_ui_element_dummy_impl_factory;
}  // namespace
