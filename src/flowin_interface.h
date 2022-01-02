#pragma once
#include <oaidl.h>

#pragma warning(disable:4467)

[module(name = "foo_flowin")];

[
    object,
    dual,
    pointer_default(unique),
    uuid("952d6250-1f3e-4b7e-b4d0-1aef6a94bb40")
]
__interface IFlowinHost : IDispatch
{
    [propget] STDMETHOD(Left)([out, retval]INT* pp);
    [propget] STDMETHOD(Top)([out, retval]INT* pp);
    [propget] STDMETHOD(Width)([out, retval]INT* pp);
    [propget] STDMETHOD(Height)([out, retval]INT* pp);

    [propput] STDMETHOD(Left)(INT p);
    [propput] STDMETHOD(Top)(INT p);
    [propput] STDMETHOD(Width)(INT p);
    [propput] STDMETHOD(Height)(INT p);

    [propget] STDMETHOD(Show)([out, retval]VARIANT_BOOL* pp);
    [propget] STDMETHOD(AlwaysOnTop)([out, retval]VARIANT_BOOL* pp);
    [propget] STDMETHOD(NoFrame)([out, retval]VARIANT_BOOL* pp);
    [propget] STDMETHOD(SnapToEdge)([out, retval]VARIANT_BOOL* pp);
    [propget] STDMETHOD(AutoHideWhenSnap)([out, retval]VARIANT_BOOL* pp);
    [propget] STDMETHOD(Title)([out, retval]BSTR* pp);

    [propput] STDMETHOD(Show)(VARIANT_BOOL p);
    [propput] STDMETHOD(AlwaysOnTop)(VARIANT_BOOL p);
    [propput] STDMETHOD(NoFrame)(VARIANT_BOOL p);
    [propput] STDMETHOD(SnapToEdge)(VARIANT_BOOL p);
    [propput] STDMETHOD(AutoHideWhenSnap)(VARIANT_BOOL p);
    [propput] STDMETHOD(Title)(BSTR p);

    STDMETHOD(Move)(INT x, INT y, [defaultvalue(-1)]INT width, [defaultvalue(-1)]INT height);
};

[
    object,
    dual,
    pointer_default(unique),
    uuid("7eb7b0ed-8a1a-484e-a141-50d698a78fc0")
]
__interface IFlowinControl : IDispatch
{
    STDMETHOD(FindById)(UINT window_id, [out, retval]IFlowinHost** pp);
    STDMETHOD(FindByName)(BSTR window_title, [out, retval]IFlowinHost** pp);
    STDMETHOD(FindByGuid)(BSTR host_guid, [out, retval]IFlowinHost** pp);
};