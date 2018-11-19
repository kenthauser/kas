#ifndef KAS_BSD_SYMBOL_OPS_DEF_H
#define KAS_BSD_SYMBOL_OPS_DEF_H

#include "bsd_insn.h"
#include "kas_core/opc_symbol.h"

#include "kas/defn_utils.h"
#include "utility/string_mpl.h"

#include <ostream>

namespace kas::bsd
{
struct bsd_common :  opc_common
{
    void proc_args(Inserter& di, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(di, std::move(args), num_v[0]);
    }
    
    void proc_args(Inserter& di, bsd_args&& args, short binding)
    {
        if (auto result = validate_min_max(args, 2, 3))
            return make_error(result);

        // validate first arg: symbol
        auto iter = args.begin();
        auto& loc = *iter;
        auto sym_ref_p  = iter++->get_p<core::symbol_ref>();
        if (!sym_ref_p)
            return make_error("symbol name required", loc);
        
        // validate second arg: size
        auto size_p = iter->get_fixed_p();
        if (!size_p)
            return make_error("common size must be fixed value", *iter);
        if (*size_p == 0)
            return make_error("common size of zero not allowed", *iter);
        if (*size_p < 0)
            return make_error("common size must be positive", *iter);

        // optional third argument is required alignment
        // must be fixed, defaults to zero
        short align{};          // default
        if (++iter != args.end()) {
            if (auto p = iter->get_fixed_p())
                align = *p;
            else 
                return make_error("alignment must be fixed", *iter);
        }
        
        // execute
        opc_common::proc_args(di, binding, *size_p, align, sym_ref_p->get(), loc);
    }
};

struct bsd_sym_binding :  opc_sym_binding
{
    void proc_args(Inserter& di, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(di, std::move(args), num_v[0]);
    }
    
    void proc_args(Inserter& di, bsd_args&& args, short binding)
    {
        if (auto result = validate_min_max(args, 1))
            return make_error(result);

        // get per-arg processing fn 
        auto proc_fn = gen_proc_one(di, binding);

        for (auto& e : args) {
            std::cout << "sym_binding: " << e << " -> " << binding << std::endl;
            proc_fn(std::move(e), e);      // pass arg & loc
        }
    }
};


struct bsd_elf_type : opc_sym_type
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        if (auto err = validate_min_max(args, 2, 2))
            return make_error(err);

        auto iter = args.begin();

        auto sym_p = iter->template get_p<core::symbol_ref>();
        if (!sym_p)
            return make_error("symbol required", *iter);

        // several formats for symbol type:
        // eg: the following are equiv: @function, @2, STT_FUNC
        auto& loc = *++iter;
        auto value = -1;
        if (iter->template get_p<token_at_ident>())
            value = parser::get_symbol_type(*iter, true);
        else if (iter->template get_p<token_at_num>())
            value = *iter->get_fixed_p();
        else if (iter->template get_p<core::symbol_ref>())
            value = parser::get_symbol_type(*iter, false);

        if (value < 0)
            return make_error("invalid symbol type", *iter);

        opc_sym_type::proc_args(di, value, sym_p->get(), loc);
    }
};

struct bsd_elf_size : opc_sym_size
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        if (auto err = opcode::validate_min_max(args, 2, 2))
            return make_error(err);
        auto iter = args.begin();

        auto sym_p = iter->template get_p<core::symbol_ref>();
        if (!sym_p)
            return make_error("symbol required", *iter);

        ++iter;
        opc_sym_size::proc_args(di, sym_p->get(), std::move(*iter), *iter);
    }
};


}


#endif
