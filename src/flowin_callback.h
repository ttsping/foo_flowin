#pragma once
#include "flowin_defines.h"

class NOVTABLE cfg_flowin_callback
{
public:
    virtual void on_cfg_pre_write() = 0;
};
