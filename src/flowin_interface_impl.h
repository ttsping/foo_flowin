#pragma once
#include "flowin_interface.h"
#include <comdef.h>
#include "com_utils.h"
#include "flowin_config.h"

class FlowinHostImpl : public IDispatchImpl3<IFlowinHost> {
public:
    FlowinHostImpl(GUID guid);
    virtual ~FlowinHostImpl();

    // IFlowinControl
    STDMETHODIMP get_Left(INT* p);
    STDMETHODIMP get_Top(INT* p);
    STDMETHODIMP get_Width(INT* p);
    STDMETHODIMP get_Height(INT* p);

    STDMETHODIMP get_Show(VARIANT_BOOL* pp);
    STDMETHODIMP get_AlwaysOnTop(VARIANT_BOOL* pp);
    STDMETHODIMP get_NoFrame(VARIANT_BOOL* pp);
    STDMETHODIMP get_SnapToEdge(VARIANT_BOOL* pp);
    STDMETHODIMP get_AutoHideWhenSnap(VARIANT_BOOL* pp);
    STDMETHODIMP get_Title(BSTR* pp);

    STDMETHODIMP get_NoFrameResizable(VARIANT_BOOL* pp);
    STDMETHODIMP get_NoFrameShadow(VARIANT_BOOL* pp);
    STDMETHODIMP get_NoFrameMovable(VARIANT_BOOL* pp);

    STDMETHODIMP put_Left(INT p);
    STDMETHODIMP put_Top(INT p);
    STDMETHODIMP put_Width(INT p);
    STDMETHODIMP put_Height(INT p);

    STDMETHODIMP put_Show(VARIANT_BOOL p);
    STDMETHODIMP put_AlwaysOnTop(VARIANT_BOOL p);
    STDMETHODIMP put_NoFrame(VARIANT_BOOL p);
    STDMETHODIMP put_SnapToEdge(VARIANT_BOOL p);
    STDMETHODIMP put_AutoHideWhenSnap(VARIANT_BOOL p);
    STDMETHODIMP put_Title(BSTR p);

    STDMETHODIMP put_NoFrameResizable(VARIANT_BOOL p);
    STDMETHODIMP put_NoFrameShadow(VARIANT_BOOL p);
    STDMETHODIMP put_NoFrameMovable(VARIANT_BOOL p);

    STDMETHODIMP Move(INT x, INT y, INT width, INT height);

protected:
    HWND try_get_flowin_window();

protected:
    GUID host_guid_;
    HWND host_window_;
    cfg_flowin_host::sp_t config_;
};


[
    appobject,
    coclass,
    library_block,
    default(IFlowinControl),
    uuid("1a367c90-e445-4585-88e5-9b6b73388dcb")
]
class FlowinControlImpl : public IDispatchImpl3<IFlowinControl> {
public:
    FlowinControlImpl();
    virtual ~FlowinControlImpl();

    // IFlowinControl
    STDMETHODIMP FindById(UINT window_id, IFlowinHost** pp);
    STDMETHODIMP FindByName(BSTR window_title, IFlowinHost** pp);
    STDMETHODIMP FindByGuid(BSTR host_guid, IFlowinHost** pp);
};

class FlowinControlImplFactory : public IClassFactory {
public:
    FlowinControlImplFactory();
    virtual ~FlowinControlImplFactory();

    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI(IUnknown, IClassFactory)
        COM_QI_ENTRY(IClassFactory)
    END_COM_QI_IMPL()

public:
    virtual void FinalRelease() {}
    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv);
    STDMETHODIMP LockServer(BOOL lock) { return S_OK; }

protected:
    volatile LONG ref_count_;
};