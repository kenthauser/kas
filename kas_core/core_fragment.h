#ifndef KAS_CORE_FRAGMENT_H
#define KAS_CORE_FRAGMENT_H

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

#include "core_segment.h"
#include "kas_object.h"
#include "core_size.h"
#include "kas/kas_string.h"


#include <map>


namespace kas::core
{

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
    core_fragment(core_segment *seg_p = {}, uint8_t align = {});
    
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
    auto frag_num()    const { return base_t::index() - 1; }

    // also tests segment
    bool operator<(core_fragment const& other) const;
    
    // used by core::addr print routine
    operator bool() const { return frag_size != INIT_SIZE; }


    //
    // methods used to update `core_fragment` instances
    //

    // required alignment for this fragment
    //void set_alignment(uint16_t align);
    
    //
    // `set_org` is a unicorn. May (still) be used in some
    // os base pages. Support it, but don't add any cycles
    // to modules which don't use it. See, for contrast
    // `set_alignment` which is very common and worthy of cycles.
    //

    frag_offset_t set_org(uint32_t org);

    // setter method used by `insert` & `relax` to update fragment's size
    // propogate to next frag's "base_addr" until this frag is "relaxed"
    void set_size(frag_offset_t const& size);

    // used by `relax` to propogate base address to next fragment
    // when size is relaxed, but base address may not be.
    void set_size();

    template <typename OS> void print(OS& os) const;
    void dump_one(std::ostream&) const;

private:
    // update base address from previous frag
    // apply appropriate delta to provide requested alignment
    void set_base(addr_offset_t const& base);

    // undo_relax: use when `align` or `org` attributes are applied to frag
    void undo_relax();
    

    core_segment const *seg_p;      // containing segment (mutable)
    core_fragment *next_p_ {};      // linked list in segment order
    expr_t        *org_expr;        // expression for "org"

    uint8_t  frag_alignment{};      // alignemnt for fragment
    bool     frag_is_relaxed{};     // this fragment addressing complete
    uint16_t align_delta{};         // current frag alignment delta

    // `frag_size` & `frag_base_addr` values are managed by `relax`
    // values updated at end of `insn_container::do_frag`

    // value for size un-inited.
    static constexpr frag_offset_t INIT_SIZE { 0, frag_offset_t::max_limit };

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

}
#endif
