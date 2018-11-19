#ifndef KAS_CORE_OPC_DW_LINE_H
#define KAS_CORE_OPC_DW_LINE_H

#include "opcode.h"
#include "dwarf/dl_state.h"

namespace kas::core::opc
{
struct opc_dw_file : opcode
{
	// Dwarf File info is odd.
	// File index `0` is stored in symbol table as first local symbol
	// Other file indexes stored emitted in ELF `.debug_line` section
	// with directory & line# info

	OPC_INDEX();

	const char *name() const override { return "DW_FILE"; }

	void proc_args(Inserter& di, unsigned index, std::string name,
                    ::kas::parser::kas_position_tagged loc)
	{
		fixed_p->fixed = index;
		if (index == 0) {
			auto& sym = core_symbol::add(name, STB_LOCAL, STT_FILE);
#ifdef ENFORCE_FILE_FIRST_LOCAL
			auto err = sym.set_file_symbol();
			if (err)
				make_error(err, loc);
#else
            // save FILE:0 as negative of symbol::index
            fixed_p->fixed = -sym.index();
#endif
		} else {
			auto& obj = dwarf::dwarf_file::get(index);
			if (obj.name.empty())
				obj.name = name;
			else
				make_error("file index previously defined", loc);
		}
	}

	void fmt(Iter it, uint16_t cnt, std::ostream& os) override
	{
        // convert file# to signed index
        int32_t index = fixed_p->fixed;

        // if !defined FILE_SYMBOL
        if (index < 0)
        {
            auto& sym = core_symbol::get(-index);
            os << "0: SYM: " << -index << " " << sym.name();
#ifdef ENFORCE_FILE_FIRST_LOCAL
        } else if (index == 0) {
			// get "file" from symbol table
			if (auto sym_p = core_symbol::file())
				os << "0: SYM: " << sym_p->index() << " " << sym_p->name();
			else 
				os << "0: ** FILE SYMBOL NOT DEFINED **";
#endif
		} else {
			auto& obj = dwarf::dwarf_file::get(index);
			if (obj.name.empty())
				os << fixed_p->fixed << " : **undefined**";
			else
				os << fixed_p->fixed << " : " << obj.name;
		}
	}	
};

struct opc_dw_line : opcode
{
	OPC_INDEX();

    using dl_data = dwarf::dl_data;
    using dl_pair = typename dl_data::dl_pair;

	const char *name() const override { return "DW_LINE"; }

    void proc_args(Inserter& di, unsigned file, unsigned line
                  , dl_pair const *data, unsigned cnt)
    {
		auto& obj = dl_data::add(file, line, data, cnt);
		fixed_p->fixed = obj.index();
	}

	void fmt(Iter it, uint16_t cnt, std::ostream& os) override
	{
		auto& obj = dl_data::get(fixed_p->fixed);

        os << obj.line_num();
        if (obj.section())
        {
            os << " " << core::core_section::get(obj.section()) << "+";
            os << std::hex << obj.address();
        }
    }

	// emit records `dot` in the `dwarf_line` entry for use generating `.debug_line`
	void emit(Iter data, uint16_t cnt, emit_base& base, core_expr_dot const& dot) override
	{
		auto& obj     = dl_data::get(fixed_p->fixed);
		obj.section() = dot.section().index();
		obj.address() = dot.offset()();
	}
};
}
#endif
