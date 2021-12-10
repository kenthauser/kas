#ifndef KAS_TARGET_TGT_INSN_EVAL_H
#define KAS_TARGET_TGT_INSN_EVAL_H

#include "tgt_insn.h"

namespace kas::tgt
{

template <typename INSN_T, typename OK_T, typename ARGS_T, typename STMT_INFO_T>
auto eval_insn_list
        ( INSN_T const& insn
        , OK_T& ok
        , ARGS_T& args
        , STMT_INFO_T& stmt_info
        , core::opc::opcode::op_size_t& insn_size
        , expression::expr_fits const& fits
        , std::ostream* trace
        ) -> typename INSN_T::mcode_t const *
{
    using namespace expression;     // get fits_result

    // save current "match"
    using mcode_t      = typename INSN_T::mcode_t;
    using op_size_t    = typename mcode_t::op_size_t;
    using stmt_info_t  = typename mcode_t::stmt_info_t;

    // "state" is argument modes
    using state_t = typename mcode_t::arg_t::arg_state;
    state_t initial_state[mcode_t::MAX_ARGS];
    
    auto print_state = [&args](const char *desc)
        {
            std::cout << "eval_insn_list: " << desc << ": ";
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
        std::cout << "tgt_insn::eval: " << insn.name;
        std::cout << std::dec << " [" << insn.mcodes().size() << " mcodes]";
        for (auto& arg : args)
            std::cout << ", " << arg;
        std::cout << std::endl;
    }
        
    // loop thru "opcodes" until no more matches
    auto bitmask = ok.to_ulong();

    if (ok.count() > 1)
    {
        auto p = initial_state;
        for (auto& arg : args)
            *p++ = arg.get_state();

        print_state("initial state");
    }

    bool state_updated {};      // both start false
    bool needs_restore {};

    // loop thru candidates & find best match
    auto index = 0;
    for (auto op_iter = insn.mcodes().begin(); bitmask; ++op_iter, ++index)
    {
        bool op_is_ok = bitmask & 1;    // test if current was OK
        bitmask >>= 1;                  // set up for next 
        if (!op_is_ok)                  // loop if not ok
            continue;

        // don't restore on first iteration
        if (needs_restore)
        {
            //state_updated = false;
            auto p = initial_state;
            for (auto& arg : args)
                arg.set_state(*p++);
        }
        needs_restore = true;
        
        // size for this opcode
        if (trace)
            *trace << std::dec << std::setw(2) << index << ": ";
        
        op_size_t size;
        print_state("before state");
       
        // NB: size `binds` info & may modify global arg
        auto result = (*op_iter)->size(args, stmt_info, size, fits, trace);
        print_state("after state");
        
        if (result == fits.no)
        {
            ok.reset(index);
            continue;
        }
        
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
            state_updated = true;       // use state from this iteration
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

    // if found a single match, update args
    if (ok.count() == 1)
    {
        // if state modified, need to rerun size. sigh.
        if (state_updated)
        {
            std::cout << "eval_insn_list: restore initial state: ";
        
            // restore state
            auto p = initial_state;
            for (auto& arg: args)
                arg.set_state(*p++);

            // rerun size
            op_size_t size;
            
            // NB: size `binds` info. modifies passed `size` arg.
            mcode_p->size(args, stmt_info, size, fits, nullptr);
        }
    
    }
    
    print_state("final_state");
    return mcode_p;
}

// templated definition to cut down on noise in `tgt_insn_t` defn
template <typename O, typename T, typename B, unsigned M, unsigned N, typename I>
template <typename...Ts>
auto tgt_insn_t<O, T, B, M, N, I>::
        eval(bitset_t& ok, Ts&&...args) const
        -> mcode_t const *
{
    return eval_insn_list(*this, ok, std::forward<Ts>(args)...);
}

}


#endif
