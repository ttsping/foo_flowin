#include "pch.h"
#include "flowin_vars.h"
#include "flowin_core.h"
#include "flowin_config.h"

namespace {

class flowin_config_menu_node_command : public mainmenu_node_command {
  public:
    flowin_config_menu_node_command(t_uint32 id, cfg_flowin_host::sp_t cfg = nullptr) : cmd_(id), config_(cfg) {}
    void get_display(pfc::string_base& text, t_uint32& flags) override {
        flags = 0;
        bool is_alive = config_ ? flowin_core::get()->is_flowin_alive(config_->guid) : false;
        switch (cmd_) {
            case t_menu_cmd_new_flowin:
                text = "New flowin";
                break;
            case t_menu_cmd_show_all:
                text = "Show all";
                break;
            case t_menu_cmd_close_all:
                text = "Close all";
                break;
            case t_menu_cmd_show_flowin:
                text = "Show";
                flags = is_alive ? mainmenu_commands::flag_checked : 0;
                break;
            case t_menu_cmd_show_flowin_on_startup:
                PFC_ASSERT(config_ != NULL);
                text = "Show on startup";
                flags = config_->show_on_startup ? mainmenu_commands::flag_checked : 0;
                break;
            case t_menu_cmd_always_on_top:
                PFC_ASSERT(config_ != NULL);
                text = "Always on top";
                flags = config_->always_on_top ? mainmenu_commands::flag_checked : 0;
                if (!is_alive) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            case t_menu_cmd_flowin_no_frame:
                PFC_ASSERT(config_ != NULL);
                text = "No frame";
                flags = config_->show_caption ? 0 : mainmenu_commands::flag_checked;
                if (!is_alive) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            case t_menu_cmd_snap_to_edge:
                PFC_ASSERT(config_ != NULL);
                text = "Snap to screen edge";
                flags = config_->enable_snap ? mainmenu_commands::flag_checked : 0;
                if (!is_alive || !config_->show_caption) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            case t_menu_cmd_snap_auto_hide:
                PFC_ASSERT(config_ != NULL);
                text = "Auto hide when snapped";
                flags = config_->enable_autohide_when_snapped ? mainmenu_commands::flag_checked : 0;
                if (!is_alive || !config_->enable_snap || !config_->show_caption) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            case t_menu_cmd_edit_mode:
                PFC_ASSERT(config_ != NULL);
                text = "Edit mode";
                flags = config_->edit_mode ? mainmenu_commands::flag_checked : 0;
                if (!is_alive) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            case t_menu_cmd_destroy_element:
                text = "Delete";
                // fixme
                if (!is_alive) {
                    flags |= mainmenu_commands::flag_disabled;
                }
                break;
            default:
                break;
        }
    }
    void execute(service_ptr_t<service_base> callback) override {
        switch (cmd_) {
            case t_menu_cmd_new_flowin:
                flowin_core::get()->create_flowin_host();
                break;

            case t_menu_cmd_show_all:
                cfg_flowin::get()->enum_configuration([](cfg_flowin_host::sp_t config) {
                    if (!flowin_core::get()->is_flowin_alive(config->guid)) {
                        flowin_core::get()->create_flowin_host(config->guid);
                    } else {
                        flowin_core::get()->post_message(config->guid, UWM_FLOWIN_ACTIVE);
                    }
                });
                break;

            case t_menu_cmd_close_all:
                cfg_flowin::get()->enum_configuration(
                    [](cfg_flowin_host::sp_t config) { flowin_core::get()->post_message(config->guid, WM_CLOSE); });
                break;

            case t_menu_cmd_show_flowin:
                PFC_ASSERT(config_ != NULL);
                if (flowin_core::get()->is_flowin_alive(config_->guid)) {
                    flowin_core::get()->post_message(config_->guid, WM_CLOSE);
                } else {
                    flowin_core::get()->create_flowin_host(config_->guid);
                }
                break;

            case t_menu_cmd_show_flowin_on_startup:
                PFC_ASSERT(config_ != NULL);
                config_->show_on_startup = !config_->show_on_startup;
                break;

            case t_menu_cmd_always_on_top:
            case t_menu_cmd_flowin_no_frame:
            case t_menu_cmd_snap_to_edge:
            case t_menu_cmd_snap_auto_hide:
            case t_menu_cmd_edit_mode:
            case t_menu_cmd_destroy_element:
                PFC_ASSERT(config_ != NULL);
                flowin_core::get()->post_message(config_->guid, UWM_FLOWIN_COMMAND, (WPARAM)cmd_);
                break;

            default:
                break;
        }
    };

    GUID get_guid() override {
        static const GUID guid_dummy = { 0x52101dd0, 0x15c2, 0x4371, { 0xa4, 0x6f, 0x72, 0xd7, 0xe3, 0x3, 0x67, 0x60 } };
        t_uint32 flags;
        pfc::string8 name;
        get_display(name, flags);
        name += pfc::print_guid(config_ ? config_->guid : guid_dummy);
        return hasher_md5::get()->process_single_guid(name.get_ptr(), name.get_length());
    };

    bool get_description(pfc::string_base& out) override { return false; }

  private:
    const t_uint32 cmd_;
    cfg_flowin_host::sp_t config_;
};

class live_flowin_node_group : public mainmenu_node_group {
  public:
    live_flowin_node_group(cfg_flowin_host::sp_t cfg) : config_(cfg) {
        t_uint32 menus_id[] = { t_menu_cmd_show_flowin,  t_menu_cmd_show_flowin_on_startup, t_menu_cmd_always_on_top, t_menu_cmd_flowin_no_frame,
                                t_menu_cmd_snap_to_edge, t_menu_cmd_snap_auto_hide,         t_menu_cmd_edit_mode,     t_menu_cmd_destroy_element };
        for (t_size n = 0, m = pfc::array_size_t(menus_id); n < m; ++n) {
            auto node = fb2k::service_new<flowin_config_menu_node_command>(menus_id[n], cfg);
            menu_nodes_.push_back(std::move(node));
        }
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override {
        flags = 0;
        text = config_->window_title;
    }

    t_size get_children_count() override { return menu_nodes_.size(); }

    mainmenu_node::ptr get_child(t_size index) override { return menu_nodes_[index]; }

  private:
    cfg_flowin_host::sp_t config_;
    std::vector<mainmenu_node::ptr> menu_nodes_;
};

static t_uint32 fixed_menus_id[] = { t_menu_cmd_new_flowin, t_menu_cmd_show_all, t_menu_cmd_close_all };

class flowin_menu_group : public mainmenu_node_group {
  public:
    flowin_menu_group() {
        // fixed menus
        for (t_size n = 0, m = pfc::array_size_t(fixed_menus_id); n < m; ++n) {
            auto node = fb2k::service_new<flowin_config_menu_node_command>(fixed_menus_id[n]);
            menu_nodes_.push_back(std::move(node));
        }
        bool add_sep = false;
        cfg_flowin::get()->enum_configuration([&](cfg_flowin_host::sp_t config) {
            if (!add_sep) {
                auto sep_node = fb2k::service_new<mainmenu_node_separator>();
                menu_nodes_.push_back(std::move(sep_node));
                add_sep = true;
            }
            auto node = fb2k::service_new<live_flowin_node_group>(config);
            menu_nodes_.push_back(std::move(node));
        });
    }

    void get_display(pfc::string_base& text, t_uint32& flags) override {
        flags = 0;
        text = "Flowin";
    }

    t_size get_children_count() override { return menu_nodes_.size(); }

    mainmenu_node::ptr get_child(t_size index) override { return menu_nodes_[index]; }

  private:
    std::vector<mainmenu_node::ptr> menu_nodes_;
};

class flowin_mainmenu : public mainmenu_commands_v2 {
  public:
    t_uint32 get_command_count() override { return 1; }
    GUID get_command(t_uint32 p_index) override { return g_flowin_mainmenu_group_guid; }
    void get_name(t_uint32 p_index, pfc::string_base& p_out) override { p_out = "Flowin Menu"; }
    bool get_description(t_uint32 p_index, pfc::string_base& p_out) override {
        p_out = "Flowin menus.";
        return true;
    }
    GUID get_parent() override { return mainmenu_groups::view; }
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {}
    bool is_command_dynamic(t_uint32 index) override { return true; }
    mainmenu_node::ptr dynamic_instantiate(t_uint32 index) override { return fb2k::service_new<flowin_menu_group>(); }
    bool dynamic_execute(t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback) override {
        return __super::dynamic_execute(index, subID, callback);
    }
};

static mainmenu_commands_factory_t<flowin_mainmenu> g_flowin_mainmenu_factory;

}  // namespace