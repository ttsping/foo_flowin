#pragma once
#include "flowin_defines.h"

class NOVTABLE CfgFlowinCallback
{
public:
    virtual void OnCfgPreWrite() = 0;
};
