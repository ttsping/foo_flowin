#include "pch.h"
#include "flowin_core.h"

namespace {

ULONG_PTR g_ctx_cookie = 0;

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
        if (stage == init_stages::before_ui_init) {
            static wchar_t debugFile[MAX_PATH]{};
            if (debugFile[0] == '\0') {
                DWORD len = GetModuleFileNameW(core_api::get_my_instance(), debugFile, MAX_PATH);
                debugFile[len] = 0;
                PathRemoveFileSpecW(debugFile);
                PathAppendW(debugFile, L".flowin_enable_script_object");
            }

            if (PathFileExistsW(debugFile)) {
                wchar_t path[MAX_PATH + 4] = {};
                (VOID) GetModuleFileNameW(core_api::get_my_instance(), path, MAX_PATH);
                ACTCTXW ctx{};
                ctx.cbSize = sizeof(ctx);
                ctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
                ctx.lpSource = path;
                ctx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
                ctx.hModule = core_api::get_my_instance();
                HANDLE h = CreateActCtxW(&ctx);
                if (h != INVALID_HANDLE_VALUE) {
                    (VOID) ActivateActCtx(h, &g_ctx_cookie);
                }
            }
        } else if (stage == init_stages::after_ui_init) {
            flowin_core::get()->initalize();
            flowin_core::get()->show_startup_flowin();
        }
    }
};

static initquit_factory_t<flowin_initquit> g_flowin_initquit_factory;
static initquit_factory_t<flowin_init_stage> g_flowin_init_stage_factory;
}  // namespace
