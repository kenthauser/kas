#ifndef KAS_CORE_ASSEMBLE_H
#define KAS_CORE_ASSEMBLE_H

#include "core_emit.h"          // forward declares `kbfd_object`
#include "insn_container.h"
#include "core_relax.h"
#include "parser/parser_obj.h"

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

#define DUMP_AFTER_PARSE
#ifdef DUMP_AFTER_PARSE
        INSNS::for_each([&](auto& container)
            {
                auto& parse_out = std::cout;

                container.proc_all_frags(
                    [&parse_out](auto& insn, auto& dot)
                    {
                        auto& loc = insn.loc();
                        parse_out << "loc : " << loc.get() << std::endl;
                        if (loc)
                        {
                            auto where = loc.where();
                            parse_out << "in  : " << where << std::endl;
                        }
                        parse_out << "raw : " << insn.raw() << std::endl;
                        parse_out << "fmt : " << insn.fmt() << '\n' << std::endl;
                    });
            });

        std::cout << "insn dump complete" << std::endl;
#endif
        
        // 2. combine sections according to run-time flags
        // XXX here combine sections (ie .text + .text1, .data + .bss, etc
        // XXX according to run-time flags. Then relax.

        // 3. relax object code
        do_relax(obj, out);
        std::cout << "relax complete" << std::endl;
        
        kas::parser::kas_diag_t::dump(std::cout);
        
        // schedule sections with deferred generation for output
        core_section::for_each([](auto& s)
            {
                // if deferred generation:
                //   1. execute `end_of_parse` method
                //   2. if required, add container for deferred INSNs
                if (auto p = s.deferred_ops_p)
                    if (p->end_of_parse(s))
                        INSNS::add(s).set_deferred_ops(*p);
            });

        std::cout << "assemble complete" << std::endl;
        kas::core::core_symbol_t::dump(std::cout);
    }

    void emit(emit_stream_base& e)
    {
        // 1. rewind to initial section (normally ".text")
        e.set_segment(core_section::get_initial());
       
        // 2. emit all insns for all containers
        INSNS::for_each([&](auto& container)
            {
                // if deferred generation, exec `gen_data` when first seen
                if (auto& p = container.deferred_ops_p)
                {
                    // generate data & relax container
                    p->gen_data(container.inserter());
                    do_relax(container, &std::cout);
                    p = {};     // don't generate again
                }
                
                // emit all insns in container
                container.proc_all_frags(
                    [&e](auto& insn, core_expr_dot const& dot)
                    {
                        e.emit(insn, &dot);
                    });
            });
    }

private:
    template <typename Inserter>
    void assemble_src(Inserter&& inserter, parser::parser_src& src, std::ostream *out)
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
   

    kbfd::kbfd_object& obj;
    
    // test fixture support
    static inline kas_clear _c{INSNS::obj_clear};
};
}
#endif

