#include "pch.h"
#include "ui_transparency_settings.h"
#include "flowin_config.h"
#include "flowin_vars.h"

BOOL CTransparencySetDialog::OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/)
{
    ui_config_manager::ptr api;
    if (ui_config_manager::tryGet(api))
    {
        dark_mode_hooks_.AddDialogWithControls(m_hWnd);
        dark_mode_hooks_.SetDark(api->is_dark_mode());
    }
    CenterWindow(GetParent());
    track_ctrl_.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY));
    track_ctrl_.SetRange(0, 100);
    track_ctrl_.SetPos((int)cfg_->transparency);

    uButton_SetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE, cfg_->enable_transparency_active);

    track_hover_ctrl_.Attach(GetDlgItem(IDC_SLIDER_TRANSPARENCY_HOVER));
    track_hover_ctrl_.SetRange(0, 100);
    track_hover_ctrl_.SetPos((int)cfg_->transparency_active);
    if (!cfg_->enable_transparency_active)
    {
        track_hover_ctrl_.EnableWindow(FALSE);
    }
    return TRUE;
}

void CTransparencySetDialog::OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/)
{
    EndDialog(id);
}

void CTransparencySetDialog::OnEnableHoverTransparency(UINT /*code*/, int /*id*/, CWindow /*ctrl*/)
{
    cfg_->enable_transparency_active = uButton_GetCheck(*this, IDC_CHK_TRANSPARENCY_ACTIVE);
    track_hover_ctrl_.EnableWindow(cfg_->enable_transparency_active);
}

void CTransparencySetDialog::OnHScroll(UINT code, UINT /*pos*/, CTrackBarCtrl ctrl)
{
    if (code == TB_THUMBTRACK)
    {
        int transparency = -1;
        cfg_->transparency = track_ctrl_.GetPos();
        cfg_->transparency_active = track_hover_ctrl_.GetPos();
        transparency =
            ctrl.GetDlgCtrlID() == IDC_SLIDER_TRANSPARENCY ? (int)cfg_->transparency : (int)cfg_->transparency_active;
        ::PostMessage(window_, UWM_FLOWIN_UPDATE_TRANSPARENCY, (WPARAM)transparency, 0);
    }
}
