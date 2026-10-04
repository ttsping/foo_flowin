#include "pch.h"
#include "ui_no_frame.h"
#include "flowin_config.h"

BOOL CNoFrameSettingsDialog::OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/)
{
    ui_config_manager::ptr api;
    if (ui_config_manager::tryGet(api))
    {
        dark_mode_hooks.AddDialogWithControls(m_hWnd);
        dark_mode_hooks.SetDark(api->is_dark_mode());
    }

    CenterWindow(GetParent());

    pfc::string8 tips_fmt, tips;
    uGetDlgItemText(m_hWnd, IDC_NO_FRAME_TIPS, tips_fmt);
    uReplaceString(tips, tips_fmt, tips_fmt.length(), "%s", 2, cfg->window_title, cfg->window_title.length(), false);
    uSetDlgItemText(m_hWnd, IDC_NO_FRAME_TIPS, tips);

    uButton_SetCheck(m_hWnd, IDC_RESIZABLE, cfg->cfg_frameless.resizable);
    uButton_SetCheck(m_hWnd, IDC_MOVABLE, cfg->cfg_frameless.draggable);
    
    if (cfg->cfg_frameless.legacy_no_frame)
    {
        uButton_SetCheck(m_hWnd, IDC_SHOW_SHADOW, false);
        GetDlgItem(IDC_SHOW_SHADOW).EnableWindow(FALSE);

        uButton_SetCheck(m_hWnd, IDC_ROUNDED_CORNER, false);
        GetDlgItem(IDC_ROUNDED_CORNER).EnableWindow(FALSE);
    }
    else
    {
        uButton_SetCheck(m_hWnd, IDC_SHOW_SHADOW, cfg->cfg_frameless.shadowed);
        uButton_SetCheck(m_hWnd, IDC_ROUNDED_CORNER, cfg->cfg_frameless.rounded_corner);
    }

    return TRUE;
}

void CNoFrameSettingsDialog::OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/)
{
    if (id == IDOK)
    {
        cfg->cfg_frameless.resizable = uButton_GetCheck(m_hWnd, IDC_RESIZABLE);
        cfg->cfg_frameless.draggable = uButton_GetCheck(m_hWnd, IDC_MOVABLE);
        cfg->cfg_frameless.shadowed = uButton_GetCheck(m_hWnd, IDC_SHOW_SHADOW);
        cfg->cfg_frameless.rounded_corner = uButton_GetCheck(m_hWnd, IDC_ROUNDED_CORNER);
    }

    EndDialog(id);
}
