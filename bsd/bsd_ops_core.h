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
struct bsd_align :  bsd_opcode
{
    static inline opc_align base_op;


    // allow "pseudo-op" to specify alignment (eg: .even)
    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        proc_args(data, std::move(args), num_v[0]);
    }

    void proc_args(data_t& data, bsd_args&& args, short n = 0) const
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
        base_op.proc_args(data, loc, n);
    }

    opcode const& op() const override
    {
        return base_op;
    }
};

struct bsd_org : bsd_opcode
{
    static inline opc_org base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        if (auto result = validate_min_max(args, 1, 1))
            return make_error(data, result);

        base_op.proc_args(data, args.front());
    }

    opcode const& op() const override
    {
        return base_op;
    }
};

struct bsd_skip : bsd_opcode
{
    static inline opc_skip base_op;

    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        if (auto result = validate_min_max(args, 1, 2))
            return make_error(data, result);
        
        args.emplace_back();        // add `fill` if not specified
        base_op.proc_args(data, args[0], args[1]);
    }

    opcode const& op() const override
    {
        return base_op;
    }
};

// front-end for `core_fixed` opcodes with BSD args
template <typename T>
struct bsd_fixed : bsd_opcode
{
    static inline T base_op;

    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        // require at least one argument to be well formed
        if (auto result = validate_min_max(args, 1))
            return make_error(data, result);
        
        // XXX need better method...
        using hook_t = std::true_type;
        if constexpr (!std::is_void_v<meta::_t<hook_t>>)
#if 1
            //hook_t()(*this);
            print_type_name{"bsd_fixed::hook"}.name<hook_t>();
#else
            arm::arm_data_seg();
#endif
        // get per-arg processing fn 
        auto proc_fn = base_op.gen_proc_one(data);

        // process container
        op_size_t size{}; 
        for (auto& tok : args)
        {
            // tokens hold both location (for error) and value
            // interpret `token` into core types
            if (auto p = tok.get_fixed_p())
                size += proc_fn(tok, *p);
#if 1
            else if (auto p = expression::tok_float(tok))
                size += proc_fn(tok, p(&tok)->value());
#endif
            else if (auto p = expression::tok_string(tok))
                size += proc_fn(tok, p(&tok)->value());
            else if (expression::tok_missing(tok))
            {
                expr_t e = e_diag_t::error("Missing value", tok);
                size += proc_fn(tok, e);
            }
            else
                size += proc_fn(tok, tok.expr());
        }

        data.size = size;
    }

    opcode const& op() const override
    {
        return base_op;
    }
};
}


#endif
