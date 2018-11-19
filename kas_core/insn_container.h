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

#include "core_insn.h"
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

    template <typename Insn_Deque_t = std::deque<core_insn>>
    struct insn_container : kas_object<insn_container<Insn_Deque_t>>
    {
        using base_t    = kas_object<insn_container<Insn_Deque_t>>;
        using op_size_t = typename opc::opcode::op_size_t;

        using insn_iter = typename Insn_Deque_t::iterator;
        using PROC_FN = std::function<void(insn_iter&, core_expr_dot const&)>;

        // insn_iter & count for iterating a frag
        using frag_iter_t = std::pair<insn_iter, uint32_t>;

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
            data_initial = core_insn::data.size();
            return insn_inserter(*this);
        }
        
        // getter for container data
        static auto& data()
        {
            static auto data_ = new std::deque<expr_t>;
            return *data_;
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
            data().clear();
        }
    // XXX temp
    // private:
    public:
        core_segment& initial_segment;
        DTOR_FN       dtor_fn{};

        // first frag for this container
        core_fragment const *first_frag_p {};
        
        void do_frag(core_fragment&, insn_iter&, uint32_t, PROC_FN);
        Insn_Deque_t insns;
        decltype(core_insn::data)::size_type data_initial = -1;
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
        insn_inserter& operator=(core_insn&&);
        auto& operator++()    { return *this; }
        auto& operator++(int) { return *this; }
        auto& operator*()     { return *this; }

        void new_frag(core_segment *seg_p = {});
      
    // XXX temp
    //private:
        // references to associated container
        insn_container &c;
        std::back_insert_iterator<Insn_Deque_t> bi;
        core_segment *lcomm_segment_p{};
        core_fragment *frag_p {};
        op_size_t      op_size;     // size of current insn

        // insn types (generic & special requirements)
        void put_insn(core_insn&&);
        void put_label(core_insn&&);
        void put_segment(core_insn&&);
        void put_align(core_insn&&);
        void put_org(core_insn&&);

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

        auto insn_iter = insns.begin();
        auto it = insn_index_list.begin();

        auto proc_one_frag = [&](auto& frag)
            {
                auto count = it[1] - it[0];
                if (must_generate_iters)
                    insn_iters->emplace_back(insn_iter, count);
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

        // NB: must copy `frag_iter_t`, don't use reference!
        unsigned index = frag.frag_num() - first_frag_p->frag_num();
        if (index < insn_iters->size()) {
            auto iter_info = (*insn_iters)[index];
            do_frag(frag, iter_info.first, iter_info.second, fn);
        }
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
            (core_fragment& frag, insn_iter& it, uint32_t n, PROC_FN fn)
    {
        static const auto idx_label = opc::opc_label().index();
#define TRACE_DO_FRAG   3       // 1 = FRAG, 2 = INSN, 3 = RAW
//#undef  TRACE_DO_FRAG
        
#ifdef  TRACE_DO_FRAG
        std::cout << "do_frag::begin: " << frag;
        std::cout  << ": " << frag.base_addr() << " + " << frag.size();
        if (frag.alignment())
            std::cout << " align = " << std::to_string(frag.alignment());
        std::cout << std::hex << " (" << frag.base_addr().min;
        std::cout << " / " << frag.base_addr().max << ")";
        std::cout << " count = " << std::dec << n;
        std::cout << std::dec << std::endl;
#endif
        
        dot.set_frag(frag);
        while (n--) {
#ifdef TRACE_DO_FRAG
#if    TRACE_DO_FRAG >= 2
            std::cout << "do_frag::dot.offset = " << dot.frag_offset();
            std::cout << " dot_delta = " << dot.cur_delta;
            std::cout << std::endl;
            std::cout << "processing: ";
#if    TRACE_DO_FRAG >= 3
            std::cout << std::endl;
            std::cout << "raw : ";
            it->raw(std::cout);
            std::cout << std::endl;
            std::cout << "fmt : ";
#endif
            it->fmt(std::cout);
            std::cout << std::endl;
#endif
#endif
            // update label with `dot` offset
            if (it->index == idx_label) {
                auto& offset = it->get_fixed().offset;
                it->get_fixed().offset = dot.frag_offset();
            }

            auto old_size = it->size;
            fn(it, dot);
            dot.advance(it->size, old_size);
            ++it;
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
        put_segment(opc::opc_section(c.initial_segment));
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

        // allocate new frag if current doesn't have room
        // NB: op_size is instance variable which can be modified
        op_size = insn.get_size();
        reserve(op_size);

        // see if special insn & process accordingly
        // if `dot` referenced: drop a label
        if (insn.index == idx_label)
            put_label(std::move(insn));
        else if (core_addr::must_init_dot())
            put_label(opc::opc_label());

        // XXX ripe for optimization: vector of fns + put_insn()
        if (insn.index == idx_section)
            put_segment(std::move(insn));
        else if (insn.index == idx_org)
            put_org(std::move(insn));
        else if (insn.index == idx_align)
            put_align(std::move(insn));
        else if (insn.index != idx_label)
            put_insn(std::move(insn));

        // move dot if object code emitted data
        if (op_size.max) {
            core_addr::new_dot();
            c.dot.dot_offset += op_size;
        }

        return *this;
    }

    // insert insn: generic instruction
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_insn(core_insn&& insn)
    {
        //std::cout << "core_insn::put: first = " << std::dec << insn.first << ", cnt = " << insn.cnt << std::endl;
        *bi++ = std::move(insn);
    }

    // insert insn: label
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_label(core_insn&& insn)
    {
        // emit label insn & init `core_addr` instance

        // if insn reference is non-zero, init the specified `addr`
        // unless current dot is already allocated. In that case,
        // init referenced addr from current dot.

        // NB: insn is temporary. don't take reference
        auto  lbl_ref = insn.get_fixed().addr;
        auto& cur_dot = core_addr::cur_dot();

        // use "fixed" area of insn for `offset`...
        // ...and initialize frag_p
	
        put_insn(std::move(insn));
        auto& fixed  = c.insns.back().get_fixed();
        fixed.offset = c.dot.frag_offset();
#if 0
        // if `cur_dot` not allocated, use `lbl_ref`
        if (cur_dot.empty()) {
            cur_dot = lbl_ref;
            lbl_ref = {};
        }
#endif       
        // init actual address
        cur_dot.init_addr(frag_p, &fixed.offset);
#if 0
        if (lbl_ref)
            lbl_ref.get().init_addr(frag_p, &fixed.offset);
#endif
    }

    // insert insn: segment
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_segment(core_insn&& insn)
    {
        // get segment from insn & start new frag
        auto& segment = core_segment::get(insn.get_fixed().fixed);
        new_frag(&segment);
        frag_p->set_alignment(segment.section().align());
        return put_insn(std::move(insn));
    }

    // insert insn: align
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_align(core_insn&& insn)
    {
        // alignment stored in fragment, so need a new frag.
        auto align = insn.get_fixed().fixed;
        new_frag();
        frag_p->set_alignment(align);
        put_insn(std::move(insn));
    }

    // insert insn: org
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::put_org(core_insn&& insn)
    {
        // determine if new "org" address is org or skip...
        auto& target = core_insn::data[insn.first];
        
        // calculate displacement to see if `skip`
        // NB: can't be skip if `dot` not previously allocated
        // XXX if (!core_addr::cur_dot().empty()) {
        if (!core_addr::cur_dot().empty()) {
            auto& disp =  target - core_addr::get_dot();
            if (auto skip = disp.get_fixed_p()) {
                // create skip insn...
                insn.set_index(opc::opc_skip().index());
                insn.set_size(*skip);
                target = disp;
                return put_insn(std::move(insn));
            }
        }

        // `org` requires fixed address past current address...
        if (auto p = target.get_fixed_p()) {
            new_frag();
            auto offset = frag_p->set_org(*p);
            if (offset.is_error()) {
                auto& err = parser::kas_diag::error("can't org backwards");
                insn.get_fixed().fixed = err.index();
            } else {
                // XXX fix when relax loop installed
                if (offset.is_relaxed())
                    offset = {0, 4};
                op_size = offset;
                insn.set_size(offset);
            }
        } else {
            // error -- need fixed target address 
            auto& err = parser::kas_diag::error("org requires fixed address");
            insn.get_fixed().fixed = err.index();
        }
        return put_insn(std::move(insn));
    }

    //
    // inserter support routines
    //

    // NB: default (nullptr) is create frag in same segment
    template <typename Insn_Deque_t>
    void insn_container<Insn_Deque_t>::
        insn_inserter::new_frag(core_segment *seg_p)
    {
        // update container accounting of fragments
        // NB: here, size is instruction index in container
        c.insn_index_list.emplace_back(c.insns.size());

        // make sure locations in previous frag remain in previous frag
        // NB: new segment might not be same, creating *total* error.
        // NB: new_dot() is no-op if old dot not previously referenced
        core_addr::new_dot();
        
        // close old frag, recording it's size
        if (frag_p)
            frag_p->set_size(c.dot.frag_offset());

        // allocate new frag. possibly in new segment
        frag_p = &core_fragment::add(seg_p);

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
