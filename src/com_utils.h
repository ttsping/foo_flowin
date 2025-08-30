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

class name_to_id_cache
{
public:
    typedef ULONG hash_type;

    bool lookup(hash_type hash, DISPID* p_dispid) const
    {
        DISPID dispId;
        if (!m_map.query(hash, dispId))
            return false;
        (*p_dispid) = dispId;
        return true;
    }

    inline void add(hash_type hash, DISPID dispid)
    {
        m_map[hash] = dispid;
    }

    static hash_type g_hash(const wchar_t* name)
    {
        return LHashValOfName(LANG_NEUTRAL, name);
    }

protected:
    typedef pfc::map_t<hash_type, DISPID> name_to_id_map;
    name_to_id_map m_map;
};

class type_info_cache
{
public:
    type_info_cache() : type_info_(nullptr)
    {
    }

    inline void set_type_info(ITypeInfo* type_info)
    {
        type_info_ = type_info;
    }

    inline bool is_valid() const
    {
        return type_info_ != nullptr;
    }

    inline bool is_empty() const
    {
        return !is_valid();
    }

    inline ITypeInfo* get_ptr() throw()
    {
        return type_info_;
    }

    void init_from_typelib(ITypeLib* type_lib_ptr, const GUID& guid)
    {
        type_lib_ptr->GetTypeInfoOfGuid(guid, &type_info_);
    }

public:
    HRESULT GetTypeInfo(UINT i, LCID lcid, ITypeInfo** ppv)
    {
        if (is_empty())
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

        type_info_->AddRef();
        *ppv = type_info_.GetInterfacePtr();
        return S_OK;
    }

    HRESULT GetIDsOfNames(LPOLESTR* names, UINT cnames, MEMBERID* memid)
    {
        if (is_empty())
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
            auto hash = name_to_id_cache::g_hash(names[i]);
            if (!m_cache.lookup(hash, &memid[i]))
            {
                hr = type_info_->GetIDsOfNames(&names[i], 1, &memid[i]);
                if (SUCCEEDED(hr))
                {
                    m_cache.add(hash, memid[i]);
                }
            }
        }
        return hr;
    }

    HRESULT Invoke(PVOID ins, MEMBERID memid, WORD flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO* excep_info,
                   UINT* err)
    {
        if (is_empty())
        {
            return E_UNEXPECTED;
        }

        return type_info_->Invoke(ins, memid, flags, params, result, excep_info, err);
    }

protected:
    ITypeInfoPtr type_info_;
    name_to_id_cache m_cache;
};

//-- IDispatch --
template <class T> class MyIDispatchImpl : public T
{
protected:
    static type_info_cache g_type_info_cache;

    MyIDispatchImpl<T>()
    {
        extern ITypeLibPtr g_typelib;
        if (g_type_info_cache.is_empty() && g_typelib)
        {
            g_type_info_cache.init_from_typelib(g_typelib, __uuidof(T));
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

template <class T> FOOGUIDDECL type_info_cache MyIDispatchImpl<T>::g_type_info_cache;

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

template <typename _Base, bool _AddRef = true> class com_object_impl_t : public _Base
{
private:
    volatile LONG m_dwRef;

    inline ULONG AddRef_()
    {
        return InterlockedIncrement(&m_dwRef);
    }

    inline ULONG Release_()
    {
        return InterlockedDecrement(&m_dwRef);
    }

    inline void Construct_()
    {
        m_dwRef = 0;
        if (_AddRef)
            AddRef_();
    }

    virtual ~com_object_impl_t()
    {
    }

public:
    STDMETHODIMP_(ULONG) AddRef()
    {
        return AddRef_();
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG n = Release_();
        if (n == 0)
        {
            FinalRelease();
            delete this;
        }
        return n;
    }

    TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(com_object_impl_t, _Base, { Construct_(); })
};