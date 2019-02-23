#ifndef KAS_CORE_FRAGMENT_IMPL_H
#define KAS_CORE_FRAGMENT_IMPL_H

#if 0

support for assembler sections

The assembler supports putting code (and other items) in mutilple
sections. In the original `a.out` format there were three sections:
'.text', '.data', and '.bss'. These sections held, respectively,
read-only code, read-write data, and zero-initialized data.

The `coff` and `elf` formats added additional user-defined sections.
Integral subsections (eg text1 and data2) have long been supported:
subsection code is grouped together before being bundled together to
make up the section.

To support this abstraction, the assembler defines three concepts:

core_section: the named grouping of code, divided into subsections.
core_segment: a subsection into which code is assembled.
core_fragment: consecutive instructions in the same segment.

/******************************************************************************

Implementation notes:


Fragments have three immutable attributes: allocation order (int `index`),
location in "physical" list, and location in "logical" list. The "physical"
list orders the fragments in parse order. The "logical" list orders the
fragments according to the `core_section` active when the fragment was allocated.

Both ordered lists are implemented as linked lists: "physical" list via
`next_p` &  XXX refactor XXX


1. A `core_fragment` encapsulates a number of sequential `core_insn` objects.

The values incorporated are:

- insn_size()


******************************************************************************/

#endif

#include "core_fragment.h"

namespace kas::core
{

//
// ctor adds fragment to end of per-segment linked-list
// linked-list head/tail in associated `core_segment`
// is also managed.
//

core_fragment::core_fragment(core_segment *seg_p, uint8_t align)
{
    // remember most recent segment
    static core_segment *last_seg;

    // copy last_seg to register
    if (seg_p)
        last_seg = seg_p;
    else
        seg_p = last_seg;
    
    // must be able to determine segment
    if (!seg_p)
        throw std::logic_error("no segment for frag");

    // complete initialization of this fragment
    // -> next_p, seg_p & base_addr
    this->seg_p = seg_p;

    if (!seg_p->first_frag)
        seg_p->first_frag = this;
    else
    {
        // this frag continues from last frag...
        auto last_p = seg_p->last_frag;
        last_p->next_p_ = this;

        // set base address based on end of previous frag
        // don't use methods: just set directly
        frag_alignment = align;
        frag_base_addr = last_p->frag_base_addr + last_p->frag_size;
    }

    // perform frag accounting in `core_segment`
    seg_p->last_frag = this;

    // locate first fragment not relaxed
    if (!seg_p->relax_frag)
        seg_p->relax_frag = this;
}


//
// methods used to update `core_fragment` instances
//
#if 0
// required alignment for this fragment
void core_fragment::set_alignment(uint16_t align)
{
    undo_relax();
    frag_alignment = align;
    set_base(frag_base_addr);
}
#endif
//
// `set_org` is a unicorn. May (still) be used in some
// os base pages. Support it, but don't add any cycles
// to modules which don't use it. See, for contrast
// `set_alignment` which is very common and worthy of cycles.
//

auto core_fragment::set_org(uint32_t org) -> frag_offset_t
{
    // algorithm:
    //
    // 1. if org backwards, error.
    //
    // 2. if current base is relaxed, previous
    //    frag will not disturb base_addr, so just
    //    set.
    //
    // 3. Otherwise, advance min to `org` & arrange
    //    for org_insn::calc_size to re-init next loop.
    //

    undo_relax();
    frag_offset_t offset;
    int delta = org - frag_base_addr.min;
    
    if (delta < 0)
        offset.set_error();
    else if (frag_base_addr.is_relaxed())
        set_base(org);
    else {
        // advance both min & max
        frag_base_addr += delta;
        offset = {0, 4};    // min = 0, max = some alignment
    }
  
   return offset;
}

// setter method used by `insert` & `relax` to update fragment's size
// propogate to next frag's "base_addr" until this frag is "relaxed"
void core_fragment::set_size(frag_offset_t const& size)
{
    std::cout << "frag: set_size: " << *this << " " << frag_size << " -> " << size << std::endl;
    // ignore noise from `insn_container::do_proc`
    // size updated on relaxed frags when emitting
    // object code or listings...
    if (frag_is_relaxed)
        return;

    // record new size
    frag_size = size;
    
    // propogate to next frag using common method
    set_size();
}

// used by `relax` to propogate base address to next fragment
// when size is relaxed, but base address may not be.
void core_fragment::set_size()
{
    std::cout << "frag: set_size: " << *this << " (push) " << frag_size << std::endl;
    // update base_address() in next frag (if not last...)
    // NB: don't propogate if have previously pushed a "relaxed" address.
    // This to prevent overwriting addresses modified by "align" & "org" 

    auto end = end_addr();
    if (next_p_)
        next_p_->set_base(end);

    // are addresses final?
    frag_is_relaxed = end.is_relaxed();

    // start relax at next frag.
    if (frag_is_relaxed)
        seg_p->relax_frag = next_p_;

    std::cout << "frag: set_size: " << *this << " is relaxed" << std::endl;

}

// update base address from previous frag
// apply appropriate delta to provide requested alignment
void core_fragment::set_base(addr_offset_t const& base)
{
    std::cout << "set_base: " << *this << " base = " << base;
    std::cout << " align = " << +frag_alignment;
    
    if (frag_is_relaxed) 
    {
#if 1
        std::cout << "set_base: ";
        if (seg_p)
            std::cout << "frag_p = " << *this << " seg = " << *seg_p;
        else
            std::cout << "seg_p == nullptr";
        std::cout << std::endl;
        core_segment::dump(std::cout);
        core_fragment::dump(std::cout);
#endif       
        throw std::logic_error("core_fragment::set_base: already relaxed");
    }
    
    frag_base_addr = base;

    if (frag_alignment)
    {
        bool prev_is_relaxed = base.is_relaxed();
        //auto mask = (1 << frag_alignment) - 1;
        auto mask = frag_alignment - 1;
        align_delta = frag_base_addr.max & mask;

        if (align_delta)
            align_delta = 1 + mask - align_delta;

        frag_base_addr.max += align_delta;

        // if base_address is stable, we can lock in alignment
        if (prev_is_relaxed)
            frag_base_addr.min += align_delta;
    }
    std::cout << " -> " << frag_base_addr << std::endl;
}

// undo_relax: use when `align` or `org` attributes are applied to frag
void core_fragment::undo_relax()
{
    if (frag_is_relaxed) {
        frag_is_relaxed = false;
        if (!seg_p->relax_frag)
            seg_p->relax_frag = this;
    }
}

void core_fragment::dump_one(std::ostream& os) const
{
    os << std::hex;
    os << ": " << std::left  << std::setw(20) << *seg_p;

    // if frag sizes inited, print size data
    if (*this)
    {
        os << ": base = " << frag_base_addr;
        os << ", size = " << frag_size;
        if (auto align = alignment())
            os << ", align = " << +align;
        if (auto end = base_addr().max + frag_size.max)
            os << " (end) = 0x" << std::hex << end;
        if (frag_is_relaxed)
            os << " (relaxed)";
    }
    
    if (next_p_)
        os << ", next = " << std::dec << next_p_->frag_num();
}
    
template <typename OS>
void core_fragment::print(OS& os) const
{
    std::ostringstream str;
    str << frag_num() << "[" << section().name() << "]";
    os << str.str();
}

template <typename OS>
void core_segment::print(OS& os) const
{
    std::ostringstream str;
    str << *section_p << ":" << subsection;
    os << str.str();  
}

}
#endif
