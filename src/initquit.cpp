#include "pch.h"
#include "flowin_core.h"

namespace {
class flowin_initquit : public initquit {
  public:
    void on_init() {}
    void on_quit() { 
        flowin_core::get()->finalize();
        flowin_core::get().reset();
    }
};

class flowin_init_stage : public init_stage_callback {
  public:
    void on_init_stage(t_uint32 stage) {
        if (stage == init_stages::after_ui_init) {
            flowin_core::get()->initalize();
            flowin_core::get()->show_startup_flowin();
        }
    }
};

static initquit_factory_t<flowin_initquit> g_flowin_initquit_factory;
static initquit_factory_t<flowin_init_stage> g_flowin_init_stage_factory;
}  // namespace
