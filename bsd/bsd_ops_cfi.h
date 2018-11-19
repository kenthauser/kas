#ifndef KAS_BSD_OPS_CLI_H
#define KAS_BSD_OPS_CLI_H

#include "kas_core/opc_symbol.h"
#include "kas_core/opc_dw_frame.h"
#include "bsd_insn.h"

#include <ostream>

namespace kas::bsd
{

struct bsd_cfi_sections : core::opcode
{
    OPC_INDEX();

    const char *name() const override { return "CFI_SECTIONS"; }

    // specify CFI sections to be emitted: [.eh_frame | .eh_frame_entry] [.debug_frame]
    //
    // default is `.eh_frame`

    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        // single value sets ELF name
        if (auto err = opcode::validate_min_max(args, 1, 2))
            return make_error(err);

        // grab first arg
        auto& e = args.front();

        if (auto p = e.template get_p<core::symbol_ref>()) {
            auto& sym = p->get();
            //sym.set_flags(core::ST_TEMP);     // don't define as symbol just yet
        }
    }
};


struct bsd_cfi_startproc : opc_df_startproc
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        // optional arg "simple" omits prologue
        if (auto err = opcode::validate_min_max(args, 0, 1))
            return make_error(err);
        bool omit_prologue = false;
        if (!args.empty()) {
            auto p = args.front().template get_p<core::symbol_ref>();
            if (p && p->get().name() == "simple")
                omit_prologue = true;
            else
                return make_error("Invalid argument: expected \"simple\"", 
                                    args.front());
        }

        opc_df_startproc::proc_args(di, args.front(), omit_prologue);
    }

};

struct bsd_cfi_endproc : opc_df_endproc
{
    template <typename...Ts>
    void proc_args(Inserter& di, bsd_args&& args, Ts&&...)
    {
        ::kas::parser::kas_position_tagged loc = args.front();
        if (auto err = opcode::validate_min_max(args, 0))
            return make_error(err);
        opc_df_endproc::proc_args(di, loc);
    }

};

struct bsd_cfi_oper : opc_df_oper
{
    void proc_args(Inserter& di, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(di, std::move(args), num_v[0], num_v[1]);
    }

    // don't inline common routine
    void proc_args(Inserter& di, bsd_args&& args, uint32_t cmd, uint32_t num_args);
    
    template <typename ARG_T>
    static const char *get_int_reg(ARG_T const& arg, uint32_t& result)
    {
        if (auto p = arg.template get_p<e_fixed_t>())
            result = *p;
#if 0
        else if (auto p = arg.get_p<e_register_t>())
            result = *p;
#endif
        else
            return "expected register or integer";
        return nullptr;
    }
};


void bsd_cfi_oper::proc_args(Inserter& di, bsd_args&& args, uint32_t cmd, uint32_t num_args)
{
    // currently
    if (auto err = opcode::validate_min_max(args, num_args, num_args))
        return make_error(err);
    uint32_t arg1{}, arg2{};

    auto arg_p = args.begin();
    if (auto p = get_int_reg(*arg_p, arg1))
        return make_error(p, *arg_p);

    if (auto p = get_int_reg(*++arg_p, arg2))
        return make_error(p, *arg_p);

    opc_df_oper::proc_args(di, cmd, arg1, arg2);
}

struct bsd_cfi_undef: core::opcode
{
    OPC_INDEX();

    const char *name() const override { return "CFI_OPER"; }

#if 0
        , DEFN_CFI<CFI("startproc")>        // proc
        , DEFN_CFI<CFI("endproc")>          // proc
        , DEFN_CFI<CFI("fde_data")>         // compact uwind data
        , DEFN_CFI<CFI("def_cfa")>  register, offset  // use register offset
        , DEFN_CFI<CFI_CFA("register")> register // use register for CFA
        , DEFN_CFI<CFI_CFA("offset")>   offset   // use offset for CFA
        , DEFN_CFI<CFI_ADJ("cfa_offset")> delta  // add delta to CFA
        , DEFN_CFI<CFI("offset")>           // CFA = register, offset
        , DEFN_CFI<CFI("personality")>      // unwind personality & encoding
        , DEFN_CFI<CFI("personality_id")>   // compact unwinding ID
        , DEFN_CFI<CFI("lsda")> encoding, [exp] // define LSDA & encoding
        , DEFN_CFI<CFI("inline_lsda")>  [align] // LSDA data section (compact)
#endif
    void proc_args(Inserter& di, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v);
};

void bsd_cfi_undef::proc_args(Inserter& d, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
{
    auto cmd = str_v[0];
    std::cout << "warning: " << cmd << " undefined " << std::endl;
    // // single value sets ELF name
    // if (opcode::validate_min_max(args, 1, 1))
    //     return opc_sym_file::proc_args(di, std::move(args.front()));
    // make_error("DWARF file");
}
}


#endif

