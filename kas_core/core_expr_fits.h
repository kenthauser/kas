#ifndef KAS_CORE_EXPR_FITS_H
#define KAS_CORE_EXPR_FITS_H

/******************************************************************************

This is where "{min,max}" in addresses and opcodes pays off.

An assembler has two difficult tasks: matching source code with
machine instructions, and resolving object code addresses. The parser &
opcode subsystem takes care of decoding source. The "relax" process takes
care of resolving addresses.

The core of the "relax" process is determining if an expression can
be within a range of values. Many assembler instructions have different
formats based on the value of arguments. A prime example are the "branch"
instructions which may have different forms for "near", "far", and "external"
branch locations. The `core_fit` type is used to test the limits of
"near", "far", etc, with the `offset {min,max}` to produce "YES, NO, MAYBE"
results. Hopefully, holding "{min,max}" instead of a single value allows
quicker convergence.

During the "relax" process, each fragment is examined in segment order to
calculate a "{min,max}" for each fragment (which is the sum of the {min,max}
of each insn). The static sum of this "offset" and the base address of the
fragment is then stored as the base address of the next fragment. Thus,
during a "relax" pass, all processed fragments (as well as the current
fragment) will have a "base address" based on the new pass, while the
not-yet-processed fragments will have a base address based on the previous
pass. In order to be able to calculate an offset (for `fits`, etc) the
"delta" calculated and applied to the current fragment must be applied to
not-yet-processed segments.

Also during the "relax" process, the current fragment (containing `dot`)
will contain a mix of previous-pass and current-pass addresses as the
fragment inspection proceeds. Since each `label insn` holds the {min,max}
of the previous pass, a new "dot fragment" delta to be applied to later
addresses can be calculated as a running "in-fragment delta". This
"in-fragment delta" also needs to be applied to the yet-to-be processed
fragments.

Fortunately, the code to implement this algorithm requires fewer instructions
to implement than paragraphs to describe it. The relevant calculation
code is at `insn_container::do_frag`. The deltas live in `core_expr_dot`.

The final piece of this puzzle is determining which fragments are "before",
"during", and "after" dot. This is done once during the initial resolution
of the `core_expr`. Since each expression is evaluated in exactly one
point in the translation, only a single examination is needed.

******************************************************************************/


#include "core_types.h"
#include "core_addr.h"
#include "core_fits.h"

namespace kas::core
{
// calculate the mutable value in `core_expr`

template <typename REF>
short core_expr<REF>::calc_num_relocs() const
{
    if (reloc_cnt < 0)
    {
        flatten();      // resolve labels -> addresses (if defined)
                        // resolve EQU to value (could be address/core_expr_t)
        
        pair_nodes();   // if plus & minus terms in same segment, pair to create offset

        // count number of un-paired terms:
        // NB: don't yet determine if valid relocations exist
        auto unpaired = [](auto&& p) { return !p.p; };
        reloc_cnt  = std::count_if(plus.begin(),  plus.end(),  unpaired);
        reloc_cnt += std::count_if(minus.begin(), minus.end(), unpaired);
    }
    return reloc_cnt;
}

// link `plus` & `minus` nodes in same fragment
template <typename REF>
void core_expr<REF>::pair_nodes () const
{
    // find `plus` addr_p to balance `minus` addr_p
    auto find_plus = [&](expr_term m) -> expr_term const *
    {
        auto m_fp = m.addr_p ? m.addr_p->frag_p : nullptr;

        if (m_fp)
            for (auto& p : plus)
            {
                if (p.p)            // already spoken for?
                    continue;

                // look for minus in same segment as plus
                if (auto p_fp = p.addr_p ? p.addr_p->frag_p : nullptr)
                    if (m_fp->segment() == p_fp->segment())
                        return &p;
            }
        return {};
    };

    // pair-up plus & minus
    for (auto& m : minus)
        if (auto p = find_plus(m))
        {
            p->p = &m;
            m.p = p;
        }
}

#if 1
template <typename REF>
expr_offset_t core_expr<REF>::expr_term::offset(core_expr_dot const *dot_p) const
{
    if (!addr_p)
        return {};

    auto offset = addr_p->offset(); 

    if (!dot_p)
        return offset;


    auto delta  = dot_p->rebase(*addr_p);
    
    //std::cout << "expr_term::offset: offset = " << offset << " delta = " << delta << std::endl;
    return delta;
}

#else
// declare the cases for "applying delta" to fragments
enum core_expr<REF>::expr_term::cx_delta_t : uint16_t
     {
       CX_UNDEF
     , CX_NO_DELTA
     , CX_FRAG_BEFORE
     , CX_DOT_BEFORE
     , CX_DOT_AFTER
     , CX_FRAG_AFTER
     };

expr_offset_t core_expr<REF>::expr_term::offset(core_expr_dot const *dot_ptr) const
{
    auto calc_delta = [dot_ptr](auto&& node) -> cx_delta_t
    {
        auto dot = *dot_ptr;
        // node before or diffent segment
        if (*node.addr_p->frag_p < *dot.frag_p) {
            return expr_term::CX_NO_DELTA;
        } else if (node.addr_p->frag_p != dot.frag_p) {
            return expr_term::CX_FRAG_AFTER;
        } else if (node.addr_p->offset().min < dot.frag_offset().min) {
            return expr_term::CX_DOT_BEFORE;
        } else {
            return expr_term::CX_DOT_AFTER;
        }
    };

    if (!addr_p)
        return {};

    expr_offset_t offset = *addr_p->offset_p;

    //std::cout << "core::expr::offset: before cx calc'd: offset = " << offset << " cx = " << (int)cx << std::endl;

    if (dot_ptr && (cx == CX_UNDEF))
        cx = calc_delta(*this);
    
    //std::cout << "core::expr::offset: after cx calc'd: offset = " << offset << " cx = " << (int)cx << std::endl;

    if (dot_ptr)
        switch (cx)
        {
            default:
                // XXX need BAD_CASE() macro
                assert("core_expr<REF>::expr_term::offset: cx value" == nullptr);
            case CX_UNDEF:
            case CX_NO_DELTA:
            case CX_FRAG_BEFORE:
            case CX_DOT_BEFORE:
                break;
            case CX_DOT_AFTER:
                offset += dot_ptr->cur_delta;
                break;
            case CX_FRAG_AFTER:
                offset += dot_ptr->base_delta - dot_ptr->cur_delta;
                break;
        }

    //std::cout << "core::expr::offset: cx applied: offset = " << offset << " cx = " << (int)cx << std::endl;

    

    // if both pair's delta now match, but didn't when other was calculated,
    // the other pair's delta was used in calculation. Now that this
    // node's delta was also used, if pair's deltas match, just delete.
    if (p && p->cx == cx)
        p->cx = cx = expr_term::CX_NO_DELTA;

    //std::cout << "expr_term::offset: " << offset << std::endl;
    return offset + addr_p->frag_p->base_addr();
}
#endif

template <typename REF>
expr_offset_t core_expr<REF>::get_disp(core_expr_dot const& dot) const
{
    // NB: here we know `disp_ok()` is true

    // XXX need to refactor CX_* into method & apply to `dot`
    // XXX dot.offset() is for section
    return get_offset() - dot.offset();
}

template <typename REF>
bool core_expr<REF>::disp_ok(core_expr_dot const& dot) const
{
    if (reloc_cnt != 1)
        return false;

    typename decltype(plus)::value_type const *p {};

    for (auto&& expr : plus)
        if (!expr.p)
        {
            p = &expr;
            break;
        }

    // assert(p);
    // get frag_p if defined. (not defined for external symbol for example)
    auto p_fp = p->addr_p ? p->addr_p->frag_p : nullptr;
    return p_fp && p_fp->segment() == dot.frag_p->segment();
}

//
// Hook `core_expr` into the `core_fits` world
//

auto core_fits::operator()
    (core_addr_t const& addr, fits_min_t min, fits_max_t max, int disp) const
    -> result_t
{
#if 1
    std::cout << "core_fits: (disp) core_addr: " << expr_t(addr);
    std::cout << " min/max = " << std::dec << min << "/" << max;
    std::cout << " disp = " << disp;
    std::cout << " fuzz = " << fuzz;
    std::cout << std::endl;
#endif
    
    if (!dot_p)
        return maybe;

#if 1
    std::cout << "addr frag = " << *addr.frag_p;
    std::cout << " dot frag = " << *dot_p->frag_p;
    std::cout << " base_delta = " << dot_p->base_delta;
    std::cout << " seen = " << std::boolalpha << dot_p->seen_this_pass(addr);
    std::cout << std::endl;
    std::cout << "addr offset = " << addr.offset();
    std::cout << " dot offset = " << dot_p->offset();
    std::cout << " cur delta = "  << dot_p->cur_delta;
    std::cout << std::endl;
#endif
    // initial `fuzz`: just check sections if not seen
    if (fuzz < 0)
    {
        if (&addr.section() != &dot_p->segment().section())
            return no;

        // can't test offset if haven't ever seen symbol
        if (!dot_p->seen_this_pass(addr))
            return maybe;
    }    
    
    expr_offset_t offset = dot_p->rebase(addr) - dot_p->offset();
    //std::cout << "core_fits: offset = " << offset << ", disp = " << disp << std::endl;
    return (*this)(offset, min, max, disp);
}



auto core_fits::operator()
    (core_expr_t const& e, fits_min_t min, fits_max_t max) const
    -> result_t
{
#if 1
    std::cout << "core_fits: expr: " <<  e;
    std::cout << " min/max = " << std::dec << min << "/" << max;
    std::cout << " fuzz = " << fuzz;
    std::cout << std::endl;


#endif
    // examine `core_expr_t` for unpaired terms
    auto cnt = e.calc_num_relocs();
    std::cout << "expr = " << expr_t(e) << ", cnt = " << cnt << ", reloc_cnt = " << e.reloc_cnt << std::endl;

    if (cnt == 0)
        return (*this)(e.get_offset(), min, max, 0);

    else if (fuzz < 0)
        e.reloc_cnt = -1;       // need to re-examine relocs after first pass

    // don't test offset until first relax pass complete
    if (fuzz < 0)
        return maybe;

    auto result = (*this)(e.get_offset(dot_p), min, max);
#if 0
    std::cout << "\nfits (" <<  min << ", " << max << "): ";
    std::cout << "\nexpr = " << expr_t(e) << " offset = " << e.get_offset(&dot);
    std::cout << " dot_offset = " << dot.dot_offset();
    std::cout << " base_delta = " << dot.base_delta;
    //std::cout << " delta = " << dot.cur_delta;
    std::cout << " -> " << result << std::endl;;
#endif
    return result;
}

// calculate if displacement is in range
auto core_fits::operator()
    (core_expr_t const& e, fits_min_t min, fits_max_t max, int delta) const
    -> result_t
{
    switch (e.num_relocs())
    {
        case -1:
            // initial relax -- count not known.
            return maybe;
        case  1:
            // single reloc -- check if reloc matchs `dot`
            break;
        case 0:
        default:
            // zero or multiple relocs: can not be `offset`
            return no;
    }
    
    if (!dot_p)
        return maybe;

#if 0
    std::cout << "\nfits (" <<  min << ", " << max << "): ";
    std::cout << "\nexpr = " << expr_t(e) << " offset = " << e.get_offset(dot_p);
    std::cout << " dot_offset = " << dot_p->dot_offset();
    std::cout << " base_delta = " << dot_p->base_delta;
    //std::cout << " delta = " << dot.cur_delta;
    std::cout << std::endl;
#endif

    if (!e.disp_ok(*dot_p))
            return no;

    return (*this)(e.get_disp(*dot_p) - delta, min, max);
}


}

#endif
