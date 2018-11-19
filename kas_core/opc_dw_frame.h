#ifndef KAS_CORE_OPC_DW_FRAME_H
#define KAS_CORE_OPC_DW_FRAME_H

#include "opcode.h"
#include "dwarf/dwarf_frame_data.h"

namespace kas::core::opc
{


struct opc_df_startproc : opcode
{
	OPC_INDEX();

    using df_data = dwarf::df_data;

	const char *name() const override { return "DW_START_FRAME"; }

    void proc_args(Inserter& di, ::kas::parser::kas_position_tagged const& loc,
                    bool omit_prologue = false)
    {
        auto p = df_data::current_frame_p();
        if (p) {
            return make_error("startproc: already in frame", loc);
        }

        auto& obj = df_data::add(omit_prologue);
        obj.set_begin(core_addr::get_dot().ref());
        fixed_p->fixed = obj.index();
	}
};

struct opc_df_endproc : opcode
{
	OPC_INDEX();

    using df_data = dwarf::df_data;

	const char *name() const override { return "DW_END_FRAME"; }

    void proc_args(Inserter& di, ::kas::parser::kas_position_tagged const& loc)
    {
        auto& p = df_data::current_frame_p();
        if (!p)
            return make_error("endproc: not in frame", loc);

        // record current address in frame. error if different section
        p->set_end(core_addr::get_dot().ref());
        fixed_p->fixed = p->index();
        p = nullptr;                // no longer in frame
	}
};

// generic dwarf_frame opcode
struct opc_df_oper: opcode
{
	OPC_INDEX();

    using df_insn_data = dwarf::df_insn_data;
    //using df_data = dwarf::dl_data;
    //using dl_pair = typename dl_data::dl_pair;

	const char *name() const override { return "DW_FRAME_CMD"; }

    void proc_args(Inserter& di, uint32_t cmd, uint32_t arg1 = {}, uint32_t arg2 = {})
    {
        // allocate a insn_data instance
        auto& obj = df_insn_data::add(cmd, arg1, arg2);
        fixed_p->fixed = obj.index();

        // XXX Need better way to set "offset". For now just allocate label
        obj.set_addr(core_addr::get_dot().ref());
	}

};
}
#endif
