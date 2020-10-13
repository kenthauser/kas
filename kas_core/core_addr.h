#ifndef KAS_CORE_CORE_ADDR_H
#define KAS_CORE_CORE_ADDR_H

//
// XXX Need to describe `core_addr` type
//

#include "core_terminal_types.h"
#include "kas_object.h"
#include "core_fragment.h"

namespace kas::core
{

// a location is identified as fragment + offset
template <typename REF>
struct core_addr : kas_object<core_addr<REF>, REF>
{
    using base_t  = kas_object<core_addr<REF>, REF>;
#if 0
    // causes ambiguity in ::print
    using token_t = parser::token_defn_t<
                            KAS_STRING("TOK_ADDR"), core_addr
                            >;
#endif

    
    using NAME = KAS_STRING("core_addr");

    enum { DOT_CUR, DOT_NEXT };

    // XXX for `expr_fits`
    using emits_value = std::true_type;

    using base_t::add;
    using base_t::dump;

    core_addr() = default;
    core_addr(core_fragment const *frag_p, addr_offset_t const *offset_p)
        : frag_p(frag_p), offset_p(offset_p) {}


    core_fragment const *frag_p   {};
    addr_offset_t const *offset_p {};

    frag_offset_t        offset()  const;
    core_section  const& section() const;

    // bool true iff `core_addr` is initialized
    bool empty() const { return !frag_p; }

    //
    // Routines which allocate addresses
    // (default ctor) returns dummy address
    //

    static core_addr& get_dot(int which = DOT_CUR)
    {
        auto dot_p = (which == DOT_NEXT) ? &next_dot : &current_dot;

        if (!*dot_p)
            *dot_p = &add();

        //std::cout << __FUNCTION__ << ": dot = " << *dot_p << std::endl;;
        return **dot_p;
    }

    //
    // methods used by `insn_container` to allocate
    // label instructions
    //

    static auto& cur_dot()
    {
        if (!current_dot)
            current_dot = &add();
        return *current_dot;
    }

    static void new_dot()
    {
        current_dot = next_dot;
        next_dot = {};
    }

    // for insn_inserter: was dot referenced
    static bool must_init_dot()
    {
        return current_dot && current_dot->empty();
    }

    auto& init_addr(core_fragment const *frag_p, addr_offset_t const* offset_p)
    {
        this->frag_p   = frag_p;
        this->offset_p = offset_p;
        return *this;
    }

    friend std::ostream& operator<< (std::ostream& os, core_addr const& addr)
    {
        addr.print(os);
        return os;
    }
    template <typename OS> void print(OS& os) const;
    
    static void clear()
    {
        current_dot = {};
        next_dot    = {};
    }

private:
    friend std::ostream& operator<< (std::ostream&, const core_addr&);
    static inline core_addr *current_dot;
    static inline core_addr *next_dot;

    // XXX why no local clear calls dependent clear?
    static inline core::kas_clear _c{base_t::obj_clear};
};

// with core_fragment defined, implement core_addr method
template <typename REF>
inline frag_offset_t core_addr<REF>::offset() const
{
    auto offset = frag_p->base_addr();
    if (offset_p)
        offset += *offset_p;
    return offset;
}

template <typename REF>
inline core_section const& core_addr<REF>::section() const
{
    return frag_p->segment().section();
}

inline bool core_fragment::operator<(core_fragment const& other) const
{
    // primary (only?) use of operator< is for `seen_this_pass`.
    // since relax a segment at a time, not-looking is same as has-seen.
    if (segment() != other.segment())
        return true;
    return frag_num() < other.frag_num();
}

template <typename REF>
template <typename OS>
void core_addr<REF>::print(OS& os) const
{
    // generate a single emit to OS (for formatting)
    std::ostringstream str;
    str << "ca(";
    if (!frag_p) {
        str << "[dot]";      // only `addr` w/o frag is dot.
    } else if (*frag_p && offset_p) {
        auto n = frag_p->base_addr().max + offset_p->max;
        str << frag_p->section() << "+" << std::hex << n;
    } else {
        str << frag_p->segment();
    }
    str << ")";

    os << str.str();
}

}
#endif
