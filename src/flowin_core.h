#pragma once
#include <memory>
#include <map>
#include "flowin_callback.h"

class FlowinCore : public CfgFlowinCallback
{
public:
    using Ptr = std::shared_ptr<FlowinCore>;

    static Ptr Get();

    // CfgFlowinCallback
    void OnCfgPreWrite() override;

    void Initialize();
    void Finalize();

    void ShowStartupFlowin();

    void RegisterFlowin(HWND hwnd, const GUID& guid);
    void UnregisterFlowin(HWND hwnd);

    bool IsFlowinAlive(const GUID& host_guid);

    void SetLatestActiveFlowin(const GUID& host_guid);
    GUID GetLatestActiveFlowin() const;

    void SetInstanceCallback(ui_element_instance_callback_ptr p_callback)
    {
        callback = p_callback;
    }

    void Notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

    GUID GetFlowinByChild(HWND child);
    GUID GetFlowinByGuid(const wchar_t* guid);
    GUID GetFlowinByName(const wchar_t* name);

    ui_element_instance_ptr CreateFlowin(const GUID& inst_guid = pfc::guid_null);
    void RemoveFlowin(const GUID& host_guid, bool delete_config = false);

    HWND GetFlowinWindow(const GUID& host_guid);
    ui_element_instance_ptr GetFlowinInstance(const GUID& host_guid);

    BOOL PostFlowinMessage(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    LRESULT SendFlowinMessage(HWND wnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

    BOOL PostFlowinMessage(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    LRESULT SendFlowinMessage(const GUID& host_guid, UINT msg, WPARAM wp = 0, LPARAM lp = 0);

private:
    ui_element_instance_callback_ptr callback;
    service_list_t<ui_element_instance> flowin_hosts;
    ui_element_popup_host::ptr dummy_element_inst;
    GUID latest_active_flowin_guid;
    std::map<HWND, GUID> alive_flowins;
};