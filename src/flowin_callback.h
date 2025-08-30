#pragma once
#include "flowin_defines.h"

class NOVTABLE cfg_flowin_callback
{
public:
    DECL_SMART_PTR(cfg_flowin_callback);
    virtual void on_cfg_pre_write() = 0;
};
