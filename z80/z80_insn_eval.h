#ifndef KAS_Z80_INSN_EVAL_H
#define KAS_Z80_INSN_EVAL_H



namespace kas::z80::opc
{


template <typename ARGS_T>
z80_opcode_t const* eval_insn_list
        ( z80_insn_t const& insn
        , z80_insn_t::insn_bitset_t& ok
        , ARGS_T& args
        , op_size_t& insn_size
        , expression::expr_fits const& fits
        , std::ostream* trace
        )
{
    using namespace expression;     // get fits_result

    // save current "match"
    z80_opcode_t const *opcode_p{};
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
        
        op_size_t size;
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
}


#endif
