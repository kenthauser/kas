#ifndef KAS_TARGET_TGT_INSN_EVAL_H
#define KAS_TARGET_TGT_INSN_EVAL_H

#include "tgt_insn.h"
#include "tgt_mcode.h"
#include <memory>

namespace kas::tgt
{

template <typename INSN_T>
auto tgt_insn_eval
        ( INSN_T const                         & insn
        , typename INSN_T::ok_bitset_t         & ok
        , typename INSN_T::mcode_t::argv_t     & args
        , typename INSN_T::mcode_t::stmt_info_t  stmt_info
        , core::opc::opcode::op_size_t         & insn_size
        , expression::expr_fits const          & fits
        , std::ostream                         * trace = {}
        ) -> typename INSN_T::mcode_t const *
{
    using namespace expression;     // import fits_result

    // save current "match"
    using mcode_t      = typename INSN_T::mcode_t;
    using op_size_t    = typename mcode_t::op_size_t;
    using stmt_info_t  = typename mcode_t::stmt_info_t;

    auto print_state = [&args](const char *desc)
        {
            std::cout << "tgt_insn_eval: " << desc << ": ";
            for (auto arg: args)
                std::cout << +arg.get_state() << ", ";
            std::cout << std::endl;
        };

    mcode_t const *mcode_p{};
    auto match_result = fits.no;
    auto match_index  = 0;

    trace = &std::cout;
    if (trace)
    {
        std::cout << "tgt_insn_eval: " << insn.name;
        std::cout << std::dec << " [" << insn.mcodes().size() << " mcodes]";
        for (auto& arg : args)
            std::cout << ", " << arg;
        std::cout << std::endl;
    }
        
    // "state" is argument modes
    using state_t = typename mcode_t::arg_t::arg_state;
    state_t initial_state[mcode_t::MAX_ARGS];
    using state_array = std::array<state_t, mcode_t::MAX_ARGS>;
    
    // create array of `states` & sizes (+1 to hold initial state)
    // NB: elements don't require init -- not referenced if not first written
    auto states = std::make_unique<state_array[]>(insn.mcodes().size() + 1);
    auto sizes  = std::make_unique<op_size_t  []>(insn.mcodes().size() + 1);

    // NB: index zero is initial values. others are `ok` index + 1
    auto save_results = [&args, &states, &sizes](unsigned index, op_size_t& size)
        {
            std::cout << "tgt_insn_eval::save_results: index = " << +index << std::endl;
            sizes[index] = size;
            
            auto p = states[index].begin();
            for (auto& arg : args)
                *p++ = arg.get_state();
         };
    
    auto update_modes = [&args, &states, &sizes](unsigned index)
        {
            std::cout << "tgt_insn_eval::update_results: index = " << +index << std::endl;
            // NB: argv_t has virtual method which takes array pointer
            args.update_modes(states[index].begin());
        };
    
    // loop thru "opcodes" until no more matches
    auto bitmask = ok.to_ulong();

    // if only 1 matching mcode, will not need to restore args
    if (ok.count() > 1)
        save_results(0, insn_size);

    // loop thru candidates & find best match
    bool first_iteration = true;        // if first time thru eval
    auto index = 0;
    for (auto op_iter = insn.mcodes().begin(); bitmask; ++op_iter, ++index)
    {
        bool op_is_ok = bitmask & 1;    // test if current was OK
        bitmask >>= 1;                  // set up for next 
        if (!op_is_ok)                  // loop if not ok
            continue;

        // don't restore on first iteration
        if (!first_iteration)
            update_modes(0);
        else
            first_iteration = false;    // no longer first interation

        // size for this opcode
        if (trace)
            *trace << std::dec << std::setw(2) << index << ": ";
        
        op_size_t size;
        print_state("before state");
       
        // NB: size method may modify global `args` & local `size`
        auto result = (*op_iter)->size(args, stmt_info, size, fits, trace);
        print_state("after state");
        
        if (result == fits.no)
        {
            ok.reset(index);
            continue;
        }
        
        // save results: modes & size (args is globl)
        save_results(index + 1, size);

        // Better match if new max better than current min
        if (result == fits.yes && insn_size.min > size.max)
        {
            // clear old match
            if (mcode_p)
                ok.reset(match_index);
            mcode_p = {};
        }
        
        // first "match" is always "current best"
        if (!mcode_p)
        {
            mcode_p      = *op_iter;
            match_index  = index;
            match_result = result;
            insn_size    = size;
            continue;
        } 

        // otherwise, see if this is better match
        // a "DOES" match is better than a "MIGHT" match
        if (match_result == fits.yes)
        {
            // see if new match is no better than old choices
            if (size.max >= insn_size.max && size.min >= insn_size.min)
            {
                // this match is no better than previous. Delete it.
                ok.reset(index);
                continue;
            }
        }

        // pick a smaller "does match"
        if (result == fits.yes)
        {
            if (insn_size.max > size.max)
                insn_size.max = size.max;

            // possibly downgrade current match based on previous best
            result = match_result;
        }

        // allow larger "might" match max if "maybe" in mix
        if (match_result != fits.yes)
        {
            if (insn_size.max < size.max)
                insn_size.max = size.max;
        }

        // always take the smallest "min"
        if (insn_size.min > size.min)
        {
            insn_size.min = size.min;
            match_result = result;
        }
    }

    // if no match, show error
    if (!mcode_p)
        insn_size.set_error();

    if (trace)
    {
        *trace << "candidates: " << ok.count();
        *trace << " : " << ok.to_string().substr(ok.size()-insn.mcodes().size());
        *trace << " : size = "   << insn_size;
        *trace << '\n' << std::endl;
    }

    // if found a single match, update global args
    if (ok.count() == 1)
    {
        // update state & retrieve saved size
        update_modes(match_index+1);
        insn_size = sizes[match_index+1];
    }
    
    print_state("final_state");
    return mcode_p;
}

// definition of `insn::eval` with almost all args templated.
// NB because of "order of declaration", `tgt_insn_t` don't know 
// defns of `arg_t`, argv_t`, and `stmt_t`. 

// NB: the `tgt_insn_eval` function (above) resolves templated args &
// errors out on mismatch
template <typename O, typename T, typename B
        , unsigned A, unsigned M, unsigned N, typename I>
template <typename...Ts>
auto tgt_insn_t<O, T, B, A, M, N, I>::
        eval(ok_bitset_t& ok, Ts&&...args) const
        -> mcode_t const *
{
    return tgt_insn_eval(*this, ok, std::forward<Ts>(args)...);
}

}


#endif
