#pragma once
#include "resource.h"

class CCustomTitleDialog : public CDialogImpl<CCustomTitleDialog>
{
public:
    enum
    {
        IDD = IDD_HOST_CUSTOM_TITLE
    };

    CCustomTitleDialog(pfc::string8& title) : title_(title)
    {
    }

    BEGIN_MSG_MAP_EX(CCustomTitleDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow /*wnd*/, LPARAM /*lp*/);
    void OnCloseCmd(UINT /*code*/, int id, CWindow /*ctrl*/);

private:
    pfc::string8& title_;
    DarkMode::CHooks dark_mode_hooks_;
};