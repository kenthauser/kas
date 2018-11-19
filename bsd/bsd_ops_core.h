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
    void proc_args(Inserter& di, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(di, std::move(args), num_v[0]);
    }

    void proc_args(Inserter& di, bsd_args&& args, short n = 0);
};

void bsd_align::proc_args(Inserter& di, bsd_args&& args, short n)
{
    // copy location_tagged value
    kas_token loc = args.front();
    if (auto result = validate_min_max(args, !n, !n))
        return make_error(result);

    // if not implied size (eg: .even), arg must be fixed
    if (args.size()) {
        if (auto p = args.front().get_fixed_p())
            n = *p;
        else
            return make_error("fixed alignment required", loc);
    }
    opc_align::proc_args(di, loc, n);
}

struct bsd_org : core::opc::opc_org
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate_min_max(args, 1, 1))
            return make_error(result);

        kas_token loc = args.front();
        opc_org::proc_args(di, loc, std::move(args.front()));
    }
};

struct bsd_skip : core::opc::opc_skip
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate_min_max(args, 1, 2))
            return make_error(result);
        
        // XXX refactor opc_skip
        kas_token loc = args.front();
        opc_skip::proc_args(di, loc, std::move(args.front()));
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
    void proc_args(core::opcode::Inserter& di, bsd_args&& args, Ts&&...)
    {
        if (auto result = validate(args))
            return core::opcode::make_error(result);
    
        // get per-arg processing fn 
        auto proc_fn = T::gen_proc_one(di);

        core::opcode::op_size_t size{}; 

        // process container
        for (auto& e : args)
            size += proc_fn(std::move(e), e);      // pass arg & loc

        *this->size_p = size;
    }
    
};
}


#endif
