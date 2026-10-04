#pragma once
#include "resource.h"

class CfgFlowinHost;

class CTransparencySetDialog : public CDialogImpl<CTransparencySetDialog>
{
public:
    enum
    {
        IDD = IDD_TRANSPARENCY
    };

    CTransparencySetDialog(HWND wnd, std::shared_ptr<CfgFlowinHost>& host_cfg) : window(wnd), cfg(host_cfg)
    {
    }

    BEGIN_MSG_MAP_EX(CTransparencySetDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
        COMMAND_ID_HANDLER_EX(IDC_CHK_TRANSPARENCY_ACTIVE, OnEnableHoverTransparency)
        MSG_WM_HSCROLL(OnHScroll)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/);
    void OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/);
    void OnEnableHoverTransparency(UINT /*code*/, int /*id*/, CWindow /*ctrl*/);
    void OnHScroll(UINT code, UINT /*pos*/, CTrackBarCtrl ctrl);

private:
    std::shared_ptr<CfgFlowinHost> cfg;
    HWND window;
    CTrackBarCtrl track_ctrl;
    CTrackBarCtrl track_hover_ctrl;
    DarkMode::CHooks dark_mode_hooks;
};