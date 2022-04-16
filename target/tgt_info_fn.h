#ifndef KAS_TARGET_TGT_INFO_FN_H
#define KAS_TARGET_TGT_INFO_FN_H

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_info_fn_t
{
    // expose dependent types & values
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using stmt_info_t  = typename MCODE_T::stmt_info_t;
    using defn_info_t  = typename MCODE_T::defn_info_t;
    using code_t       = typename MCODE_T::code_t;

    constexpr tgt_info_fn_t() {}

    // insert `sz` into machine code
    virtual void insert(code_t&     code
                      , stmt_info_t const& stmt_info
                      , defn_info_t const& defn_info) const {}
    
    // extract `sz` from machine code
    virtual stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const { return {}; }

    virtual uint8_t sz(stmt_info_t const& stmt_info)   const { return {}; }

    virtual code_t mask(stmt_info_t const& stmt_info
                      , defn_info_t const& defn_info
                      , MCODE_T const&)               const { return {}; }

protected:
    // default method used by `info_fn_t`
    // also used by `stmt_info_t` to resolve size
    friend stmt_info_t;

    // allow sz < 0 to indicate error
    virtual int8_t defn_sz(defn_info_t const& defn_info) const { return {}; }
    
};          

}

#endif

