#ifndef KAS_CORE_CORE_EXPR_DOT_H
#define KAS_CORE_CORE_EXPR_DOT_H


/******************************************************************************


core_expr_dot(frag const&)

Delta from previous pass applied to base address calculated as follows:

// XXX fix up comment.
Consider: previous frag this pass applied set_base(end_base()) to current frag
-> frag::set_base() re-inits base address & re-calculates delta.
-> frag::size()     unaffected

Thus next_p-> base_addr() based on previous pass sum of 
    - this frag previous base address (now overwritten)
    - this frag size (still valid)
    - next frag alignment

Thus this->set_base delta is:

    prev_end = next_p->base_addr;

    // remove alignment delta possibly applied.
    if (prev_end.is_relaxed())
        prev_end -= next_p->delta();
    else
        prev_end.max -= next_p->delta();


`base_delta` must be applied to all frags *after* this one to correct for previous
changes this pass.

In particular, don't apply base_delta to frags which have been previously visited this pass, or
frags which are not in same `core_segment`. Use `core_fragment::operator>()` to identify
fragments which need correcttion.

Adjustment of addresses is more difficult: it's harder to tell if an address
in this fragment is `last pass` or `current pass`.

To sort his out, use the two `offset_t` invariants: the `min` value may only
increase & the `max` value may only decrease. Thus if `max` is greater or equal
the current-pass accumulated `max`, the address *may* have been seen. For these
addresses set `min` to maximum of currently accumulated `min` and stored `min`.

Since these address deltas are used to see if displacement is in range, this logic
trades some accuracy for simplicity.



When processing a frag, we accumulate a running "{min,max}" of changes to the current
program counter since the beginning of the fragment. To determine if the "address" is
before or after the current PC consider the following:

min may only increase. max may only decrease. Thus, if addresses min is > current min,
it must be after. Likewise if address max < current max, it must be after. 

We have a current frag delta. It is always { +min, -max }.

Note: not a problem to "not" apply reduction of min. Should apply reduction of "max"

Thus don't adjust max (stored in address). Max adjusts itself downward as walk thru fragment.
Since size(max) never decreases, walk-thru max is the appropriate value.

Adjust min to insure never decreasing. min = max of current(min) & addr(min)



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


namespace kas::core
{

struct core_expr_dot
{
    // methods to examine `dot`
    // analogous method to those in core_addr
    auto& section() const
    {
        return frag_p->segment().section();
    }

    // offset into section
    auto offset() const
    {
        auto base = frag_p->base_addr();
        return base + dot_offset();
    }

    // methods used by `insn_container` to
    // to track dot
    void set_frag(core_fragment const& new_frag)
    {
        frag_p     = &new_frag;
        dot_offset = {};
        cur_delta  = {};
        base_delta = {};

#if 0
        // calculate the frag_delta for this relax pass
        // used by friend `core_expr` to calculate deltas
        if (auto p = frag_p->next_p())
            base_delta = p->base_addr() - frag_p->end_addr();

#else
        auto next_p   = frag_p->next_p();
        if (!next_p) {
            // last frag -- ignore base delta
            return;
        }
            

        auto prev_end = next_p->base_addr();

        // remove alignment delta possibly applied.
        if (prev_end.is_relaxed())
            prev_end -= next_p->delta();        // min & max
        else
            prev_end.max -= next_p->delta();    // max only

        // NB: be careful with unsigned arithmetic
        base_delta  = frag_p->base_addr();
        base_delta -= (prev_end - frag_p->size());
#endif
    }

    bool seen_this_pass(core_fragment const *expr_frag_p, addr_offset_t const *p) const
    {
        if (*expr_frag_p < *frag_p)
            return true;

        if (expr_frag_p != frag_p)
            return false;
        
        // 1. first invariant: max is always increasing. Thus if `dot`.max has
        //      exceeded corrected `addr` max, we've seen it.
        // 2. second invariant: `min` <= `max`. Thus `min` can't increase w/o
        //      an increase in `max`. 
    
        return (p->max + cur_delta.max) < dot_offset.max;
    }

    bool seen_this_pass(core_addr const& addr) const
    {
        return seen_this_pass(addr.frag_p, addr.offset_p);
    }

    // calculate offset to apply to raw address to allow
    // comparision of current pass & previous pass addresses
    expr_offset_t rebase(core_fragment const *expr_frag_p, addr_offset_t const *p) const
    {
        auto offset = expr_frag_p->base_addr() + *p;

        if (seen_this_pass(expr_frag_p, p))
            return offset;
        
        // if not in current frag, apply base_delta
        if (expr_frag_p != frag_p)
            offset += base_delta;

        return offset + cur_delta;
    }

    auto rebase(core_addr const& addr) const
    {
        return rebase(addr.frag_p, addr.offset_p);
    }

    template <typename OP_SIZE_T>
    void advance(OP_SIZE_T const& new_size, OP_SIZE_T const& old_size)
    {
        dot_offset += new_size;
        // NB: be careful with unsigned arithmetic
        cur_delta  += new_size;
        cur_delta  -= old_size;
    }

    auto& frag_offset() const
    {
        return dot_offset;
    }

    //
    // part of the pseudo-org `org` implementation.
    // cast away const & modify base_delta as required.
    //
private:
    auto set_org(uint32_t org)
    {
        auto offset = const_cast<core_fragment *>(frag_p)->set_org(org);
        return offset;
    }

public:
    auto set_org(uint32_t org) const
    {
        return const_cast<core_expr_dot *>(this)->set_org(org);
    }
    
    core_fragment const *frag_p {};

//private:

    // XXX should probably use public interface
    friend struct core_expr;

    // initialized by `set_frag`
    addr_offset_t dot_offset;       // dot's offset in current frag
    expr_offset_t base_delta;       // delta applied to the frag base_addr (to be propogated)
//private:
    expr_offset_t cur_delta;
};

}
#endif
