#ifndef KAS_CORE_SEGMENT_H
#define KAS_CORE_SEGMENT_H

/*
 * core_segment: hold linked list of `core_fragment`s
 *
 * `core_segment` manages object code for `core_section`
 *
 * Refer to "core_segment.h" for more detailed explaination.
 */

#include "kas_object.h"
#include "core_size.h"
#include "kas/kas_string.h"

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
    // NB: subsection number is used for "print" only...
    core_segment(core_section const& section, unsigned subsection = {})
             : section_p(&section), subsection(subsection) {}

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

    // get pointer to first frag in segment linked list
    auto initial()       const { return first_frag; }
    auto initial()             { return first_frag; }
    auto initial_relax()       { return relax_frag; }
    
    // execute FN for all frags in this segment
    // XXX implementation in `core_fragment.h`
    template <typename FN> void for_frags(FN fn);

    // find frag for offset
    template <typename Offset_t>
    core_fragment const *find_frag_for_offset(Offset_t const&) const;

    template <typename OS> void print(OS&) const;

    template <typename OS>
    void dump_one(OS& os) const
    {
        // print segment # in decimal, everything else in hex.
        os << ": " << std::left  << std::setw(20) << *section_p;
        os << " "  << std::dec   << size();
        os << " (hex) = " << std::hex << size().max;
    }

    friend std::ostream& operator<<(std::ostream& os, core_segment const& s)
    {
        s.print(os); return os;
    }
    
    // linked-list is managed in the `core_fragment` ctor
    friend core_fragment;
    core_fragment         *first_frag {};
    core_fragment         *last_frag  {};
    mutable core_fragment *relax_frag {};

    // inited by ctor
    core_section  const *section_p;
    unsigned             subsection;
    static inline core::kas_clear _c{base_t::obj_clear};
};
}
#endif

