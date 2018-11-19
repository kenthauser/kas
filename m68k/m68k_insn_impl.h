#ifndef KAS_M68K_INSN_IMPL_H
#define KAS_M68K_INSN_IMPL_H

//

#include "m68k_arg_defn.h"
#include "m68k_arg_size.h"
#include "m68k_formats_type.h"
//#include "m68k_insn_serialize.h"

#include "m68k_insn_types.h"
// #include "kas_core/core_insn.h"     // core_insn::data.back_inserter();

//#include "opc_list.h"
//#include "opc_general.h"
//#include "opc_branch.h"
//#include "opc_emit.h"
//#include "opc_cp_branch.h"

// #include "m68k_arg_validate.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"
//#include "m68k_insn_eval.h"
#include "m68k_error_messages.h"

namespace kas::m68k::opc
{
#if 0
template <typename ARGS_T>
parser::tagged_msg validate_arg_modes(
                        m68k_insn_t const& insn
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
    auto sz = insn.opcodes.front()->sz();
    
    // check all args are OK
    for (auto& arg : args)
    {
        if (auto msg = arg.ok_for_target(sz))
            return { msg, arg };
        if (args_are_const)
            if (!arg.is_const())
                args_are_const = false;
    }
    
    return {};
}

// templated definition to cut down on noise in `insn_t` defn
template <typename...Ts>
parser::tagged_msg m68k_insn_t::validate_args(Ts&&...args) const
{
    return validate_arg_modes(*this, std::forward<Ts>(args)...);
}
#endif

template <typename ARGS_T>
const char *m68k_opcode_t::validate_args(ARGS_T& args, std::ostream *trace) const
{
    auto& val_c = defn().val_c();
    auto  val_p = val_c.arg_index.begin();
    auto  cnt   = val_c.arg_count;

    expr_fits fits;
    
    for (auto& arg : args)
    {
        // if too many args
        if (!cnt--)
            return error_msg::ERR_invalid;
       
        if (trace)
            *trace << " " << val_c.names_base[*val_p - 1] << " ";
        
        auto result = val_c.vals_base[*val_p - 1]->ok(arg, sz(), fits);
        if (result == expression::NO_FIT)
            return error_msg::ERR_argument;
        ++val_p;
    }

    // error if not enough args
    return cnt ? error_msg::ERR_invalid : nullptr;
}


// calculate size of `opcode` given a set of args        
template <typename ARGS_T>
auto m68k_opcode_t::size(ARGS_T& args, op_size_t& size, expr_fits const& fits, std::ostream *trace) const
    -> fits_result
{
    // hook into validators
    auto& val_c = defn().val_c();
    auto  val_p = val_c.arg_index.begin();

    // size of base opcode
    size = opc_long ? 4 : 2;

    // here know val cnt matches
    auto result = fits.yes;
    for (auto& arg : args)
    {
        if (result == expression::NO_FIT)
            break;

        if (trace)
            *trace << " " << val_c.names_base[*val_p - 1] << " ";

        
        //switch(val_c.vals_base[*val_p - 1]->size(arg, sz(), fits, &size))
        auto r = val_c.vals_base[*val_p - 1]->size(arg, sz(), fits, &size);
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

}
#endif
