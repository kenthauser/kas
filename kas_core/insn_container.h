#ifndef KAS_CORE_INSN_CONTAINER_H
#define KAS_CORE_INSN_CONTAINER_H

/*
insn_container

The `insn_container` accepts a stream of instructions via an
`inserter` & arranges them into fragments which can be processed as a
group. While inserting instructions, the `segment` and `label` opcodes
are interpreted.

The `insn_container` is templated on the type of actual storage used
for the instructions. The processing code is organized so as to only
require a `forward iterator` to the underlying storage container.

Three public methods are defined by `insn_container`


`inserter(FN)`

Return an `STL` inserter which appends instructions to container. Invoke
method FN on `container` at `dtor` of inserter.

`proc_all_frags`

`proc_frag`



At the end of instruction insertion (`inserter` dtor), all lcomm symbols
are moved to the `bss` section and the symbol table updated.

RELAX

Forward-Inserter


  */

#include "insn_container_data.h"
#include "core_section.h"
#include "core_addr.h"
#include "core_expr_dot.h"
#include "core_symbol.h"

#include "opc_misc.h"
#include "opc_symbol.h"

#include <limits>
#include <cassert>

namespace kas::core
{

    template <typename Insn_Deque_t = std::deque<insn_container_data>>
    struct insn_container : kas_object<insn_container<Insn_Deque_t>>
    {
        using base_t      = kas_object<insn_container<Insn_Deque_t>>;
        using value_type  = typename Insn_Deque_t::value_type;
        using op_size_t   = typename opc::opcode::op_size_t;

        using insn_iter   = typename Insn_Deque_t::iterator;
        using PROC_FN     = std::function<void(core_insn&, core_expr_dot const&)>;

        // insn_iter, insn_state & count for iterating a frag
        using insn_state_t = typename value_type::state_t;
        using frag_iter_t  = std::tuple<insn_iter, insn_state_t, uint32_t>;

        using base_t::for_each;

        // declare inserter -- implement below.
        struct insn_inserter;

        using DTOR_FN = std::function<void(insn_inserter&)>;

        insn_container(core_segment& initial, DTOR_FN fn = {}) 
                : initial_segment(initial)
                , dtor_fn(fn)
                {}

        // create container inserter
        auto inserter()
        {
            return insn_inserter(*this);
        }
        
        // getter for dot()
        auto& get_insn_dot() const
        {
            return dot;
        }

        // methods for processing container data
        core_fragment& proc_frag(core_fragment& frag, PROC_FN fn);
        void proc_all_frags(PROC_FN fn);

    public:
        friend base_t;
        static void clear()
        {
            //data().clear();
            value_type::clear();
        }
    // XXX temp
    // private:
    public:
        core_segment& initial_segment;
        DTOR_FN       dtor_fn{};

        // first frag for this container
        core_fragment const *first_frag_p {};
        
        void do_frag(core_fragment&, insn_iter&, uint32_t, PROC_FN);
        void do_frag(core_fragment&, frag_iter_t const&  , PROC_FN);
        Insn_Deque_t insns;
        //decltype(core_insn::data)::size_type data_initial = -1;
        std::deque<uint32_t> insn_index_list;
        std::vector<frag_iter_t> *insn_iters{};
        core_expr_dot dot;

        static inline core::kas_clear _c{base_t::obj_clear};
    };

    //////////////////////////////////////////////////////////////////////////
    //
    // declare `inserter` object in `insn_container`
    //
    //////////////////////////////////////////////////////////////////////////

    template <typename Insn_Deque_t>
    struct insn_container<Insn_Deque_t>::insn_inserter
    {
        using value_type = typename Insn_Deque_t::value_type;

        insn_inserter(insn_container&);
        ~insn_inserter();

        // inserter iterator methods
        //insn_inserter& operator=(value_type&&);
        insn_inserter& operator=(core_insn&&);
        auto& operator++()    { return *this; }
        auto& operator++(int) { return *this; }
        auto& operator*()     { return *this; }

        void new_frag(uint8_t align = {}, core_segment *seg_p = {});
      
    // XXX temp
    //private:
        // references to associated container
        insn_container &c;
        std::back_insert_iterator<Insn_Deque_t> bi;
        core_segment *lcomm_segment_p{};
        core_fragment *frag_p {};
        op_size_t      insn_size;     // size of current insn

        // insn types (generic & special requirements)
        void put_insn   (value_type&&);
        void put_label  (value_type&&);
        void put_segment(value_type&&);
        void put_align  (value_type&&);
        void put_org    (value_type&&);

        void reserve(op_size_t const&);

        // frag tuning variables...
        addr_offset_t offset;
        uint16_t frag_insn_max  {};
        uint16_t frag_relax_max {};

        uint16_t frag_insn_cnt;
        uint16_t frag_relax_cnt;
    };

    //////////////////////////////////////////////////////////////////////////
    //
    // container methods
    //
    //////////////////////////////////////////////////////////////////////////

    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::proc_all_frags(PROC_FN fn)
    {
        // `insn_index_list` holds "index" into container where each frag
        // begins. Use this information to create a container of
        // forward interators to access an individual `core_fragment`.
        // NB: Just copy the container forward iterator at the start of
        // each fragment.

        // if empty, skip
        if (insn_index_list.empty())
            return;

        // if first pass thru, save "forward iterators" for `proc_frag`
        bool must_generate_iters = !insn_iters;

        if (must_generate_iters) {
            insn_iters = new std::vector<frag_iter_t>();
            insn_iters->reserve(insn_index_list.size() - 1);
        }

        // proc_all: reinit "value_type" data structures
        value_type::reinit();
        auto insn_iter = insns.begin();
        auto it = insn_index_list.begin();

        auto proc_one_frag = [&](auto& frag)
            {
                auto count = it[1] - it[0];
                if (must_generate_iters)
                    insn_iters->emplace_back(insn_iter, value_type::get_state(), count);
                do_frag(frag, insn_iter, count, fn);
                ++it;
            };

        // process frags in this container
        auto num_frags = insn_index_list.size() - 1;
        core_fragment::for_each(proc_one_frag, first_frag_p, num_frags);
    }

    template <typename Insn_Deque_t>
    core_fragment& insn_container<Insn_Deque_t>::proc_frag
            (core_fragment& frag, PROC_FN fn)
    {
        assert(insn_iters);

        unsigned index = frag.frag_num() - first_frag_p->frag_num();
        if (index < insn_iters->size()) 
            do_frag(frag, (*insn_iters)[index], fn);
        return frag;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // `do_frag`
    //
    // do work for `relax`, `emit`, etc: update address and fragment offsets
    //
    //////////////////////////////////////////////////////////////////////////

    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::do_frag
            (core_fragment& frag, frag_iter_t const& frag_it, PROC_FN fn)
    {
        // NB: must copy `frag_iter_t`, don't use reference!
        auto it  = std::get<insn_iter>(frag_it);
        auto cnt = std::get<uint32_t>(frag_it);
        value_type::set_state(std::get<insn_state_t>(frag_it));
        do_frag(frag, it, cnt, fn);
    }

    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::do_frag
            (core_fragment& frag, insn_iter& it, uint32_t n, PROC_FN fn)
    {
        static const auto idx_label = opc::opc_label().index();
#define TRACE_DO_FRAG   3       // 1 = FRAG, 2 = INSN, 3 = RAW
#undef  TRACE_DO_FRAG

#ifdef  TRACE_DO_FRAG
        std::cout << "do_frag::begin: " << frag;
        std::cout  << ": " << frag.base_addr() << " + " << frag.size();
        if (frag.alignment())
            std::cout << " align = " << +frag.alignment();
        if (frag.is_relaxed())
            std::cout << " (relaxed)";
        std::cout << std::hex << " (" << frag.base_addr().min;
        std::cout << " / " << frag.base_addr().max << ")";
        std::cout << " count = " << std::dec << n;
        std::cout << std::dec << std::endl;
#endif
        
        dot.set_frag(frag);
        while (n--)
        {
            core_insn insn(*it);

#ifdef TRACE_DO_FRAG
#if    TRACE_DO_FRAG >= 2
            std::cout << "do_frag::dot.offset = " << dot.frag_offset();
            std::cout << " dot_delta = " << dot.cur_delta;
            std::cout << std::endl;
            std::cout << "processing: ";
#if    TRACE_DO_FRAG >= 3
            std::cout << std::endl;
            std::cout << "raw : ";
            insn.raw(std::cout);
            std::cout << std::endl;
            std::cout << "fmt : ";
#endif
            insn.fmt(std::cout);
            std::cout << std::endl;
#endif
#endif
            // update label with `dot` offset
            if (insn.opc_index == idx_label)
                insn.fixed().offset = dot.frag_offset();

// XXX ip->update knows old size. Could return delta for `dot.advance`
            auto old_size = insn.size();
            fn(insn, dot);
            auto new_size = insn.size();

            if (old_size != new_size)
                it->update(new_size);
            
            dot.advance(new_size, old_size);
            it->advance(insn);      // consume `data`
            ++it;                   // next insn
        }
#ifdef TRACE_DO_FRAG
        std::cout << "do_frag::loop end: " << frag;
        std::cout  << ": " << frag.base_addr() << " + " << frag.size() << std::endl;
        std::cout << "do_frag::loop end: dot.offset = " << dot.frag_offset() << std::endl;
#endif
        frag.set_size(dot.frag_offset());
#ifdef TRACE_DO_FRAG
        std::cout << "do_frag::end: " << frag;
        std::cout  << ": " << frag.base_addr() << " + " << frag.size();
        if (frag.is_relaxed())
            std::cout << " (relaxed)";
        auto e = frag.base_addr() + frag.size();
        std::cout << std::hex << " (" << e.min;
        std::cout << " / " << e.max << ")";
        std::cout << std::dec << std::endl;
        std::cout << std::endl;
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // inserter methods
    //
    //////////////////////////////////////////////////////////////////////////

    // inserter `ctor`
    template <typename Insn_Deque_t>
    insn_container<Insn_Deque_t>::
        insn_inserter::insn_inserter(insn_container& c)
            : c(c), bi(std::back_inserter(c.insns))
    {
        // initialize container with SEG (initial_segment)
        *this = {opc::opc_section(), c.initial_segment};
        c.first_frag_p = frag_p;
    }

    // inserter `dtor`: resolve symbols
    template <typename Insn_Deque_t>
    insn_container<Insn_Deque_t>::insn_inserter::~insn_inserter()
    {
        // apply hook at end of inserting
        // generally used to resolve local commons -> .bss, etc
        if (c.dtor_fn)
            c.dtor_fn(*this);
        
        // close index_list
        c.insn_index_list.emplace_back(c.insns.size());
    }
    
    // `inserter` primary method
    template <typename Insn_Deque_t>
    auto insn_container<Insn_Deque_t>::
        insn_inserter::operator=(core_insn&& insn) -> insn_inserter&
    {
        // get special opcode indexes
        static const auto idx_section = opc::opc_section().index();
        static const auto idx_org     = opc::opc_org()    .index();
        static const auto idx_align   = opc::opc_align()  .index();
        static const auto idx_label   = opc::opc_label()  .index();

        // generate container_data from insn
        value_type data{insn};
        
        // allocate new frag if current doesn't have room
        // NB: insn_size is instance variable which can be modified
        reserve(data.size());

        // see if special insn & process accordingly
        // if `dot` referenced: drop a label
        auto  opc_index = insn.opc_index;
        if (opc_index == idx_label)
            put_label(std::move(data));
        else if (core_addr_t::must_init_dot())
            *this = opc::opc_label();       // insert dummy label insn

        // extract size from `insn`
        insn_size = insn.data.size;
        
        if (opc_index == idx_section)
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
            c.dot.dot_offset += insn_size;
        }

        return *this;
    }

    // insert insn: generic instruction
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_insn(value_type&& data)
    {
        *bi++ = std::move(data);
    }

    // insert insn: label
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_label(value_type&& data)
    {
        // emit label insn & init `core_addr` instance
        // NB: if `label` is to be created at this location, 
        // `core_symbol::make_label()` method will have already been
        // called. This routine creates the `core_addr` infrastructure
        // to initialize `dot` at this location.

        // NB: don't allocate two `labels` at the same address. 
        // convert INSN to `nop`
        if (!core_addr_t::must_init_dot())
        {
            static opc::opc_nop opc{};
            data.set_opc_index(opc.index());
            put_insn(std::move(data));
            return;
        }

        // use "fixed" area of insn for `offset`...
        // ...and initialize frag_p
	
        put_insn(std::move(data));
        auto& fixed  = c.insns.back().fixed;
        fixed.offset = c.dot.frag_offset();     // init with first pass value
        
        // init `core:addr` instance for this location
        core_addr_t::cur_dot().init_addr(frag_p, &fixed.offset);
    }

    // insert insn: segment
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_segment(value_type&& data)
    {
        // get segment from insn & start new frag
        auto& segment = core_segment::get(data.fixed.fixed);
        auto  align   = segment.section().align();
        new_frag(align, &segment);
        return put_insn(std::move(data));
    }

    // insert insn: align
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_align(value_type&& data)
    {
        // alignment stored in fragment, so need a new frag.
        auto align = data.fixed.fixed;
        new_frag(align);
        put_insn(std::move(data));
    }

    // insert insn: org
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_org(value_type&& data)
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
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::new_frag(uint8_t align, core_segment *seg_p)
    {
        // update container accounting of fragments
        // NB: here, size is instruction index in container
        c.insn_index_list.emplace_back(c.insns.size());

        // make sure locations in previous frag remain in previous frag
        // NB: new segment might not be same, creating *total* error.
        // NB: new_dot() is no-op if old dot not previously referenced
        core_addr_t::new_dot();

#ifdef XXX
        // XXX could reduce relax a bit, but currently
        // screws up `frag.frag_is_relaxed` calculation
        // close old frag, recording it's size
        if (frag_p)
            frag_p->set_size(c.dot.frag_offset());
#endif
            
        // allocate new frag. possibly in new segment
        frag_p = &core_fragment::add(seg_p, align);

        // clear fragment tuning counts
        c.dot.set_frag(*frag_p);
        frag_insn_cnt  = 0;
        frag_relax_cnt = 0;
    }

    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::reserve(op_size_t const& op_size)
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
