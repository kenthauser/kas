#ifndef KAS_CORE_INSN_INSERTER_H
#define KAS_CORE_INSN_INSERTER_H


#include "insn_container_data.h"
#include "core_section.h"
#include "core_addr.h"
#include "core_expr_dot.h"
#include "core_symbol.h"

#include "opc_misc.h"
#include "opc_symbol.h"
#include "opc_segment.h"

#include <limits>
#include <cassert>

namespace kas::core
{

//////////////////////////////////////////////////////////////////////////
//
// declare `inserter` object in `insn_container`
//
//////////////////////////////////////////////////////////////////////////

template <typename INSN_DATA_T>
struct insn_inserter
{
    using value_t   = INSN_DATA_T;
    using op_size_t = typename INSN_DATA_T::op_size_t;

    template <typename CONTAINER_T>
    insn_inserter(CONTAINER_T&);
    ~insn_inserter();

    // inserter iterator methods
    insn_inserter& operator=(core_insn&&);
    auto& operator++()    { return *this; }
    auto& operator++(int) { return *this; }
    auto& operator*()     { return *this; }

    void new_frag(uint8_t align = {}, core_segment *seg_p = {});
  
// XXX temp
//private:
    // references to associated container
//    insn_container &c;
//    std::back_insert_iterator<INSN_DATA_T> bi;
    core_fragment *frag_p {};
    op_size_t      insn_size;     // size of current insn

    // insn types (generic & special requirements)
    value_t& put_insn   (value_t&&);
    void put_label  (value_t&&);
    value_t& put_segment(value_t&&);
    void put_align  (value_t&&);
    value_t& put_org    (value_t&&);

    void reserve(op_size_t const&);

    // container callback interface
    void     *cb_container_p {};
    value_t& (*bi_fn)(void *, value_t&&)            {};
    void     (*end_frag)(void *)                    {};
    void     (*at_end_fn)(void *, insn_inserter&)   {};

    // `dot` used while inserting insns
    core_expr_dot dot;

    // frag tuning variables...
    addr_offset_t offset;
    uint16_t frag_insn_max  {};
    uint16_t frag_relax_max {};

    uint16_t frag_insn_cnt;
    uint16_t frag_relax_cnt;
};

//////////////////////////////////////////////////////////////////////////
//
// inserter methods
//
//////////////////////////////////////////////////////////////////////////

// inserter `ctor`
template <typename INSN_DATA_T>
template <typename CONTAINER_T>
insn_inserter<INSN_DATA_T>::insn_inserter(CONTAINER_T& c)
{
    // initialize with container methods
    cb_container_p = &c;            // pointer to container
    bi_fn          = c.cb_bi;       // implement `back-inserter`
    end_frag       = c.cb_end_frag; // record end of fragment
    at_end_fn      = c.cb_at_end;   // implement `at_end`

    // initialize container with SEG (initial_segment)
    *this = {opc::opc_segment(), c.initial_segment};
    c.first_frag_p = frag_p;
}

// inserter `dtor`: resolve symbols
template <typename INSN_DATA_T>
insn_inserter<INSN_DATA_T>::~insn_inserter()
{
    at_end_fn(cb_container_p, *this);
}

// `inserter` primary method
template <typename INSN_DATA_T>
auto insn_inserter<INSN_DATA_T>::operator=(core_insn&& insn) -> insn_inserter&
{
    // get special opcode indexes
    static const auto idx_segment = opc::opc_segment().index();
    static const auto idx_org     = opc::opc_org()    .index();
    static const auto idx_align   = opc::opc_align()  .index();
    static const auto idx_label   = opc::opc_label()  .index();

    // generate container_data from insn
    value_t data{insn};
    
    // allocate new frag if current doesn't have room
    // NB: insn_size is instance variable which can be modified
    reserve(data.size());

    // see if special insn & process accordingly
    // if `dot` referenced: drop a label
    auto opc_index = insn.opc_index;
    if (opc_index == idx_label)
        put_label(std::move(data));
    else if (core_addr_t::must_init_dot())
        *this = opc::opc_label();       // insert dummy label insn

    // extract size from `insn`
    insn_size = insn.data.size;
    
    if (opc_index == idx_segment)
        put_segment(std::move(data));
    else if (opc_index == idx_org)
        put_org(std::move(data));
    else if (opc_index == idx_align)
        put_align(std::move(data));
    else if (opc_index != idx_label)
        put_insn(std::move(data));

    // move dot if object code emitted data
    if (insn_size.max)
    {
        core_addr_t::new_dot();
        dot.dot_offset += insn_size;
    }

    return *this;
}

// insert insn: generic instruction
template <typename INSN_DATA_T>
auto insn_inserter<INSN_DATA_T>::put_insn(value_t&& data) -> value_t&
{
    return bi_fn(cb_container_p, std::move(data));
}

// insert insn: label
template <typename INSN_DATA_T>
void insn_inserter<INSN_DATA_T>::put_label(value_t&& data)
{
    // emit label insn & init `core_addr` instance
    // NB: if `label` is to be created at this location, 
    // `core_symbol::make_label()` method will have already been
    // called. This routine creates the `core_addr` infrastructure
    // to initialize `dot` at this location.

    // if defining existing `addr` instance, retrieve pointer
    core_addr_t *addr_p {};
    if (auto idx = data.fixed.fixed)
        addr_p = &core_addr_t::get(idx);

    // NB: don't allocate two `labels` at the same address. 
    // convert INSN to `nop`
    if (!addr_p && !core_addr_t::must_init_dot())
    {
        static opc::opc_nop opc{};
        data.set_opc_index(opc.index());
        put_insn(std::move(data));
        return;
    }

    // use "fixed" area of insn for `offset`...
    // ...and initialize frag_p
    auto& fixed = put_insn(std::move(data)).fixed;
    fixed.offset = dot.frag_offset();     // init with first pass value
    
    // init `core_addr_t` instance for this location
    // default is current_dot
    if (!addr_p)
        addr_p = &core_addr_t::cur_dot();

    addr_p->init_addr(frag_p, &fixed.offset);
}

// insert insn: segment
template <typename INSN_DATA_T>
auto insn_inserter<INSN_DATA_T>::put_segment(value_t&& data) -> value_t&
{
    // get segment from insn & start new frag
    auto& segment = core_segment::get(data.fixed.fixed);
    auto  align   = segment.section().align();
    new_frag(align, &segment);
    return put_insn(std::move(data));
}

// insert insn: align
template <typename INSN_DATA_T>
void insn_inserter<INSN_DATA_T>::put_align(value_t&& data)
{
    // alignment stored in fragment, so need a new frag.
    auto align = data.fixed.fixed;
    new_frag(align);
    put_insn(std::move(data));
}

// insert insn: org
template <typename INSN_DATA_T>
auto insn_inserter<INSN_DATA_T>::put_org(value_t&& data) -> value_t&
{
#if 0
    // determine if new "org" address is org or skip...
    auto& target = core_insn::data[data.first];
    
    // calculate displacement to see if `skip`
    // NB: can't be skip if `dot` not previously allocated
    // XXX if (!core_addr_t::cur_dot().empty()) {
    if (!core_addr_t::cur_dot().empty()) {
        auto& disp =  target - core_addr_t::get_dot();
        if (auto skip = disp.get_fixed_p()) {
            // create skip insn...
            data.set_index(opc::opc_skip().index());
            data.set_size(*skip);
            target = disp;
            return put_insn(std::move(data));
        }
    }

    // `org` requires fixed address past current address...
    if (auto p = target.get_fixed_p()) {
        new_frag();
        auto offset = frag_p->set_org(*p);
        if (offset.is_error()) {
            auto& err = parser::kas_diag::error("can't org backwards");
            data.fixed.fixed = err.index();
        } else {
            // XXX fix when relax loop installed
            if (offset.is_relaxed())
                offset = {0, 4};
            op_size = offset;
            data.set_size(offset);
        }
    } else {
        // error -- need fixed target address 
        auto& err = parser::kas_diag::error("org requires fixed address");
        data.fixed.fixed = err.index();
    }
#endif
    return put_insn(std::move(data));
}

//
// inserter support routines
//

// NB: default (nullptr) is create frag in same segment
template <typename INSN_DATA_T>
void insn_inserter<INSN_DATA_T>::new_frag(uint8_t align, core_segment *seg_p)
{
    // update container accounting of fragments
    // NB: here, size is instruction index in container
    end_frag(cb_container_p);

    // make sure locations in previous frag remain in previous frag
    // NB: new segment might not be same, creating *total* error.
    // NB: new_dot() is no-op if old dot not previously referenced
    core_addr_t::new_dot();

#ifdef XXX
    // XXX could reduce relax a bit, but currently
    // screws up `frag.frag_is_relaxed` calculation
    // close old frag, recording it's size
    if (frag_p)
        frag_p->set_size(dot.frag_offset());
#endif
        
    // allocate new frag. possibly in new segment
    frag_p = &core_fragment::add(seg_p, align);

    // clear fragment tuning counts
    dot.set_frag(*frag_p);
    frag_insn_cnt  = 0;
    frag_relax_cnt = 0;
}

template <typename INSN_DATA_T>
void insn_inserter<INSN_DATA_T>::reserve(op_size_t const& op_size)
{
    // if offset type overflows, then new frag
    if (!offset.can_add(op_size))
        return new_frag();

    // test tuning variables for new frag
    if (frag_insn_max && frag_insn_max > ++frag_insn_cnt)
        return new_frag();
    if (!op_size.is_relaxed())
        if (frag_relax_max && frag_relax_max > ++frag_relax_cnt)
            return new_frag();
}

}
#endif
