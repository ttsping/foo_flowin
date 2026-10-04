#pragma once
#include "resource.h"

class CfgFlowinHost;

class CNoFrameSettingsDialog : public CDialogImpl<CNoFrameSettingsDialog>
{
public:
    enum
    {
        IDD = IDD_NO_FRAME_SETTING
    };

    CNoFrameSettingsDialog(std::shared_ptr<CfgFlowinHost>& host_cfg) : cfg(host_cfg)
    {
    }

    BEGIN_MSG_MAP_EX(CNoFrameSettingsDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/);
    void OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/);

private:
    std::shared_ptr<CfgFlowinHost> cfg;
    DarkMode::CHooks dark_mode_hooks;
};
