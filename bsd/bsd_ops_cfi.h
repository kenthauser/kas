#ifndef KAS_BSD_OPS_CLI_H
#define KAS_BSD_OPS_CLI_H

#include "kas_core/opc_symbol.h"
#include "kas_core/opc_dw_frame.h"
#include "bsd_stmt.h"

#include <ostream>

namespace kas::bsd
{

struct bsd_cfi_sections : bsd_opcode
{
    OPC_INDEX();

    const char *name() const override { return "CFI_SECTIONS"; }

    // specify CFI sections to be emitted: [.eh_frame | .eh_frame_entry] [.debug_frame]
    //
    // default is `.eh_frame`

    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        // single value sets ELF name
        if (auto err = opcode::validate_min_max(args, 1, 2))
            return make_error(data, err);

        // grab first arg
        auto& e = args.front();
#if 0
        if (auto p = e.template get_p<core::symbol_ref>())
        {
            auto& sym = p->get();
            //sym.set_flags(core::ST_TEMP);     // don't define as symbol just yet
        }
#endif
    }
    
    core::opc::opcode const& op() const override
    {
        return *this;
    }
};


struct bsd_cfi_startproc : bsd_opcode
{
    static inline core::opc::opc_df_startproc base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        // optional arg "simple" omits prologue
        if (auto err = opcode::validate_min_max(args, 0, 1))
            return make_error(data, err);
        bool omit_prologue = false;
        if (!args.empty())
        {
            // convert iter to ptr
            auto p = &*args.front().begin();
            if (p && !strcmp(p, "simple"))
                omit_prologue = true;
            else
                return make_error(data
                                , "Invalid argument: expected \"simple\""
                                , args.front()
                                );
        }

        base_op.proc_args(data, args.front(), omit_prologue);
    }
    
    core::opc::opcode const& op() const override
    {
        return base_op;
    }

};

struct bsd_cfi_endproc : bsd_opcode
{
    static inline core::opc::opc_df_endproc base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        ::kas::parser::kas_position_tagged loc = args.front();
        if (auto err = opcode::validate_min_max(args, 0))
            return make_error(data, err);
        base_op.proc_args(data, loc);
    }


    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};

struct bsd_cfi_oper : bsd_opcode
{
    static inline core::opc::opc_df_oper base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        proc_args(data, std::move(args), num_v[0], num_v[1]);
    }

    // don't inline common routine
    void proc_args(data_t& data, bsd_args&& args, uint32_t cmd, uint32_t num_args) const;
    
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

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};


void bsd_cfi_oper::proc_args(data_t& data, bsd_args&& args, uint32_t cmd, uint32_t num_args) const
{
    // currently
    if (auto err = opcode::validate_min_max(args, num_args, num_args))
        return make_error(data, err);
    uint32_t arg1{}, arg2{};

    auto arg_p = args.begin();
    if (auto p = get_int_reg(*arg_p, arg1))
        return make_error(data, p, *arg_p);

    if (auto p = get_int_reg(*++arg_p, arg2))
        return make_error(data, p, *arg_p);

    base_op.proc_args(data, cmd, arg1, arg2);
}

struct bsd_cfi_undef: bsd_opcode
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
    static inline core::opc::opc_dw_file base_op;


    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        auto cmd = str_v[0];
        std::cout << "warning: " << cmd << " undefined " << std::endl;
        // // single value sets ELF name
        // if (opcode::validate_min_max(args, 1, 1))
        //     return opc_sym_file::proc_args(di, std::move(args.front()));
        // make_error("DWARF file");
    }

    core::opc::opcode const& op() const override
    {
        return *this;
    }
};
}
#endif

