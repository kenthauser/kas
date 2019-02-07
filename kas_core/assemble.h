#ifndef KAS_CORE_ASSEMBLE_H
#define KAS_CORE_ASSEMBLE_H

#include "core_emit.h"
#include "insn_container.h"
#include "core_relax.h"
#include "parser/parser_obj.h"
#include "dwarf/dwarf_impl.h"
#include "dwarf/dwarf_frame.h"

namespace kas::core
{

struct kas_assemble
{
    using INSNS = insn_container<>;
    static inline kas_clear _c{INSNS::obj_clear};

    // perform data structure inits -- refactor if need to parameterize...
    kas_assemble()
    {
        // declare "default" sections
        // NB: put here so they are numbered 1, 2, 3
        // NB: this normalizes assembly listings
        // 
        // NB: just a convention. I can find no requirement.
        // NB: this can be safely removed. (at risk of modifying listings)
        
        core_section::get(".text");
        core_section::get(".data");
        core_section::get(".bss");
    }

    template <typename It>
    void assemble(It const& begin, It const& end, std::ostream *out = {})
    {
        // 1. assemble source code into ".text". Resolve symbols into ".bss"
        auto& text_seg  = core_section::get(".text")[0]; 
        auto& lcomm_seg = core_section::get(".bss") [0]; 
        auto  at_end    = resolve_symbols(lcomm_seg);
#if 0
        std::cout << __FUNCTION__ << ": .text = " << text_seg << std::endl;
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        auto& obj = INSNS::add(text_seg, at_end);
        assemble_src(obj.inserter(), begin, end, out);
        std::cout << "parse complete" << std::endl;
#if 0
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        // 2. combine sections according to run-time flags
        // XXX here combine sections (ie .text + .text1, .data + .bss, etc
        // XXX according to run-time flags. Then relax

        // 3. relax object code
        //do_relax(obj, &std::cout);
        do_relax(obj);
        std::cout << "relax complete" << std::endl;
#if 0
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        // 4. generate `cframe` data into a second container
        if (dwarf::df_data::size() != 0) {
            auto& cf = core_section::get(".debug_frame", SHT_PROGBITS);
            cf.set_align();
            auto& cf_obj = INSNS::add(cf[0]);
            gen_cframe(cf_obj.inserter());
            do_relax(cf_obj, &std::cout);
            std::cout << "cframe complete" << std::endl;
        }
#if 0
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        // 5. schedule `dwarf line` output for generation (after obj emit)
        if (dwarf::dl_data::size() != 0) {
            // add "end_sequence" to mark end of `dl_data` instructions 
            dwarf::dl_data::mark_end(text_seg);
            auto& dl = core_section::get(".debug_line", SHT_PROGBITS);
            auto& dw_obj = INSNS::add(dl[0]);

            // schedule generation after `text` addresses resolved
            do_gen_dwarf = &dw_obj;
        }
        std::cout << "assemble complete" << std::endl;
    }

    void emit(emit_base& e)
    {
#ifdef XXX
        auto proc_container = [&e](auto& container)
            {
                container.proc_all_frags(
                    [&e](auto& insn_iter, core_expr_dot const& dot)
                    {
                       e.emit(*insn_iter, &dot);
                    });
            };

        INSNS::for_each([&](auto& container)
            {
                if (&container == do_gen_dwarf)
                    gen_dwarf();
                proc_container(container);
            });
#endif
    }

private:
    template <typename Inserter, typename It>
    void assemble_src(Inserter inserter, It const& begin, It const& end, std::ostream *out)
    {
    
        kas::core::opcode::trace = out;
#ifdef XXX
        // create parser object
        auto stmt_stream = parser::kas_parser(kas::stmt(), std::cout);
        stmt_stream.add(begin, end, "dummy_path");

        for (auto&& stmt : stmt_stream) {
            try {
                *inserter++ = core_insn::gen_insn(stmt); 
            } catch (std::exception const& e) {
                auto exc_name = typeid(e).name();        // internal error: ugly name is fine
                if (out)
                    *out << "\n\nerr : " << exc_name << ": " << e.what() << std::endl;
            }
            if (out)
                *out  << std::endl;
        }
#endif
    }

    // don't need to forward declare static lambdas
    static constexpr auto resolve_symbols = [](core_segment& seg)
    {
        return [&seg](auto& inserter)
        {
            // Symbol processing at end of "parse".
            // 1. flag undefined "internal" symbols.
            // 2. mark undefined "referenced" symbols as GLOBL
            // 3. move lcomm symbols to bss
            auto resolve_one_symbol = [&inserter, seg=seg.index()](auto& sym) mutable
            {
                // 1. error undefined internal symbols
                if (sym.binding() == STB_INTERNAL && !sym.addr_p())
                    sym.make_error("Undefined local symbol");

                // 2. mark undefined & "referenced" symbols as GLOBAL
                if (sym.binding() == STB_UNKN)
                    sym.set_binding(STB_GLOBAL);

                // 3. convert local common to bss symbol
                if (sym.binding() != STB_GLOBAL && sym.kind() == STT_COMMON)
                {

                // put commons in specified segment (do once)
                    if (seg) {
                        *inserter++ = { opc::opc_section(), seg };
                        seg = {};
                        std::cout << "opening local common segment" << std::endl;
                    }
                    // if common specified alignment, make it so
                    if (sym.align() > 1)
                        *inserter++ = { opc::opc_align(), sym.align() };
#ifdef XXX
                    // common now Block-Starting-with-Symbol (bss)
                    std::cout << "move to bss: " << sym.name() << std::endl;
                    *inserter++ = opc::opc_label(sym.ref(), STB_LOCAL);   // picks up size from symbol 
#endif
                }
            };
            core::core_symbol::dump(std::cout);
           
            core_symbol::for_each(resolve_one_symbol);
            
            core::core_symbol::dump(std::cout);
            
        };
    };

    template <typename Inserter>
    void gen_cframe(Inserter inserter)
    {
        dwarf::dwarf_frame_gen(inserter);
    }

    void gen_dwarf()
    {
        dwarf::dwarf_gen(do_gen_dwarf->inserter());
        do_relax(*do_gen_dwarf);
        //do_relax(*do_gen_dwarf, &std::cout);
        do_gen_dwarf = {};
    }

    INSNS *do_gen_dwarf {};
};
}
#endif

