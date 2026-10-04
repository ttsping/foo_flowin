//////////////////////////////////////////////////////////////////////////
// rip from WSH panel project
//////////////////////////////////////////////////////////////////////////

#pragma once

#define TO_VARIANT_BOOL(v) ((v) ? (VARIANT_TRUE) : (VARIANT_FALSE))

//-- IUnknown ---
#define BEGIN_COM_QI_IMPL()                                                                                            \
public:                                                                                                                \
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)                                                                 \
    {                                                                                                                  \
        if (!ppv)                                                                                                      \
            return E_INVALIDARG;

// C2594: ambiguous conversions
#define COM_QI_ENTRY_MULTI(Ibase, Iimpl)                                                                               \
    if (riid == __uuidof(Ibase))                                                                                       \
    {                                                                                                                  \
        *ppv = static_cast<Ibase*>(static_cast<Iimpl*>(this));                                                         \
        goto qi_entry_done;                                                                                            \
    }

#define COM_QI_ENTRY(Iimpl) COM_QI_ENTRY_MULTI(Iimpl, Iimpl);

#define END_COM_QI_IMPL()                                                                                              \
    *ppv = NULL;                                                                                                       \
    return E_NOINTERFACE;                                                                                              \
    qi_entry_done:                                                                                                     \
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();                                                                       \
    return S_OK;                                                                                                       \
    }                                                                                                                  \
                                                                                                                       \
private:

class NameToIdCache
{
public:
    typedef ULONG HashType;

    bool Lookup(HashType hash, DISPID* p_dispid) const
    {
        DISPID dispId;
        if (!ids_map.query(hash, dispId))
            return false;
        (*p_dispid) = dispId;
        return true;
    }

    inline void Add(HashType hash, DISPID dispid)
    {
        ids_map[hash] = dispid;
    }

    static HashType GHash(const wchar_t* name)
    {
        return LHashValOfName(LANG_NEUTRAL, name);
    }

protected:
    typedef pfc::map_t<HashType, DISPID> NameToIdMap;
    NameToIdMap ids_map;
};

class TypeInfoCache
{
public:
    TypeInfoCache() : type_info(nullptr)
    {
    }

    inline void SetTypeInfo(ITypeInfo* p_type_info)
    {
        type_info = p_type_info;
    }

    inline bool IsValid() const
    {
        return type_info != nullptr;
    }

    inline bool IsEmpty() const
    {
        return !IsValid();
    }

    inline ITypeInfo* GetPtr() throw()
    {
        return type_info;
    }

    void InitFromTypelib(ITypeLib* type_lib_ptr, const GUID& guid)
    {
        type_lib_ptr->GetTypeInfoOfGuid(guid, &type_info);
    }

public:
    HRESULT GetTypeInfo(UINT i, LCID lcid, ITypeInfo** ppv)
    {
        if (IsEmpty())
        {
            return E_UNEXPECTED;
        }

        if (!ppv)
        {
            return E_POINTER;
        }

        if (i != 0)
        {
            return DISP_E_BADINDEX;
        }

        type_info->AddRef();
        *ppv = type_info.GetInterfacePtr();
        return S_OK;
    }

    HRESULT GetIDsOfNames(LPOLESTR* names, UINT cnames, MEMBERID* memid)
    {
        if (IsEmpty())
        {
            return E_UNEXPECTED;
        }

        if (names == nullptr)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = S_OK;
        for (unsigned i = 0; i < cnames && SUCCEEDED(hr); ++i)
        {
            auto hash = NameToIdCache::GHash(names[i]);
            if (!type_info_cache.Lookup(hash, &memid[i]))
            {
                hr = type_info->GetIDsOfNames(&names[i], 1, &memid[i]);
                if (SUCCEEDED(hr))
                {
                    type_info_cache.Add(hash, memid[i]);
                }
            }
        }
        return hr;
    }

    HRESULT Invoke(PVOID ins, MEMBERID memid, WORD flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO* excep_info,
                   UINT* err)
    {
        if (IsEmpty())
        {
            return E_UNEXPECTED;
        }

        return type_info->Invoke(ins, memid, flags, params, result, excep_info, err);
    }

protected:
    ITypeInfoPtr type_info;
    NameToIdCache type_info_cache;
};

//-- IDispatch --
template <class T> class MyIDispatchImpl : public T
{
protected:
    static TypeInfoCache g_type_info_cache;

    MyIDispatchImpl<T>()
    {
        extern ITypeLibPtr g_typelib;
        if (g_type_info_cache.IsEmpty() && g_typelib)
        {
            g_type_info_cache.InitFromTypelib(g_typelib, __uuidof(T));
        }
    }

    virtual ~MyIDispatchImpl<T>()
    {
    }

    virtual void FinalRelease()
    {
    }

public:
    STDMETHOD(GetTypeInfoCount)(unsigned int* n)
    {
        if (!n)
            return E_INVALIDARG;
        *n = 1;
        return S_OK;
    }

    STDMETHOD(GetTypeInfo)(unsigned int i, LCID lcid, ITypeInfo** pp)
    {
        return g_type_info_cache.GetTypeInfo(i, lcid, pp);
    }

    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** names, unsigned int cnames, LCID lcid, DISPID* dispids)
    {
        return g_type_info_cache.GetIDsOfNames(names, cnames, dispids);
    }

    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flag, DISPPARAMS* params, VARIANT* result,
                      EXCEPINFO* excep, unsigned int* err)
    {
        return g_type_info_cache.Invoke(this, dispid, flag, params, result, excep, err);
    }
};

template <class T> FOOGUIDDECL TypeInfoCache MyIDispatchImpl<T>::g_type_info_cache;

template <class T> class IDispatchImpl3 : public MyIDispatchImpl<T>
{
    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
        COM_QI_ENTRY(T)
        COM_QI_ENTRY(IDispatch)
    END_COM_QI_IMPL()

protected:
    IDispatchImpl3<T>()
    {
    }

    virtual ~IDispatchImpl3<T>()
    {
    }
};

template <typename _Base, bool _AddRef = true> class ComObjectImpl : public _Base
{
private:
    volatile LONG m_dwRef;

    inline ULONG AddRefImpl()
    {
        return InterlockedIncrement(&m_dwRef);
    }

    inline ULONG ReleaseImpl()
    {
        return InterlockedDecrement(&m_dwRef);
    }

    inline void Construct()
    {
        m_dwRef = 0;
        if (_AddRef)
            AddRefImpl();
    }

    virtual ~ComObjectImpl()
    {
    }

public:
    STDMETHODIMP_(ULONG) AddRef()
    {
        return AddRefImpl();
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG n = ReleaseImpl();
        if (n == 0)
        {
            FinalRelease();
            delete this;
        }
        return n;
    }

    TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(ComObjectImpl, _Base, { Construct(); })
};