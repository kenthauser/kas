#ifndef KAS_BSD_CORE_OPS_DEF_H
#define KAS_BSD_CORE_OPS_DEF_H

#include "kas_core/opcode.h"
#include "kas_core/opc_misc.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/core_section.h"
#include "kas_core/core_symbol.h"

#include <ostream>

namespace kas::bsd
{
struct bsd_align : core::opc::opc_align
{
    // allow "pseudo-op" to specify alignment (eg: .even)
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(data, std::move(args), num_v[0]);
    }

    void proc_args(data_t& data, bsd_args&& args, short n = 0)
    {
        // copy location_tagged value
        parser::kas_position_tagged loc = args.front();
        if (auto result = validate_min_max(args, !n, !n))
            return make_error(data, result);

        // if not implied size (eg: .even), arg must be fixed
        if (args.size())
        {
            if (auto p = args.front().get_fixed_p())
                n = *p;
            else
                return make_error(data, "fixed alignment required", loc);
        }
        opc_align::proc_args(data, loc, n);
    }
};

struct bsd_org : core::opc::opc_org
{
    template <typename...Ts>
    void proc_args(data_t& data, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate_min_max(args, 1, 1))
            return make_error(data, result);

        opc_org::proc_args(data, args.front());
    }
};

struct bsd_skip : core::opc::opc_skip
{
    template <typename...Ts>
    void proc_args(data_t& data, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate_min_max(args, 1, 2))
            return make_error(data, result);
        
        args.emplace_back();        // add `fill` if not specified
        opc_skip::proc_args(data, args[0], args[1]);
    }
};

// front-end for `core_fixed` opcodes with BSD args
template <typename T>
struct bsd_fixed : T
{
    template <typename...Ts>
    auto validate(bsd_args& args, Ts&...)
    {
        // handle the solo "missing_arg" case
        return core::opcode::validate_min_max(args);
    }

    template <typename...Ts>
    void proc_args(core::opcode::data_t& data, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate(args))
            return core::opcode::make_error(data, result);
    
        // get per-arg processing fn 
        auto proc_fn = T::gen_proc_one(data);

        core::opcode::op_size_t size{}; 

        // process container
        for (auto& tok : args)
        {
            if (expression::tok_missing(tok))
                tok = e_diag_t::error("Missing value", tok);
            size += proc_fn(tok);
        }

        data.size = size;
    }
    
};
}


#endif
