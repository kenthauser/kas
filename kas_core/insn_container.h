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
#include "insn_inserter.h"
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

    template <typename Insn_Deque_t = std::deque<insn_container_data>>
    struct insn_container : kas_object<insn_container<Insn_Deque_t>>
    {
        using base_t      = kas_object<insn_container<Insn_Deque_t>>;
        using value_type  = typename Insn_Deque_t::value_type;
        using op_size_t   = typename opc::opcode::op_size_t;

        using insn_iter   = typename Insn_Deque_t::iterator;
        using PROC_FN     = std::function<void(core_insn&, core_expr_dot const&)>;

        // insn_iter, insn_state & count for iterating a frag
        using initial_state_t = typename value_type::initial_state_t;
        using insn_state_t    = typename value_type::state_t;
        using frag_iter_t     = std::tuple<insn_iter, insn_state_t, uint32_t>;

        using base_t::for_each;

        // declare inserter -- implement below.
        using inserter_t = insn_inserter<value_type>;

        // declare method for end-of-parse (eg: resolve symbols)
        using AT_END_FN = std::function<void(inserter_t&)>;

        insn_container(core_segment& initial, AT_END_FN fn = {}) 
                : initial_segment(initial)
                , at_end(fn)
                , initial_state(value_type::get_initial_state())
                {}

        // create container inserter
        auto inserter()
        {
            return inserter_t(*this);
        }
        
        void set_deferred_ops(core_section::deferred_ops& ops)
        {
            deferred_ops_p = &ops;
        }

        // getter for dot(). used in `core_relax.h`
        auto& get_insn_dot() const
        {
            return dot;
        }

        // methods for processing container data
        core_fragment& proc_frag(core_fragment& frag, PROC_FN fn);
        void proc_all_frags(PROC_FN fn);

    public:
        friend base_t;

        // inserter callbacks: `back_inserter`, `reserve`, `at_end`
        static value_type& cb_bi(void *cb, value_type&& value)
        {
            auto& c = *static_cast<insn_container *>(cb);
            return c.insns.emplace_back(std::move(value));
        }
    
        static void cb_end_frag(void *cb)
        {
            auto& c = *static_cast<insn_container *>(cb);
            c.insn_index_list.emplace_back(c.insns.size());
        }

        static void cb_at_end(void *cb, inserter_t& inserter)
        {
            auto& c = *static_cast<insn_container *>(cb);
            if (c.at_end)
                c.at_end(inserter);
            c.insn_index_list.emplace_back(c.insns.size());
        }

    
    // XXX temp
    // private:
    public:
        initial_state_t initial_state;        // initial state of opcode_data, etc
        core_segment&   initial_segment;
        AT_END_FN         at_end{};

        // deferred generation attributes
        core_section::deferred_ops *deferred_ops_p {};

        // first frag for this container
        core_fragment const *first_frag_p {};
        
        void do_frag(core_fragment&, insn_iter&, uint32_t, PROC_FN);
        void do_frag(core_fragment&, frag_iter_t const&  , PROC_FN);

        // insns for this container
        Insn_Deque_t insns;
        
        std::deque<uint32_t> insn_index_list;
        std::vector<frag_iter_t> *insn_iters{};
        core_expr_dot dot;
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

        if (must_generate_iters)
        {
            insn_iters = new std::vector<frag_iter_t>();
            insn_iters->reserve(insn_index_list.size() - 1);
        }

        // proc_all: reset insn_container values to initial states
        //value_type::reinit();
        value_type::set_initial_state(initial_state);

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

        // process all frags in this container
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
//#undef  TRACE_DO_FRAG

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
}
#endif
