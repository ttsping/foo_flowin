#include "pch.h"
#include "flowin_vars.h"
#include "helpers/atl-misc.h"
#include "flowin_core.h"

namespace {

typedef CWinTraits<WS_POPUP, 0> CDummpyFlowinTraits;

class flowin_dummy_ui_element : public ui_element_instance, public CWindowImpl<flowin_dummy_ui_element, CWindow, CDummpyFlowinTraits> {
  public:
    DECLARE_WND_CLASS(TEXT("{A2143483-196F-45A4-81F3-621ABF577E69}"));

    BEGIN_MSG_MAP_EX(flowin_dummy_ui_element)
    END_MSG_MAP()

    flowin_dummy_ui_element(ui_element_config::ptr, ui_element_instance_callback_ptr p_callback) : callback_(p_callback) {
        flowin_core::get()->set_instance_callback(callback_);
    }

    HWND get_wnd() { return NULL; }
    void set_configuration(ui_element_config::ptr config) { config_ = config; }
    ui_element_config::ptr get_configuration() { return config_; }
    static GUID g_get_guid() { return g_dui_dummy_element_guid; }
    static GUID g_get_subclass() { return ui_element_subclass_utility; }
    static void g_get_name(pfc::string_base& out) { out = "dummp_dui_window"; }
    static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return ""; }
    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) {
        flowin_core::get()->notify(p_what, p_param1, p_param2, p_param2size);
    }

    void initialize_window(HWND parent) {
        ::ShowWindow(parent, SW_HIDE);
        ::PostMessage(parent, WM_CLOSE, 0, 0);
    }

  private:
    ui_element_config::ptr config_;
    const ui_element_instance_callback_ptr callback_;
};

class flowin_dummy_ui_element_impl : public ui_element_impl<flowin_dummy_ui_element> {
  public:
    bool is_user_addable() { return false; }
};

static service_factory_single_t<flowin_dummy_ui_element_impl> g_flowin_dummy_ui_element_impl_factory;

}


