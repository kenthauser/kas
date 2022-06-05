#ifndef KAS_TARGET_TGT_MCODE_IMPL_H
#define KAS_TARGET_TGT_MCODE_IMPL_H

#include "tgt_mcode.h"
#include "tgt_info_fn.h"

namespace kas::tgt
{
// mcode_t base implementations
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
template <typename ARGS_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        validate_mcode(ARGS_T& args
                      , stmt_info_t const& info 
                      , std::ostream *trace) const
     -> std::pair<const char *, int>
{
    if (trace)
        print(*trace);

    auto& val_c = vals();
    auto  val_p = val_c.begin();
    auto  cnt   = val_c.size();
    auto  msg   = err_msg_t::ERR_invalid;   // "invalid arguments"

    // validate mcode against parsed `info`
    if (auto p = info.ok(derived()))
        return { p, 0 };

    if (cnt == 1)
        msg = err_msg_t::ERR_argument;

    if (trace)
        *trace << " cnt = " << +cnt;

    expr_fits fits;
    auto  err_index = 1;            // index of zero reserved for `validate_mcode`

    // NB: here `args` still holds `MISSING` as first for no args
    if (args.front().is_missing())
    {
        msg = cnt ? err_msg_t::ERR_missing : nullptr;
        return { msg, err_index };
    }
   
    for (auto& arg : args)
    {
        // if too many args
        if (!cnt--)
            return { err_msg_t::ERR_too_many, err_index };
        if (trace)
            *trace << " " << val_p.name() << " ";
       
        // if invalid for info, pick up in size() method
        auto result = val_p->ok(arg, fits);

        if (result == expression::NO_FIT)
            return { msg, err_index };

        // note that 1 matched, change msg
        msg = err_msg_t::ERR_argument;
        ++val_p;
        ++err_index;
    }

    // error if not enough args
    if (cnt)
        return { err_msg_t::ERR_too_few, err_index };
    return {};
}

// calculate size of `opcode` given a set of args        
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        size(argv_t& args
           , stmt_info_t const& info
           , opc::op_size_t& size
           , expr_fits const& fits
           , std::ostream *trace) const
    -> fits_result
{
    return fmt().get_opc().do_size(derived(), args, size, fits, info);
}

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
void tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        emit(core::core_emit& base, argv_t& args, stmt_info_t const& info) const
{
    fmt().get_opc().do_emit(base, derived(), args, info);
}


template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    defn()  const -> defn_t  const&
    { return defns_base[defn_index]; }

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    fmt()   const -> fmt_t   const&
    { return defn().fmt(); }

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    vals()  const -> val_c_t const&
    { return defn().vals(); }

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    name()  const -> std::string
    { return defn().name(); }

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    code_size() const -> uint8_t
{
    return defn().code_words * sizeof(mcode_size_t);
}
    
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    extract_info(mcode_size_t const *code_p) const -> stmt_info_t
{
    // extract stmt_info from code using `info_fn_t`
    return defn().info.extract(code_p);
}

    
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    code(stmt_info_t const &stmt_info) const
    -> code_t
{
    // split `code` into array of words
    code_t code_data {};
    auto& d = defn();

    auto value = d.code;
    auto n     = d.code_words;
    auto p     = &code_data[n];

    // fill array with trailing zeros
    while(n--)
    {
        *--p = value;
        if constexpr (sizeof(mcode_size_t) < sizeof(value))
            value >>= 8 * sizeof(mcode_size_t);
        else
            value = 0;
    }
   
    // insert "stmt_info" into `code`
    d.info.insert(code_data, stmt_info);
    return code_data;
}

// trampoline functions for `defn_info` -> `tgt_info_fn` methods
template <typename MCODE_T, typename VALUE_T, unsigned FN_BITS>
auto tgt_defn_info_t<MCODE_T, VALUE_T, FN_BITS>::
    insert(code_t& code, stmt_info_t const& stmt_info) const
    -> void
{
    return defn_t::info_fns_base[fn_idx]->insert(code, stmt_info, *this);
}

template <typename MCODE_T, typename VALUE_T, unsigned FN_BITS>
auto tgt_defn_info_t<MCODE_T, VALUE_T, FN_BITS>::
    extract(mcode_size_t const *code_p) const
    -> stmt_info_t
{
    return defn_t::info_fns_base[fn_idx]->extract(code_p, *this);
}

template <typename MCODE_T, typename VALUE_T, unsigned FN_BITS>
auto tgt_defn_info_t<MCODE_T, VALUE_T, FN_BITS>::
    sz(stmt_info_t const& stmt_info) const
    -> uint8_t 
{
    return defn_t::info_fns_base[fn_idx]->sz(stmt_info, *this);
}

template <typename MCODE_T, typename VALUE_T, unsigned FN_BITS>
auto tgt_defn_info_t<MCODE_T, VALUE_T, FN_BITS>::
    mask(MCODE_T const& mcode) const
    -> code_t
{
    return defn_t::info_fns_base[fn_idx]->mask(mcode, value);
}


template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
void tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    print(std::ostream& os) const
{
#if 0
    auto& d = defn();
    os << "mcode_t:";
    os << " name: " << name();
    os << " fmt: " << fmt().name();
    //os << " vals: << 
#endif
}
}

#endif

