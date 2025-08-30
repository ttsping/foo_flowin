#include "pch.h"
#include "ui_custom_title.h"

BOOL CCustomTitleDialog::OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/)
{
    ui_config_manager::ptr api;
    if (ui_config_manager::tryGet(api))
    {
        dark_mode_hooks_.AddDialogWithControls(m_hWnd);
        dark_mode_hooks_.SetDark(api->is_dark_mode());
    }

    CenterWindow(GetParent());
    ::uSetDlgItemText(*this, IDC_EDIT_CUSTOM_TITLE, title_);
    return TRUE;
}

void CCustomTitleDialog::OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/)
{
    if (id == IDOK)
        ::uGetDlgItemText(*this, IDC_EDIT_CUSTOM_TITLE, title_);

    EndDialog(id);
}
