#ifndef KAS_CORE_FRAGMENT_H
#define KAS_CORE_FRAGMENT_H

/*
 * core_fragment
 *
 * `core_fragment` manages a consecutive sequence of object code
 * values. The `core_fragment` object manages "attributes" of the
 * fragment, and does not hold the actual object code.
 *
 * The actual object code data is generated from objects
 * stored in a `insn_container`. The `insn_container` allocates
 * `core_fragment` instances. A new "fragment" is allocated to support
 * opcodes such as `align` or `org`. A new "fragment" can also be allocated
 * to keep individual fragments from becoming too large or inefficient for
 * relax operations.
 *
 * `core_segment` is used to manage a linked list of `core_fragment` instances.
 * A new "segment" is allocated when an opcode such as ".text" causes the
 * assembler to generate "non-consective" object code. 
 *
 * Attributes stored in `core_fragment` consist of links to the next 
 * (linked list) fragment, the containing segment, and the starting address.
 * Additional attributes are used by `relax` to support fragment alignment.
 * 
 * Counter-intuitively, `core_fragments` to not hold a "size" attribute.
 * To calculate the "size" of a fragment, the entire fragment must be 
 * walked. This design decision is made to simplify ".align" and ".org"
 * operations. If the "next" fragment beginning address "relaxed"
 * (ie min==max), then size is irrelevent and fragment won't be walked.
 *
 * A pointer to a `core_fragment` object, along with the fragment `offset`,
 * are used to make up the `dot` describing the current location counter.
 *
 * Refer to "core_segment.h" for more detailed explaination of "segments".
 */


#include "core_segment.h"
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

    // last allocated frag (global)
    static inline core_fragment const *cur_frag;

    //
    // instance getters
    //
    
    auto  next_p()     const { return next_p_;          }
    auto& segment()    const { return *seg_p;           }
    auto& section()    const { return seg_p->section(); }
    auto& base_addr()  const { return frag_base_addr;   }
    auto  end_addr()   const { return base_addr() + frag_size; }
    auto& size()       const { return frag_size;        }
    auto  alignment()  const { return frag_alignment;   }
    auto  delta()      const { return align_delta;      }
    bool  is_relaxed() const { return frag_is_relaxed;  }

    // `frag` indexes are zero based (and disallow lookup).
    // Force walking the linked-list or using saved pointer.
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
    void set_base(size_offset_t const& base);

    // undo_relax: use when `align` or `org` attributes are applied to frag
    void undo_relax();
    

    core_segment const *seg_p;      // containing segment (mutable)
    core_fragment *next_p_ {};      // linked list in segment order
    expr_t        *org_expr;        // expression for "org"

    uint16_t align_delta{};         // current frag alignment delta
    uint8_t  frag_alignment{};      // alignment for fragment
    bool     frag_is_relaxed{};     // this fragment addressing complete

    // `frag_size` & `frag_base_addr` values are managed by `relax`
    // values updated at end of `insn_container::do_frag`

    // value for size un-inited.
    static constexpr frag_offset_t INIT_SIZE { 0, frag_offset_t::max_limit };

    // base_addr is start of fragment (relative segment)
    size_offset_t  frag_base_addr{};
    frag_offset_t  frag_size;

    friend std::ostream& operator<<(std::ostream& os, core_fragment const& obj)
    {
        obj.print(os); return os;
    }
    static inline core::kas_clear _c{base_t::obj_clear};
};

inline size_offset_t core_segment::size() const
{
    if (last_frag)
        return last_frag->end_addr();
    
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
