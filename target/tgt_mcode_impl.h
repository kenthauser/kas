#ifndef KAS_TARGET_TGT_MCODE_IMPL_H
#define KAS_TARGET_TGT_MCODE_IMPL_H

#include "tgt_mcode.h"

namespace kas::tgt
{
// mcode_t base implementations
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
template <typename ARGS_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        validate_args(ARGS_T& args, std::ostream *trace) const
     -> std::pair<const char *, int>
{
    if (trace)
        print(*trace);

    auto& val_c = vals();
    auto  val_p = val_c.begin();
    auto  cnt   = val_c.size();
    auto  msg   = err_msg_t::ERR_invalid;   // "invalid arguments"

    if (cnt == 1)
        msg = err_msg_t::ERR_argument;

    if (trace)
        *trace << " cnt = " << +cnt;

    expr_fits fits;

    // NB: here `args` still holds `MISSING` as first for no args
    if (args.front().is_missing())
    {
        msg = cnt ? err_msg_t::ERR_missing : nullptr;
        return { msg, 0 };
    }
   
    int n = 0;      // doesn't conflict with missing
    for (auto& arg : args)
    {
        // if too many args
        if (!cnt--)
            return { err_msg_t::ERR_too_many, n };
        if (trace)
            *trace << " " << val_p.name() << " ";
       
        // if invalid for sz(), pick up in size() method
        auto result = val_p->ok(arg, sz(), fits);

        if (result == expression::NO_FIT)
            return { msg, n };

        // not that 1 matched, change msg
        msg = err_msg_t::ERR_argument;
        ++val_p;
        ++n;
    }

    // error if not enough args
    if (cnt)
        return { err_msg_t::ERR_too_few, n };
    return {};
}

// calculate size of `opcode` given a set of args        
template <typename MCODE_T, typename STMT_T, typename ERR_T, typename SIZE_T>
template <typename ARGS_T>
auto tgt_mcode_t<MCODE_T, STMT_T, ERR_T, SIZE_T>::
        size(ARGS_T& args, opc::op_size_t& size, expr_fits const& fits, std::ostream *trace) const
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

        auto r = val_p->size(arg, sz(), fits, size);
        
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
        emit(core::emit_base& base
            , mcode_size_t *op_p
            , ARGS_T&&   args
            , core::core_expr_dot const* dot_p
            ) const
{
    // 1. emit base code
   
    auto n = code_size()/sizeof(mcode_size_t);
    while (n--)
        base << *op_p++;

    // 2. emit args
    
    // hook into validators
    auto& val_c = defn().vals();
    auto  val_p = val_c.begin();
    auto  fits = core::core_fits(dot_p);

    for (auto& arg : args)
    {
        op_size_t size;

        // arg needs "size" to emit properly
        val_p->size(arg, sz(), fits, size);

        // arg_t not templated by MCODE_T, so pass as arg
        arg.emit(base, sz(), size());

        // next validator
        ++val_p; 
    }
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
    code() const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    std::array<mcode_size_t, derived_t::MAX_MCODE_WORDS> code_data;
    auto& d = defn();

    auto value = d.code;
    auto n     = d.code_words;
    auto p     = &code_data[n];

    while(n--)
    {
        *--p = value;
        if constexpr ((8 * sizeof(mcode_size_t)) <= sizeof (value))
            value >>= 8 * sizeof(mcode_size_t);
        else
            value = 0;
    }

    auto& sz_obj = d.sizes_base[d.sz_index];
#ifdef XXX
    if (!sz_obj.single_size)
    {
        auto& size_fn = sz_obj.size_fn;
        auto sz_code = size_fn(sz());
        std::cout << "adding code: " << std::hex << +sz_code << " sz = " << +sz() << std::endl;
        code_data[size_fn.word()] |= sz_code;
    }
#endif

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

