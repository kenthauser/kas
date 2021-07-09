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

    static constexpr auto MAX_MCODE_WORDS = MCODE_T::MAX_MCODE_WORDS;

    // declare object-code format
    using code_t = std::array<mcode_size_t, MAX_MCODE_WORDS>;

    constexpr tgt_info_fn_t() {}

    virtual void insert(code_t&     code
                      , stmt_info_t const& stmt_info
                      , defn_info_t const& defn_info) const {}
    
    virtual stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const { return {}; }

    virtual code_t mask(stmt_info_t const& stmt_info
                      , defn_info_t const& defn_info) const { return {}; }
};          

}

#endif

