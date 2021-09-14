#ifndef KAS_CORE_OPC_DW_FRAME_H
#define KAS_CORE_OPC_DW_FRAME_H

#include "opcode.h"
#include "dwarf/dwarf_impl.h"
#include "dwarf/dwarf_frame.h"
#include "dwarf/dwarf_frame_data.h"

namespace kas::core::opc
{



// schedule deferrred ops
struct gen_debug_frame : core_section::deferred_ops
{
    gen_debug_frame()
    {
        std::cout << "gen_debug_frame::ctor" << std::endl;
        auto& s = core_section::get(".debug_frame", SHT_PROGBITS);
        s.set_deferred_ops(*this);
    }

    bool end_of_parse(core_section& s) override
    {
        std::cout << "gen_debug_frame::end_of_parse" << std::endl;
        // XXX ensure not in `proc` -- close if so...
        return true;    // need to generate data
    }

    void gen_data(insn_inserter_t&& inserter) override
    {
        std::cout << "gen_debug_frame::gen_data" << std::endl;
        dwarf::dwarf_frame_gen(std::move(inserter));
    }
};

// generic dwarf_frame opcode
struct opc_df_oper : opcode
{
	OPC_INDEX();

    using dw_frame_data = dwarf::dw_frame_data;

	const char *name() const override { return "DW_FRAME"; }

    void proc_args(data_t& data
                 , uint32_t cmd
                 , parser::kas_position_tagged const& loc
                 , std::vector<parser::kas_token>&& args)
    {
        static gen_debug_frame _;    // schedule generation of `.debug_frame`

        // special processing for `startproc`
        dw_frame_data::frame_info *info_p {};
        if (cmd == dwarf::DF_startproc)
        {
            info_p = &dw_frame_data::add_frame(args.size());
            args.clear();
        }
        
        // allocate a frame_data instance
        auto& obj = dw_frame_data::add(cmd, std::move(args));
        data.fixed.fixed = obj.index();
        
        // special processing for `startproc` & `end_proc`
        switch (cmd)
        {
            case dwarf::DF_startproc:
                info_p->set_start(data.fixed.fixed);
                break;
            case dwarf::DF_endproc:
                info_p = dw_frame_data::get_frame_p();
                if (info_p && !info_p->has_end())
                    info_p->set_end(data.fixed.fixed);
                else
                    return make_error(data, "X endproc without startproc", loc);
                break;
            default:
                break;
        }
    }
	
    void fmt(data_t const& data, std::ostream& os) const override
	{
		auto& obj = dw_frame_data::get(data.fixed.fixed);
        obj.print(os);
    }

	// emit records `dot` in `dwarf_line` entry for use generating `.debug_line`
	void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
	{
		auto& obj     = dw_frame_data::get(data.fixed.fixed);
		obj.segment() = dot_p->segment().index();
		obj.address() = dot_p->offset()();
	}
};
}
#endif
