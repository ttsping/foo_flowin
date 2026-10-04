#include "pch.h"
#include "flowin_core.h"

namespace
{

ULONG_PTR g_ctx_cookie = 0;

class FlowinInitquit : public initquit
{
public:
    void on_init()
    {
    }
    void on_quit()
    {
        FlowinCore::Get()->Finalize();
        FlowinCore::Get().reset();
    }
};

class FlowinInitStage : public init_stage_callback
{
public:
    void on_init_stage(t_uint32 stage)
    {
        if (stage == init_stages::before_ui_init)
        {
            static wchar_t debugFile[MAX_PATH]{};
            if (debugFile[0] == '\0')
            {
                DWORD len = GetModuleFileNameW(core_api::get_my_instance(), debugFile, MAX_PATH);
                debugFile[len] = 0;
                PathRemoveFileSpecW(debugFile);
                PathAppendW(debugFile, L".flowin_enable_script_object");
            }

            if (PathFileExistsW(debugFile))
            {
                wchar_t path[MAX_PATH + 4] = {};
                (VOID) GetModuleFileNameW(core_api::get_my_instance(), path, MAX_PATH);
                ACTCTXW ctx{};
                ctx.cbSize = sizeof(ctx);
                ctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
                ctx.lpSource = path;
                ctx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
                ctx.hModule = core_api::get_my_instance();
                HANDLE h = CreateActCtxW(&ctx);
                if (h != INVALID_HANDLE_VALUE)
                {
                    (VOID) ActivateActCtx(h, &g_ctx_cookie);
                }
            }
        }
        else if (stage == init_stages::after_ui_init)
        {
            FlowinCore::Get()->Initialize();
            FlowinCore::Get()->ShowStartupFlowin();
        }
    }
};

static initquit_factory_t<FlowinInitquit> g_flowin_initquit_factory;
static initquit_factory_t<FlowinInitStage> g_flowin_init_stage_factory;
} // namespace
