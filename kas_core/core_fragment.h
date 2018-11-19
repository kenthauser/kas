#ifndef KAS_CORE_FRAGMENTS_H
#define KAS_CORE_FRAGMENTS_H

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

#include "kas_object.h"
#include "core_size.h"
#include "kas/kas_string.h"

#include <map>


namespace kas::core
{

// forward declarations for friend statements
struct core_fragment;
std::ostream& operator<<(std::ostream& os, core_section const& s);

// a segment is a handle to a linked-list of fragments
// perform accounting in `core_fragment` ctor.
struct core_segment : kas_object<core_segment>
{
    using NAME   = KAS_STRING("core_segment");

    // expose protected methods
    using base_t::dump;

    // allocate with static `add` method inherited from `kas_object`
    core_segment(core_section& section, uint32_t subsection) :
        section_p(&section), subsection(subsection)
        {}

    // instance variable getter's
    auto&      section() const   { return *section_p; }
    addr_offset_t size() const;

    bool operator==(core_segment const& other) const
    {
        return &other == this;
    }

    bool operator!=(core_segment const& other) const
    {
        return !(other == *this);
    }

    // XXX refactor `for_frags` to eliminate need.
    // XXX currently used in `dwarf_emit` to create
    // XXX a `core_addr` instance.

    // get pointer to first frag in segment list
    auto initial()       const { return first_frag; }
    auto initial()             { return first_frag; }
    auto initial_relax()       { return relax_frag; }
    
    // execute FN for all frags in this section
    template <typename FN> void for_frags(FN fn);

    // find frag for offset
    template <typename Offset_t>
    core_fragment const *find_frag_for_offset(Offset_t const&) const;

    template <typename OS> void print(OS&) const;
private:
    friend base_t;
    template <typename OS>
    void dump_one(OS& os) const
    {
        // print segment # in decimal, everything else in hex.
        os << std::hex;
        os << ": " << std::left  << std::setw(20) << *section_p;
        os << ": " << std::setw(2) << std::right  << subsection;
        os << " "  << size();
        os << " (hex) = " << std::hex << size().max;
    }

    friend std::ostream& operator<<(std::ostream& os, core_segment const& s)
    {
        s.print(os); return os;
    }
    
    // linked-list managed in `core_fragment` ctor
    friend core_fragment;
    core_fragment *first_frag {};
    core_fragment *last_frag  {};
    mutable core_fragment *relax_frag {};

    // inited by ctor
    core_section  const *section_p;
    uint32_t subsection;
    static inline core::kas_clear _c{base_t::obj_clear};
};

struct core_fragment : kas_object<core_fragment>
{
    using NAME = KAS_STRING("core_fragment");

    // expose base_t protected methods
    using base_t::for_each;
    using base_t::dump;
    
    // frag indexes are zero based (and disallow lookup)
    auto index() const = delete;
    static core_fragment& get(base_t::index_t frag_num) = delete;
    
    //
    // ctor adds fragment to end of per-segment linked-list
    // linked-list head/tail in associated `core_segment`
    // is also managed.
    //
    core_fragment(core_segment *seg_p = nullptr)
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
        else {
            // this frag continues from last frag...
            auto last_p = seg_p->last_frag;
            last_p->next_p_ = this;

            // set base address based on end of previous frag
            // don't use methods: just set directly
            frag_base_addr = last_p->frag_base_addr + last_p->frag_size;
        }

        // perform frag accounting in `core_segment`
        seg_p->last_frag = this;

        // locate first fragment not relaxed
        if (!seg_p->relax_frag)
            seg_p->relax_frag = this;
    }
    
    //
    // instance getters
    //

    auto  next_p()     const { return next_p_;         }
    auto& segment()    const { return *seg_p;          }
    auto& section()    const { return *seg_p->section_p; }
    auto& base_addr()  const { return frag_base_addr;  }
    auto  end_addr()   const { return base_addr() + frag_size; }
    auto& size()       const { return frag_size;       }
    auto  alignment()  const { return frag_alignment;  }
    auto  delta()      const { return align_delta;     }
    bool  is_relaxed() const { return frag_is_relaxed; }

    // frag indexes are zero based (and disallow lookup)
    // force walking the linked-list or using saved pointer
    auto frag_num()   const { return base_t::index() - 1; }

    // also tests segment
    bool operator<(core_fragment const& other) const;
    
    // used by core::addr print routine
    operator bool() const { return frag_size != INIT_SIZE; }


    //
    // methods used to update `core_fragment` instances
    //

    // required alignment for this fragment
    void set_alignment(uint16_t align)
    {
        undo_relax();
        frag_alignment = align;
        set_base(frag_base_addr);
    }

    //
    // `set_org` is a unicorn. May (still) be used in some
    // os base pages. Support it, but don't add any cycles
    // to modules which don't use it. See, for contrast
    // `set_alignment` which is very common and worthy of cycles.
    //

    frag_offset_t set_org(uint32_t org)
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
    void set_size(frag_offset_t const& size)
    {
        //std::cout << "frag: set_size: " << *this << " " << frag_size << " -> " << size << std::endl;
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
    void set_size()
    {
        //std::cout << "frag: set_size: " << *this << " (push) " << frag_size << std::endl;
        // update base_address() in next frag (if not last...)
        // NB: don't propogate if have previously pushed a "relaxed" address.
        // This to prevent overwriteing addresses modified by "align" & "org" 

        auto end = end_addr();
        if (next_p_)
            next_p_->set_base(end);

        // are addresses final?
        frag_is_relaxed = end.is_relaxed();

        // start relax at next frag.
        if (frag_is_relaxed)
            seg_p->relax_frag = next_p_;

        //std::cout << "frag: set_size: " << *this << " is relaxed" << std::endl;

    }

    template <typename OS> void print(OS& os) const;

private:
    // update base address from previous frag
    // apply appropriate delta to provide requested alignment
    void set_base(addr_offset_t const& base)
    {
        //std::cout << "set_base: " << *this << " base = " << base;
        //std::cout << " align = " << std::to_string(frag_alignment);
        
        if (frag_is_relaxed) {
#if 0
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
    
        if (frag_alignment) {
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
        //std::cout << " -> " << frag_base_addr << std::endl;
    }

    // undo_relax: use when `align` or `org` attributes are applied to frag
    void undo_relax()
    {
        if (frag_is_relaxed) {
            frag_is_relaxed = false;
            if (!seg_p->relax_frag)
                seg_p->relax_frag = this;
        }
    }

    friend base_t;
    template <typename OS>
    auto dump_one(OS& os) const
    {
        os << std::hex;
        os << ": " << std::left  << std::setw(20) << *seg_p;

        // if frag sizes inited, print size data
        if (*this) {
            os << ": base = " << frag_base_addr;
            os << ", size = " << frag_size;
            if (auto align = alignment())
                os << ", align = " << std::to_string(align);
            if (auto end = base_addr().max + frag_size.max)
                os << " (end) = 0x" << std::hex << end;
            if (frag_is_relaxed)
                os << " (relaxed)";
        }
        if (next_p_)
            os << ", next = " << std::dec << next_p_->frag_num();
    }

    core_segment const *seg_p;      // containing segment (mutable)
    core_fragment *next_p_ {};      // linked list in segment order
    uint8_t  frag_alignment{};      // alignemnt for fragment
    bool     frag_is_relaxed{};     // this fragment addressing complete
    uint16_t align_delta{};         // current frag alignment delta

    // `frag_size` & `frag_base_addr` values are managed by `relax`
    // values updated at end of `insn_container::do_frag`

    // value for size un-inited.
    static constexpr frag_offset_t INIT_SIZE {-1}; //{0, ~0};

    // base_addr is start of fragment (relative segment)
    addr_offset_t  frag_base_addr{};
    frag_offset_t  frag_size;

    friend std::ostream& operator<<(std::ostream& os, core_fragment const& obj)
    {
        obj.print(os); return os;
    }
    static inline core::kas_clear _c{base_t::obj_clear};
};

inline addr_offset_t core_segment::size() const
{
    if (last_frag) {
        return last_frag->end_addr();
    }
    return {};
}

template <typename FN>
void core_segment::for_frags(FN fn)
{
    for (auto p = first_frag; p; p = p->next_p())
        fn(*p);
}

#if 0
template <typename Offset_t>
const core_fragment *core_segment::find_frag_for_offset(Offset_t const& offset) const
{
    auto min = offset.min;
    auto frag_p = first_frag;

    while (frag_p) {
        min -= frag_p->size().min;
        if (min <= 0)
            break;
        frag_p = frag_p->next_p();

    return frag_p;
}
#endif
}
#endif
