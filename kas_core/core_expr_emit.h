#ifndef KAS_CORE_CORE_EXPR_EMIT_H
#define KAS_CORE_CORE_EXPR_EMIT_H

#include "core_expr_type.h"
#include "core_symbol.h"
#include "core_fragment.h"
#include "core_reloc.h"

namespace kas::core
{
template <typename REF>
template <typename BASE_T, typename RELOC_T>
void core_expr<REF>::emit(BASE_T& base, RELOC_T& reloc) const
{
    auto do_emit = [&base]
                    (auto& reloc, expr_term const *term_p = {}, bool pc_rel = false)
    {
        reloc.sym_p     = {};
        reloc.section_p = {};
        
        // interpret expr_term as addr, symbol, or error
        if (term_p)
        {
            if (term_p->symbol_p)
            {
                reloc.sym_p = term_p->symbol_p;
            }
            else if (term_p->addr_p)
            {
                reloc.addend   +=  term_p->addr_p->offset()();
                reloc.section_p = &term_p->addr_p->section();
            }
            else if (term_p->value_p)
            {
                // not symbol nor addr -> error term
                // if value_p is not error, create one
                // XXX loc_p can be zero
                auto p = term_p->value_p->template get_p<parser::kas_diag_t>();
                if (!p)
                    p = &parser::kas_diag_t::error("Invalid expression"
                                                 , *reloc.loc_p);
                //diag = *p;

            }
        }

        if (pc_rel)
            reloc.reloc.set  (kbfd::kbfd_reloc::RFLAGS_PC_REL);
        else
            reloc.reloc.clear(kbfd::kbfd_reloc::RFLAGS_PC_REL);
    
        reloc.emit(base);
    };

    calc_num_relocs();      // pair terms, look for error
    

    //std::cout << "core_expr<REF>::emit: " << expr_t(*this) << " fixed = " << fixed << std::endl;
    // build "new" reloc for expression
    reloc.addend      += fixed;     // acumulate `fixed`
    reloc.core_expr_p  = {};        // processing now

    // examine `minus` list to find `pc_rel` & subs
    auto section_p = &base.get_section();
    unsigned pc_rel_cnt = reloc.reloc.has(kbfd::kbfd_reloc::RFLAGS_PC_REL);
    unsigned minus_cnt  = {};

    // convert minus terms to pc-relative if possible
    for (auto& m : minus)
    {
        // if paired, ignore
        if (m.p)
            continue;
        // if minus term in current section, convert to `PC_REL` cnt
        if (m.addr_p && &m.addr_p->section() == section_p)
        {
            reloc.addend -= m.addr_p->offset()();
            ++pc_rel_cnt;
        }
        else
            ++minus_cnt;
    }

    // loop back thru finding compatable `minus` as needed
    auto m_iter = minus.begin();

    // handle "paired" `plus` expressions
    for (auto& p : plus)
    {
        // if `plus` matched, update `offset`
        if (p.addr_p && p.p)
        {
            reloc.addend += p.addr_p->offset()();
            reloc.addend -= p.p->addr_p->offset()();
            continue;
        }
        
        // if PC_REL pending, & `p` in correct section, consume a `pc_rel`
        if (pc_rel_cnt)
            if (p.addr_p && &p.addr_p->section() == &base.get_section())
            {
                reloc.addend += p.addr_p->offset()();
                --pc_rel_cnt;
                continue;
            }

        // here generate a `relocation`. 
        // if `pc_rel` pending, perform first
        if (pc_rel_cnt)
        {
            --pc_rel_cnt;
            do_emit(reloc, &p, true);
            continue;
        }
        
        // emit pending `minus` next
        if (minus_cnt)
        {
            --minus_cnt;
            for(;;)
            {
                auto& m = *m_iter++;
                
                if (m.p)
                    continue;
                if (m.addr_p && &m.addr_p->section() == section_p)
                    continue;

                // here need to emit `minus` relocation
                // same width & offset
                RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits()}, 0, reloc.offset };
                
                do_emit(m_reloc, &m);
                break;
            }
        }

        // emit `plus` reloc (w/o pc_rel)
        do_emit(reloc, &p);
    }

    // edge cases: some object formats support subtracting symbols.
    // test if there are any.

    while(pc_rel_cnt--)
    {
        RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits()}, 0, reloc.offset };
        do_emit(m_reloc, nullptr, true);
    }
    
    if (minus_cnt)
    {
        for (auto& m : minus)
        {
            if (m.p)
                continue;
            if (m.addr_p && &m.addr_p->section() == section_p)
                continue;

            // here need to emit `minus` relocation
            // same width & offset
            RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits()}, 0, reloc.offset };
            
            do_emit(m_reloc, &m);
        }
    }
}
}

#endif
