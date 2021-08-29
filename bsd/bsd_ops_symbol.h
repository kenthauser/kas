#ifndef KAS_BSD_SYMBOL_OPS_DEF_H
#define KAS_BSD_SYMBOL_OPS_DEF_H

#include "bsd_stmt.h"
#include "kas_core/opc_symbol.h"

#include "kas/defn_utils.h"
#include "utility/string_mpl.h"

#include <ostream>

namespace kas::bsd
{
struct bsd_common :  bsd_opcode
{
    static inline core::opc::opc_common base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        proc_args(data, std::move(args), num_v[0]);
    }
    
    void proc_args(data_t& data, bsd_args&& args, short binding) const
    {
        if (auto result = validate_min_max(args, 2, 3))
            return make_error(data, result);

        // validate first arg: symbol
        auto iter = args.begin();
        auto& loc = *iter;

        print_type_name{"bsd_common::proc_args::loc"}(loc);
        auto sym_p = iter++->get_p<core::core_symbol_t>();
        if (!sym_p)
            return make_error(data, "symbol name required", loc);

        // validate second arg: size
        auto size_p = iter->get_fixed_p();
        if (!size_p)
            return make_error(data, "common size must be fixed value", *iter);
        if (*size_p == 0)
            return make_error(data, "common size of zero not allowed", *iter);
        if (*size_p < 0)
            return make_error(data, "common size must be positive", *iter);

        // optional third argument is required alignment
        // must be fixed, defaults to zero
        short align{};          // default
        if (++iter != args.end())
        {
            if (auto p = iter->get_fixed_p())
                align = *p;
            else 
                return make_error(data, "alignment must be fixed", *iter);
        }
        // execute
        base_op.proc_args(data, binding, *size_p, align, *sym_p, loc);
    }

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};

struct bsd_sym_binding :  bsd_opcode
{
    static inline core::opc::opc_sym_binding base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        proc_args(data, std::move(args), num_v[0]);
    }
    
    void proc_args(data_t& data, bsd_args&& args, short binding) const
    {
        if (auto result = validate_min_max(args, 1))
            return make_error(data, result);

        // get per-arg processing fn 
        auto proc_fn = base_op.gen_proc_one(data, binding);

        for (auto& e : args) {
            //std::cout << "sym_binding: " << e << " -> " << binding << std::endl;
            proc_fn(e, e);      // pass arg & loc
        }
    }

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};


struct bsd_elf_type : bsd_opcode
{
    static inline core::opc::opc_sym_type base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        if (auto err = validate_min_max(args, 2, 2))
            return make_error(data, err);

        auto iter = args.begin();

        auto sym_p = iter->template get_p<core::symbol_ref>();
        if (!sym_p)
            return make_error(data, "symbol required", *iter);

        // several formats for symbol type:
        // eg: the following are equiv: @function, @2, STT_FUNC
        auto& loc = *++iter;
        auto value = -1;
        if (iter->is_token_type(tok_bsd_at_ident()))
            value = parser::get_symbol_type(*iter, true);
        else if (iter->is_token_type(tok_bsd_at_num()))
            value = *iter->get_fixed_p();
        else if (iter->is_token_type(tok_bsd_ident()))
            value = parser::get_symbol_type(*iter, false);

        if (value < 0)
            return make_error(data, "invalid symbol type", *iter);

        base_op.proc_args(data, value, sym_p->get(), loc);
    }

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};

struct bsd_elf_size : bsd_opcode
{
    static inline core::opc::opc_sym_size base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        if (auto err = opcode::validate_min_max(args, 2, 2))
            return make_error(data, err);
        auto iter = args.begin();

        auto sym_p = iter->template get_p<core::core_symbol_t>();
        if (!sym_p)
            return make_error(data, "symbol required", *iter);
    
        // base_t::args are `sym&, value&&, loc const&
        base_op.proc_args(data, *sym_p, iter[1].expr(), *iter);
    }

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};


}


#endif
