#ifndef KAS_TARGET_TGT_MCODE_IMPL_H
#define KAS_TARGET_TGT_MCODE_IMPL_H

#include "tgt_mcode.h"

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
template <typename ARGS_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        size(ARGS_T& args
           , stmt_info_t const& info
           , opc::op_size_t& size
           , expr_fits const& fits
           , std::ostream *trace) const
    -> fits_result
{
    // hook into validators
    auto& val_c = vals();
    auto  val_p = val_c.begin();

    // size of base opcode
    size = derived().base_size();

    // here know val cnt matches
    auto result = fits.yes;
    for (auto& arg : args)
    {
        if (result == expression::NO_FIT)
            break;
        if (trace)
            *trace << " " << val_p.name() << " ";

        auto r = val_p->size(arg, derived(), info, fits, size);
        
        if (trace)
            *trace << +r << " ";
            
        switch (r)
        {
            case expr_fits::maybe:
                result = fits.maybe;
                break;
            case expr_fits::yes:
                break;
            case expr_fits::no:
                size = -1;
                result = fits.no;
                break;
        }
        ++val_p;
    }

    if (trace)
        *trace << " -> " << size << " result: " << result << std::endl;
    
    return result;
}

template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
template <typename ARGS_T>
void tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        emit(core::emit_base& base, ARGS_T&& args, stmt_info_t const& info) const
{
    // 0. generate base machine code data
    auto machine_code = derived().code(info);
    auto code_p       = machine_code.data();

    // 1. apply args & emit relocs as required
    // NB: matching mcodes have a validator for each arg
    
    // Insert args into machine code "base" value
    // if base code has "relocation", emit it
    auto val_iter = vals().begin();
    unsigned n = 0;
    for (auto& arg : args)
    {
        auto val_p = &*val_iter++;
        if (!fmt().insert(n, code_p, arg, val_p))
            fmt().emit_reloc(n, base, code_p, arg, val_p);
        ++n;
    }

    // 2. emit base code
    auto words = code_size()/sizeof(mcode_size_t);
    while (words--)
        base << *code_p++;

    // 3. emit arg information
    auto sz = info.sz(derived());
    for (auto& arg : args)
        arg.emit(base, sz);
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
    extract_info(mcode_size_t const *) const -> stmt_info_t
{
    return {};       // default: `no stmt_t`
}

    
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
    code(stmt_info_t const &stmt_info) const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    // split `code` into array of words
    std::array<mcode_size_t, derived_t::MAX_MCODE_WORDS> code_data {};
    auto& d = defn();

    auto value = d.code;
    auto n     = d.code_words;
    auto p     = &code_data[n];

    while(n--)
    {
        *--p = value;
        if constexpr (sizeof(mcode_size_t) < sizeof(value))
            value >>= 8 * sizeof(mcode_size_t);
        else
            value = 0;
    }
    
    return code_data;
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

