#ifndef KAS_Z80_INSN_IMPL_H
#define KAS_Z80_INSN_IMPL_H

//

#include "z80_arg_defn.h"
//#include "z80_arg_size.h"
#include "z80_formats_type.h"
//#include "z80_insn_serialize.h"

#include "z80_insn_types.h"
// #include "kas_core/core_insn.h"     // core_insn::data.back_inserter();

//#include "opc_list.h"
//#include "opc_general.h"
//#include "opc_branch.h"
//#include "opc_emit.h"
//#include "opc_cp_branch.h"

// #include "z80_arg_validate.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"
//#include "z80_insn_eval.h"
#include "z80_error_messages.h"

namespace kas::z80::opc
{

template <typename ARGS_T>
const char * z80_opcode_t::validate_args(ARGS_T& args, std::ostream *trace) const
{
    auto& val_c = defn().val_c();
    auto  val_p = val_c.begin();
    auto  cnt   = val_c.size();

    expr_fits fits;
    
    for (auto& arg : args)
    {
        // if too many args
        if (!cnt--)
            return error_msg::ERR_invalid;
        if (trace)
            *trace << " " << val_p.name() << " ";
        
        auto result = val_p->ok(arg, fits);

        if (result == expression::NO_FIT)
            return error_msg::ERR_argument;
        ++val_p;
    }

    // error if not enough args
    if (cnt)
        return error_msg::ERR_invalid;
    return {};
}


// calculate size of `opcode` given a set of args        
template <typename ARGS_T>
auto z80_opcode_t::size(ARGS_T& args, op_size_t& size, expr_fits const& fits, std::ostream *trace) const
    -> fits_result
{
    // hook into validators
    auto& val_c = defn().val_c();
    auto  val_p = val_c.begin();

    // size of base opcode
    size = opc_long ? 2 : 1;
    if (z80_arg_t::prefix)
        size += 1;

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

}
#endif
