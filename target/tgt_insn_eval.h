#ifndef KAS_TARGET_TGT_INSN_EVAL_H
#define KAS_TARGET_TGT_INSN_EVAL_H

#include "tgt_insn.h"

namespace kas::tgt
{

template <typename INSN_T, typename OK_T, typename ARGS_T>
auto eval_insn_list
        ( INSN_T const& insn
        , OK_T& ok
        , ARGS_T& args
        , core::opc::opcode::op_size_t& insn_size
        , expression::expr_fits const& fits
        , std::ostream* trace
        ) -> typename INSN_T::mcode_t const *
{
    using namespace expression;     // get fits_result

    // save current "match"
    using mcode_t   = typename INSN_T::mcode_t;
    using op_size_t = typename mcode_t::op_size_t;

    mcode_t const *mcode_p{};
    auto match_result = fits.no;
    auto match_index  = 0;

    if (trace)
    {
        std::cout << "tgt_insn::eval: " << insn.name;
        std::cout << std::dec << " [" << insn.mcodes.size() << " mcodes]";
        for (auto& arg : args)
            std::cout << ", " << arg;
        std::cout << std::endl;
    }
        
    // loop thru "opcodes" until no more matches
    auto bitmask = ok.to_ulong();
    auto index = 0;
    for (auto op_p = insn.mcodes.begin(); bitmask; ++op_p, ++index)
    {
        bool op_is_ok = bitmask & 1;    // test if current was OK
        bitmask >>= 1;                  // set up for next
        if (!op_is_ok)                  // loop if not ok
            continue;

        // size for this opcode
        if (trace)
            *trace << std::dec << std::setw(2) << index << ": ";
        
        op_size_t size;
        auto result = (*op_p)->size(args, size, fits, trace);

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
            mcode_p      = *op_p;
            insn_size    = size;
            match_result = result;
            match_index  = index;
            continue;
        } 
       
        // otherwise, see if this is better match
        // a "DOES" match is better than a "MIGHT" match
        if (match_result == fits.yes)
        {
            // see if new match is no better than old choices
            if (size.max >= insn_size.max || size.min >= insn_size.min)
            {
                // this match is no better than previous. Delete it.
                ok.reset(index);
                continue;
            }
        }

        // pick a smaller "does match"
        if (result == fits.yes)
        {
            if (insn_size.max < size.max)
                insn_size.max = size.max;

            // possibly downgrade match based on current match
            result = match_result;
        }

        // allow larger "might" match max 
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

    if (trace) {
        *trace << "candidates: " << ok.count();
        *trace << " : " << ok.to_string().substr(ok.size()-insn.mcodes.size());
        *trace << " : size = "   << insn_size;
        *trace << '\n' << std::endl;
    }

    return mcode_p;
}

// templated definition to cut down on noise in `tgt_insn_t` defn
template <typename MCODE_T, typename TST_T, unsigned MAX_MCODES, typename INDEX_T>
template <typename...Ts>
auto tgt_insn_t<MCODE_T, TST_T, MAX_MCODES, INDEX_T>
        ::eval(bitset_t& ok, Ts&&...args) const -> mcode_t const *
{
    return eval_insn_list(*this, ok, std::forward<Ts>(args)...);
}

}


#endif
