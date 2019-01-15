#ifndef KAS_TARGET_TGT_ARG_SERIALIZE_H
#define KAS_TARGET_TGT_ARG_SERIALIZE_H

// serialize a single argument
//
// for each argument, save the "mode", register number, and data
//
// If possible, save register number (and mode) in actual "opcode" data.
// If needed, save extra data.
// Data can be stored as "fixed" constant, or "expression" 

//#include "z80/z80_arg.h"
//#include "z80/z80_formats_type.h"
#include "kas_core/opcode.h"
#include "utility/align_as_t.h"


namespace kas::tgt::opc::detail
{

// Special (byte) type to hold info about an single argument
// in the insn_data deque(). The info for each "arg" is 8-bits
// some of which indicate extension words follow.

// arg_mode holds the `arg_t` mode enum
// `has_*_expr` is true if argument is `expr_t`, false for fixed
struct arg_info_t
{
    static constexpr std::size_t MODE_FIELD_SIZE = 6;
    
    uint8_t arg_mode : MODE_FIELD_SIZE;
    uint8_t has_data : 1;       // additional data stored
    uint8_t has_expr : 1;       // data stored as expression
};

// store as many infos as will fit in `mcode` word
template <typename MCODE_T>
struct tgt_arg_info : kas::detail::alignas_t<tgt_arg_info<MCODE_T>, typename MCODE_T::mcode_size_t>
{
    // XXX
    //static_assert(MCODE_T::arg_t::NUM_ARG_MODES <= (1<< arg_info_t::MODE_FIELD_SIZE)
    //            , "too many `arg_mode` enums");

    // public interface: where & how many
    constexpr static auto ARGS_PER_INFO = sizeof(typename MCODE_T::mcode_size_t)/sizeof(arg_info_t);
    auto begin()
    {
        return std::begin(info);
    }
private:
    // create "args_per_info" items 
    arg_info_t  info[ARGS_PER_INFO];
};

//#define TRACE_ARG_SERIALIZE
#ifndef TRACE_ARG_SERIALIZE
#define TRACE_ARG_SERIALIZE 0
#endif

template <typename MCODE_T, typename Inserter>
void insert_one (Inserter& inserter
                    , unsigned n
                    , detail::arg_info_t *p
                    , typename MCODE_T::arg_t& arg
                    , typename MCODE_T::fmt_t const& fmt
                    , typename MCODE_T::val_t const *val_p
                    , typename MCODE_T::mcode_size_t   *code_p
                    )
{
    // write arg data into machine code if possible (dependent on validator)
    auto result = fmt.insert(n, code_p, arg, val_p);

    // write arg data as immediate as required (NB: result is a mutable arg)
    p->has_expr = arg.serialize(inserter, result);
    p->has_data = !result;
    p->arg_mode = arg.mode();

#if TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg;
    std::cout << ": mode = " << std::setw(2) << +p->arg_mode;
    std::cout << " bits: " << +p->has_data << "/" << +p->has_expr;
    std::cout << std::endl;
#endif
}


// deserialize arguments: for format, see above
template <typename MCODE_T, typename Reader>
void extract_one(Reader& reader
                    , unsigned n
                    , detail::arg_info_t *p
                    , typename MCODE_T::arg_t *arg_p
                    , typename MCODE_T::fmt_t const& fmt
                    , typename MCODE_T::val_t const *val_p
                    , typename MCODE_T::mcode_size_t   *code_p
                    )
{
#if TRACE_ARG_SERIALIZE
    std::cout << "read_one:  mode = " << std::setw(2) << +p->arg_mode;
    std::cout << " bits: " << +p->has_data << "/" << +p->has_expr;
#endif

    // extract arg from machine code (dependent on validator)
    // XXX needed??
    arg_p->set_mode(p->arg_mode);
    fmt.extract(n, code_p, arg_p, val_p);
    arg_p->set_mode(p->arg_mode);           // restore known mode

    // extract immediate arg as required
    arg_p->extract(reader, p->has_data, p->has_expr);
#if TRACE_ARG_SERIALIZE
    std::cout << " -> " << *arg_p << std::endl;
#endif
}

#undef TRACE_ARG_SERIALIZE
}


#endif
