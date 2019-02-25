#ifndef KAS_TARGET_TGT_INSN_IMPL_H
#define KAS_TARGET_TGT_INSN_IMPL_H

#include "tgt_insn.h"
#include "tgt_insn_defn.h"

namespace kas::tgt
{

template <typename INSN_T, typename ARGS_T>
parser::tagged_msg validate_arg_modes(
                        INSN_T const& insn
                      , parser::kas_position_tagged const& pos
                      , ARGS_T const& args
                      , bool& args_are_const
                      , std::ostream *trace = {}
                      )
{
#if 0
    // if no opcodes, then result is HW_TST
    if (insn.opcodes.empty())
        return { tst.name(), pos };
#endif

    // assume same "size" for all opcodes for same name
//    auto sz = insn.opcodes.front()->sz();
    
    // check all args are OK
    for (auto& arg : args)
    {
  //      if (auto msg = arg.ok_for_target(sz))
  //          return { msg, arg };
        if (args_are_const)
            if (!arg.is_const())
                args_are_const = false;
    }
    
    return {};
}

// templated definition to cut down on noise in `insn_t` defn
template <typename OPCODE_T, typename TST_T, unsigned MAX_MCODES, typename INDEX_T>
template <typename...Ts>
auto tgt_insn_t<OPCODE_T, TST_T, MAX_MCODES, INDEX_T>
    ::validate_args(Ts&&...args) const
    -> parser::tagged_msg 
{
    return validate_arg_modes(*this, std::forward<Ts>(args)...);
}


// mcode_t base implementations
template <typename S, typename D, typename A, typename E>
template <typename ARGS_T>
auto tgt_mcode_t<S, D, A, E>
        ::validate_args(ARGS_T& args, std::ostream *trace) const
     -> const char *
{
    auto& val_c = vals();
    auto  val_p = val_c.begin();
    auto  cnt   = val_c.size();

    expr_fits fits;
    
    for (auto& arg : args)
    {
        // if too many args
        if (!cnt--)
            return err_msg_t::ERR_invalid;
        if (trace)
            *trace << " " << val_p.name() << " ";
        
        auto result = val_p->ok(arg, fits);

        if (result == expression::NO_FIT)
            return err_msg_t::ERR_argument;
        ++val_p;
    }

    // error if not enough args
    if (cnt)
        return err_msg_t::ERR_invalid;
    return {};
}

// calculate size of `opcode` given a set of args        
template <typename S, typename D, typename A, typename E>
template <typename ARGS_T>
auto tgt_mcode_t<S, D, A, E>
        ::size(ARGS_T& args, op_size_t& size, expr_fits const& fits, std::ostream *trace) const
    -> fits_result
{
    // hook into validators
    auto& val_c = vals();
    auto  val_p = val_c.begin();

    // size of base opcode
    size = base_size();

    // here know val cnt matches
    auto result = fits.yes;
    for (auto& arg : args)
    {
        if (result == expression::NO_FIT)
            break;
        if (trace)
            *trace << " " << val_p.name() << " ";

        auto r = val_p->size(arg, fits, size);
        
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
template <typename S, typename D, typename A, typename E>
template <typename ARGS_T>
void tgt_mcode_t<S, D, A, E>
        ::emit(core::emit_base& base
            , mcode_size_t *op_p
            , ARGS_T&&   args
            , core::core_expr_dot const* dot_p
            ) const
{
    
}

template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>::defn()  const -> defn_t  const&
    { return defns_base[defn_index]; }

template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>::fmt()   const -> fmt_t   const&
    { return defn().fmt(); }

template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>::vals()  const -> val_c_t const&
    { return defn().vals(); }

template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>::name()  const -> std::string
    { return defn().name(); }

template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>::code_size() const -> uint8_t
{
    return defn().code_words * sizeof(mcode_size_t);
}
    
template <typename S, typename D, typename A, typename E>
auto tgt_mcode_t<S, D, A, E>
    ::code() const
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
        value >>= 8 * sizeof(mcode_size_t);
    }

    return code_data;
}

}

#endif
