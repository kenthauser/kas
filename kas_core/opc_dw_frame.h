#ifndef KAS_CORE_OPC_DW_FRAME_H
#define KAS_CORE_OPC_DW_FRAME_H

#include "opcode.h"
#include "dwarf/dwarf_impl.h"
#include "dwarf/dwarf_frame.h"
#include "dwarf/dwarf_frame_data.h"

namespace kas::core::opc
{

// schedule deferrred ops
struct gen_eh_frame_ops : core_section::deferred_ops
{
    gen_eh_frame_ops()
    {
        std::cout << "gen_eh_frame_ops::ctor" << std::endl;
        auto& s = core_section::get(".eh_frame", SHT_PROGBITS);
        s.set_deferred_ops(*this);
    }

    bool end_of_parse(core_section& s) override
    {
        std::cout << "gen_eh_frame_ops::end_of_parse" << std::endl;
        // XXX ensure not in `proc` -- close if so...
        return true;    // need to generate data
    }

    void gen_data(insn_inserter_t&& inserter) override
    {
        std::cout << "gen_eh_frame_ops::gen_data" << std::endl;
        //dwarf::dwarf_frame_gen(std::move(inserter));
    }
};

struct gen_eh_frame_entry_ops : core_section::deferred_ops
{
    gen_eh_frame_entry_ops()
    {
        std::cout << "gen_eh_frame_entry_ops::ctor" << std::endl;
        auto& s = core_section::get(".eh_frame_entry", SHT_PROGBITS);
        s.set_deferred_ops(*this);
    }

    bool end_of_parse(core_section& s) override
    {
        std::cout << "gen_eh_frame_entry_ops::end_of_parse" << std::endl;
        // XXX ensure not in `proc` -- close if so...
        return true;    // need to generate data
    }

    void gen_data(insn_inserter_t&& inserter) override
    {
        std::cout << "gen_eh_frame_entry_ops::gen_data" << std::endl;
        //dwarf::dwarf_frame_gen(std::move(inserter));
    }
};

struct gen_debug_frame_ops : core_section::deferred_ops
{
    gen_debug_frame_ops()
    {
        std::cout << "gen_debug_frame_ops::ctor" << std::endl;
        auto& s = core_section::get(".debug_frame", SHT_PROGBITS);
        s.set_deferred_ops(*this);
    }

    bool end_of_parse(core_section& s) override
    {
        std::cout << "gen_debug_frame_ops::end_of_parse" << std::endl;
        // XXX ensure not in `proc` -- close if so...
        return true;    // need to generate data
    }

    void gen_data(insn_inserter_t&& inserter) override
    {
        std::cout << "gen_debug_frame_ops::gen_data" << std::endl;
        dwarf::dwarf_frame_gen(std::move(inserter));
    }
};

// generic dwarf_frame opcode
struct opc_df_oper : opcode
{
	OPC_INDEX();

    using dw_frame_data = dwarf::dw_frame_data;

	const char *name() const override { return "DW_FRAME"; }

    static void gen_eh_frame()
    {
        static gen_eh_frame_ops _;    // schedule generation of `.eh_frame`
    }
    static void gen_eh_frame_entry()
    {
        static gen_eh_frame_entry_ops _;
    }
    static void gen_debug_frame()
    {
        static gen_debug_frame_ops _;    // schedule generation of `.debug_frame`
    }

    void proc_args(data_t& data
                 , uint32_t cmd
                 , parser::kas_position_tagged const& loc
                 , std::vector<parser::kas_token>&& args)
    {
        std::cout << "opc_df_oper: cmd: " << cmd << std::endl;

        // special processing for `startproc`
        dw_frame_data::frame_info *info_p {};
        if (cmd == dwarf::DF_startproc)
        {
            // make sure between procs
            if (auto p = dw_frame_data::get_frame_p())
                if (!p->has_end())
                    return make_error(data, "X startproc without endproc", loc);

            // NB: args.size() (ignoring values) defines prologue
            // NB: case 0: emit "std" prologue
            // NB: case 1: omit prologue
            // ... future....
            info_p = &dw_frame_data::add_frame(args.size());
            args.clear();
        }
        
        // allocate a frame_data instance
        auto& obj = dw_frame_data::add(cmd, std::move(args));
        data.fixed.fixed = obj.index();

        std::cout << "opc_df_oper: index: " << data.fixed.fixed << std::endl;
        
        // special processing for `startproc` & `endproc`
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
        // can't print values -- byte queue must be read in order from beginning
	    auto& obj = dw_frame_data::get(data.fixed.fixed);
        auto& cmd = dwarf::DWARF_CMDS::value[obj.cmd];
        os << cmd.name;
    }

	// emit records `dot` in `dwarf` entry
	void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
	{
		auto& obj  = dw_frame_data::get(data.fixed.fixed);
		obj.frag_p = dot_p->frag_p;
		obj.frag_offset = dot_p->dot_offset;
	}
};
}
#endif
