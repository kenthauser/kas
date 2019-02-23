#ifndef KAS_CORE_SEGMENT_H
#define KAS_CORE_SEGMENT_H

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
    core_segment(core_section& section, uint32_t subsection)
            : section_p(&section)
            , subsection(subsection)
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
    // XXX XXX ??? 

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
}
#endif
