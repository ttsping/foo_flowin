#pragma once
#include "resource.h"

class cfg_flowin_host;

class CTransparencySetDialog : public CDialogImpl<CTransparencySetDialog>
{
public:
    enum
    {
        IDD = IDD_TRANSPARENCY
    };

    CTransparencySetDialog(HWND wnd, std::shared_ptr<cfg_flowin_host>& host_cfg) : window_(wnd), cfg_(host_cfg)
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
    std::shared_ptr<cfg_flowin_host> cfg_;
    HWND window_;
    CTrackBarCtrl track_ctrl_;
    CTrackBarCtrl track_hover_ctrl_;
    DarkMode::CHooks dark_mode_hooks_;
};