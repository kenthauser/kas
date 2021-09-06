#ifndef KAS_CORE_ASSEMBLE_H
#define KAS_CORE_ASSEMBLE_H

#include "core_emit.h"          // forward declares `kbfd_object`
#include "insn_container.h"
#include "core_relax.h"
#include "parser/parser_obj.h"
#include "dwarf/dwarf_impl.h"
#include "dwarf/dwarf_frame.h"

#include "core_symbol.h"        // for dump

namespace kas::core
{

struct kas_assemble
{
    using INSNS = insn_container<>;
    
    // `kbfd_object` defines target format
    kas_assemble(kbfd::kbfd_object& obj) : obj(obj)
    {
        // allocate initial sections per object format
        core_section::init(obj);
    }

    void assemble(parser::parser_src& src, std::ostream *out = {})
    {
        std::cout << "parse begins" << std::endl;

        // 1. assemble source code into ".text". Resolve symbols into ".bss"
        auto& text_seg  = core_section::get_initial();
        auto& lcomm_seg = core_section::get_lcomm();

        // NB: `resolve_symbols` finds undefined symbols & local commons
        auto  at_end    = resolve_symbols(lcomm_seg);
        
        // NB: `at_end` is method to perform after all insns consumed
        auto& obj = INSNS::add(text_seg, at_end);
        assemble_src(obj.inserter(), src, out);

        std::cout << "parse complete" << std::endl;
#if 0
        kas::core::core_symbol::dump(std::cout);
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        // 2. combine sections according to run-time flags
        // XXX here combine sections (ie .text + .text1, .data + .bss, etc
        // XXX according to run-time flags. Then relax

        // 3. relax object code
        //do_relax(obj, &std::cout);
    #if 1
        // XXX need reloc info before relax...
        do_relax(obj, out);
        std::cout << "relax complete" << std::endl;
    #endif
#if 0
        kas::core::core_symbol::dump(std::cout);
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
        kas::parser::kas_diag_t::dump(std::cout);
        
    #if 1
        // 4. generate `cframe` data into a second container
        if (dwarf::df_data::size() != 0) {
            auto& cf = core_section::get(".debug_frame", SHT_PROGBITS);
            cf.set_align();
            auto& cf_obj = INSNS::add(cf);
            gen_cframe(cf_obj.inserter());
            do_relax(cf_obj, &std::cout);
            std::cout << "cframe complete" << std::endl;
        }
    #endif
#if 0
        kas::core::core_section::dump(std::cout);
        kas::core::core_segment::dump(std::cout);
        kas::core::core_fragment::dump(std::cout);
#endif
    //dwarf::dl_data::dump(std::cout);
    #if 0
        // 5. schedule `dwarf line` output for generation (after obj emit)
        //    (goes into a third container)
        //    NB: dwarf requires resolved addresses, so generate after resolved.
        if (dwarf::dl_data::size() != 0)
        {
            // add "end_sequence" to mark end of `dl_data` instructions 
            dwarf::dl_data::mark_end(text_seg);
            auto& dl = core_section::get(".debug_line", SHT_PROGBITS);
            auto& dw_obj = INSNS::add(dl);

            // schedule generation after `text` addresses resolved
            do_gen_dwarf = &dw_obj;
        }
    #endif
        core_section::for_each([&](auto& s)
            {
                // if deferred generation:
                //   1. execute `end_of_parse` method
                //   2. add container for deferred INSNs
                if (auto p = s.deferred_ops_p)
                {
                    p->end_of_parse(s);
                    auto& dw_obj = INSNS::add(s);
                    do_gen_dwarf = &dw_obj;
                    dw_obj.set_deferred_ops(*p);
                }
            });

        std::cout << "assemble complete" << std::endl;
        kas::core::core_symbol_t::dump(std::cout);
    }
#if 1
    void emit(emit_stream_base& e)
    {
        // define method to emit all frags in container...
        auto proc_container = [&e](auto& container)
            {
                container.proc_all_frags(
                    [&e](auto& insn, core_expr_dot const& dot)
                    {
                        e.emit(insn, &dot);
                    });
            };
#if 0
        // 1. initialize base (and stream) with `kbfd_object`
        e.init(obj);
#endif
        // 2. always rewind to initial section (normally ".text")
        e.set_segment(core_section::get_initial());
       
        // 3. emit all insns for all containers
        INSNS::for_each([&](auto& container)
            {
                if (&container == do_gen_dwarf)
                    gen_dwarf();
                if (auto p = container.deferred_ops_p)
                {
                    std::cout << "Container::deferred: " << container.index() << std::endl;
                    auto inserter = container.inserter();
                    p->set_inserter(inserter);
                    if (p->gen_data())
                        std::cout << "Container::deferred: need relax" << std::endl;
                }
#if 0
                proc_container(container);
#else
                container.proc_all_frags(
                    [&e](auto& insn, core_expr_dot const& dot)
                    {
                        e.emit(insn, &dot);
                    });
#endif
            });
#if 0
         // emit complete. close stream.
         e.close();
#endif
    }
#else
    void emit(core_emit& e)
    {
        // define method to emit all frags in container...
        auto proc_container = [&e](auto& container)
            {
                container.proc_all_frags(
                    [&e](auto& insn, core_expr_dot const& dot)
                    {
                        e.emit(insn, &dot);
                    });
            };

        // 1. initialize base (and stream) with `kbfd_object`
        e.init(obj);

        // 2. always rewind to initial section (normally ".text")
        e.set_segment(core_section::get_initial());
       
        // 3. emit all insns for all containers
        INSNS::for_each([&](auto& container)
            {
                if (&container == do_gen_dwarf)
                    gen_dwarf();
                proc_container(container);
            });

         // emit complete. close stream.
         e.close();
    }

#endif
private:
    template <typename Inserter>
    void assemble_src(Inserter inserter, parser::parser_src& src, std::ostream *out)
    {
    
        kas::core::opcode::trace = out;

        // trace source file operations
        src.set_trace(out);
 
        // create parser object
        auto stmt_stream = parser::kas_parser(parser::stmt_x3(), src);
         
        // `stmt_stream` iterator returns objects, not references
        // `stmt_stream` checks syntax, not semantics
        for (auto&& stmt : stmt_stream)
        {
            if (out)
            {
                *out << "in :  " << stmt.src() << std::endl;
                *out << "out:  " << stmt       << std::endl;
            }
        
            try
            {
                // check parsed stmt for proper semantics
                auto&& insn = stmt();
                if (out)
                {
                    *out << "raw:  " << insn.raw() << std::endl;
                    *out << "fmt:  " << insn.fmt() << std::endl;
                }
                *inserter++ = std::move(insn);
            } 
            
            catch (std::exception const& e)
            {
                // internal error: ugly name is fine
                auto exc_name = typeid(e).name(); 
            
                // print diagnostic message
                std::ostream& diag = out ? *out : std::cout; 
                diag << "\n\nInternal error: " << exc_name << ": " << e.what() << std::endl;
                diag << "while processing: " << stmt.src() << std::endl;
            }
            
            if (out)
                *out  << std::endl;
        }
    }

    // NB: don't need to forward declare `static` lambdas
    static constexpr auto resolve_symbols = [](core_segment& seg)
    {
        return [&seg](auto& inserter)
        {
            // Symbol processing at end of "parse".
            // 1. error undefined "internal" symbols.
            // 2. mark undefined "referenced" symbols as GLOBL
            // 3. move lcomm symbols to bss
            auto resolve_one_symbol = [&inserter, seg_index=seg.index()](auto& sym) mutable
            {
                // 1. error undefined internal symbols
                if (sym.binding() == STB_INTERNAL && !sym.addr_p())
                    sym.make_error("Undefined local symbol");

                // 2. mark undefined & "referenced" symbols as GLOBAL
                // XXX wrong...
                else if (sym.binding() == STB_TOKEN)
                    sym.set_binding(STB_UNKN);

                if (sym.binding() == STB_UNKN)
                    sym.set_binding(STB_GLOBAL);
                
                // 3. convert local common to bss symbol
                if (sym.binding() != STB_GLOBAL && sym.kind() == STT_COMMON)
                {
                    // put commons in specified segment (do once)
                    // NB: do in loop so as not to create `.bss` data unless used
                    if (seg_index)
                    {
                        *inserter++ = { opc::opc_segment(), seg_index };
                        seg_index = {};     // don't repeat
                    }
                    
                    // if common symbol specified alignment, make it so
                    if (sym.align() > 1)
                        *inserter++ = { opc::opc_align(), sym.align() };
                    
                    // local commons: move to Block-Starting-with-Symbol (bss)
                    // pick up size from symbol  
                    *inserter++ = { opc::opc_label(), sym.ref(), STB_LOCAL };
                }
            };
           
            core_symbol_t::for_each(resolve_one_symbol);
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
        //do_relax(*do_gen_dwarf);
        do_relax(*do_gen_dwarf, &std::cout);
        do_gen_dwarf = {};
    }

    INSNS *do_gen_dwarf {};
    kbfd::kbfd_object& obj;
    
    // test fixture support
    static inline kas_clear _c{INSNS::obj_clear};
};
}
#endif

