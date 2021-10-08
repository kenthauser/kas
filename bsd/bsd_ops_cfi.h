#ifndef KAS_BSD_OPS_CLI_H
#define KAS_BSD_OPS_CLI_H

//
// `personality` & `lsda` are GNU extensions to support `eh_frame` unwind
// See https://binutils.sourceware.narkive.com\
//          /rIZxL70x/patch-support-cfi-personality-and-cfi-lsda-directives

#include "kas_core/opc_symbol.h"
#include "kas_core/opc_dw_frame.h"
#include "bsd_stmt.h"
#include "utility/ci_string.h"

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
    // multiple types allowed

    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        // single value sets ELF name
        if (auto err = opcode::validate_min_max(args, 1, 2))
            return make_error(data, err);

        using df_oper = core::opc::opc_df_oper;

        // case matters
        for (auto& tok : args)
        {
            auto arg = tok.src();
            if (!strcmp(arg.c_str(), ".eh_frame"))
                df_oper::gen_eh_frame();
            else if (!strcmp(arg.c_str(), ".eh_frame_entry"))
                df_oper::gen_eh_frame_entry();
            else if (!strcmp(arg.c_str(), ".debug_frame"))
                df_oper::gen_debug_frame();
            else
                return make_error(data, "X invalid argument", tok);
        }
    }

    core::opc::opcode const& op() const override
    {
        return *this;
    }
};

struct bsd_cfi_cmd : bsd_opcode
{
    static inline core::opc::opc_df_oper base_op;
    OPC_INDEX();

    void bsd_proc_args(data_t& data, bsd_args&& args
                     , short arg_c
                     , const char  **str_v
                     , short const *num_v
                     ) const override
    {
        // preamble: normalize args
        parser::kas_position_tagged loc = args.front();
        validate_min_max(args);         // clear empty list
        auto cmd = num_v[0];            // get parsed command
       
        // execute per-command specific processing
        switch (cmd)
        {
            // most commands are passed directly to `core`
            default:    break;

            // BSD startproc uses `simple` as arg to suppress `prologue`
            case dwarf::DF_startproc:
            {
                // allow no arguments or single argument "simple"
                auto n = args.size();
                if (n == 0)
                    break;
                if (n > 1)
                    return make_error(data, "X too many arguments", args[1]);
               
                // single argument -- test for "simple"
                // use case-insenstive compare
                ci_string arg {args[0].begin(), args[0].end()};

                if (arg.compare("simple"))
                    return make_error(data, "X invalid argument", args[0]);

                // NB: `base_op` understands any arg suppresses `prologue`
                break;
            }
        }

        base_op.proc_args(data, cmd, loc, std::move(args));
    }

    core::opc::opcode const& op() const override
    {
        return base_op;
    }
};
}
#endif

