#include "pch.h"
#include "ui_transparency_settings.h"
#include "flowin_config.h"
#include "flowin_vars.h"

BOOL CTransparencySetDialog::OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/)
{
    ui_config_manager::ptr api;
    if (ui_config_manager::tryGet(api))
    {
        dark_mode_hooks.AddDialogWithControls(m_hWnd);
        dark_mode_hooks.SetDark(api->is_dark_mode());
    }
    CenterWindow(GetParent());
    track_ctrl.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY));
    track_ctrl.SetRange(0, 100);
    track_ctrl.SetPos((int)cfg->transparency);

    uButton_SetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE, cfg->enable_transparency_active);

    track_hover_ctrl.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY_HOVER));
    track_hover_ctrl.SetRange(0, 100);
    track_hover_ctrl.SetPos((int)cfg->transparency_active);
    if (!cfg->enable_transparency_active)
    {
        track_hover_ctrl.EnableWindow(FALSE);
    }
    return TRUE;
}

void CTransparencySetDialog::OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/)
{
    EndDialog(id);
}

void CTransparencySetDialog::OnEnableHoverTransparency(UINT /*code*/, int /*id*/, CWindow /*ctrl*/)
{
    cfg->enable_transparency_active = uButton_GetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE);
    track_hover_ctrl.EnableWindow(cfg->enable_transparency_active);
}

void CTransparencySetDialog::OnHScroll(UINT code, UINT /*pos*/, CTrackBarCtrl ctrl)
{
    if (code == TB_THUMBTRACK || code == TB_ENDTRACK)
    {
        int transparency = -1;
        cfg->transparency = track_ctrl.GetPos();
        cfg->transparency_active = track_hover_ctrl.GetPos();
        transparency =
            ctrl.GetDlgCtrlID() == IDC_SLIDER_TRANSPARENCY ? (int)cfg->transparency : (int)cfg->transparency_active;
        ::PostMessage(window, UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)transparency, 0);
    }
}
