#ifndef KAS_TARGET_TGT_INSN_EVAL_H
#define KAS_TARGET_TGT_INSN_EVAL_H

#include "tgt_insn.h"
//#include "expr/expr.h"

namespace kas::tgt
{

#if 1
template <typename INSN_T, typename OK_T, typename ARGS_T>
core::opc::opcode const* eval_insn_list
        ( INSN_T const& insn
        , OK_T& ok
        , ARGS_T& args
        , core::opc::opcode::op_size_t& insn_size
        , expression::expr_fits const& fits
        , std::ostream* trace
        )
{
    using namespace expression;     // get fits_result

    // save current "match"
    using mcode_t = typename INSN_T::opcode_t;
    mcode_t const *opcode_p{};
    auto match_result = fits.no;
    auto match_index  = 0;

    if (trace)
    {
        std::cout << "eval: " << insn.name() << " [" << insn.opcodes.size() << " opcodes]";
        for (auto& arg : args)
            std::cout << ", " << arg;
        std::cout << std::endl;
    }
        
    // loop thru "opcodes" until no more matches
    auto bitmask = ok.to_ulong();
    auto index = 0;
    for (auto op_p = insn.opcodes.begin(); bitmask; ++op_p, ++index)
    {
        bool op_is_ok = bitmask & 1;    // test if current was OK
        bitmask >>= 1;                  // set up for next
        if (!op_is_ok)                  // loop if not ok
            continue;

        // size for this opcode
        if (trace)
            *trace << std::setw(2) << index << ": ";
        
        core::opc::opcode::op_size_t size;
        auto result = (*op_p)->size(args, size, fits, trace);

        if (result == fits.no)
        {
            ok.reset(index);
            continue;
        }
        
        // first "match" is always "current best"
        if (!opcode_p)
        {
            opcode_p     = *op_p;
            insn_size    = size;
            match_result = result;
            match_index  = index;
        } 
        
        // otherwise, see if this is better match
        // a "DOES" match is better than a "MIGHT" match
        else if (result == fits.yes)
        {
            // if new max better than current min, replace old
            if (size.max < insn_size.min)
            {
                ok.reset(match_index);
                opcode_p     = *op_p;
                insn_size    = size;
                match_result = result;
                match_index  = index;
                continue;       // on to next opcode
            }

            // new `max` is this opcode
            if (insn_size.max > size.max)
                insn_size = size.max;
        }

        // otherwise a "MIGHT" match. if previous is also "MIGHT" update MIGHT
        else
        {
            if (insn_size.max < insn_size.max)
                insn_size.max = size.max;
        }

        // always take the smallest "min"
        if (insn_size.min > size.min)
            insn_size.min = size.min;
    }

    // if no match, show error
    if (!opcode_p)
        insn_size.set_error();

    if (trace) {
        *trace << "candidates: " << ok.count();
        *trace << " : " << ok.to_string().substr(ok.size()-insn.opcodes.size());
        *trace << " : size = "   << insn_size;
        *trace << '\n' << std::endl;
    }

    return opcode_p;
}


// templated definition to cut down on noise in `insn_t` defn
template <typename OPCODE_T, std::size_t _MAX_ARGS, std::size_t MAX_OPCODES, typename TST_T>
template <typename...Ts>
core::opcode const* tgt_insn_t<OPCODE_T, _MAX_ARGS, MAX_OPCODES, TST_T>::eval(insn_bitset_t& ok, Ts&&...args) const
{
    return eval_insn_list(*this, ok, std::forward<Ts>(args)...);
}
#endif
}


#endif
