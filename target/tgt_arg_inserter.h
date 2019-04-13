#ifndef KAS_TARGET_TGT_ARG_INSERTER_H
#define KAS_TARGET_TGT_ARG_INSERTER_H

// serialize a single argument
//
// for each argument, save the "mode", register number, and data
//
// If possible, save register number (and mode) in actual "opcode" data.
// If needed, save extra data.
// Data can be stored as "fixed" constant, or "expression" 

#include "kas_core/opcode.h"
#include "utility/align_as_t.h"

namespace kas::tgt::opc::detail
{

// Special (byte) type to hold info about an single argument
// in the insn_data deque(). The info for each "arg" is 8-bits
// some of which indicate extension words follow.

// arg_mode holds the `arg_t` mode enum
// `has_expr` is true if argument is `expr_t`, false for fixed
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
    // make sure `arg_mode_t` can fit in subfield
    static_assert(MCODE_T::arg_t::arg_mode_t::NUM_ARG_MODES <= (1<< arg_info_t::MODE_FIELD_SIZE)
                , "too many `arg_mode` enums");

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

template <typename MCODE_T, typename Inserter>
void insert_one (Inserter& inserter
                    , unsigned n
                    , detail::arg_info_t *p
                    , typename MCODE_T::arg_t& arg
                    , uint8_t sz
                    , typename MCODE_T::fmt_t const& fmt
                    , typename MCODE_T::val_t const *val_p
                    , typename MCODE_T::mcode_size_t   *code_p
                    )
{
    // write arg data into machine code if possible (dependent on validator)
    // returns false if no validator
    bool completely_saved = fmt.insert(n, code_p, arg, val_p);

//#define TRACE_ARG_SERIALIZE
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg << ": insert = " << std::boolalpha << completely_saved << std::endl;
#endif

    // write arg data as immediate as required (NB: result is a mutable arg)
    p->has_data = !completely_saved;
    p->has_expr = arg.serialize(inserter, sz, p);
    p->arg_mode = arg.mode();

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg;
    std::cout << ": mode = "   << std::dec << std::setw(2) << +p->arg_mode;
    std::cout << " bits: "     << +p->has_data << "/" << +p->has_expr;
    std::cout << std::endl;
#endif
}


// deserialize arguments: for format, see above
template <typename MCODE_T, typename Reader>
void extract_one(Reader& reader
                    , unsigned n
                    , detail::arg_info_t *p
                    , typename MCODE_T::arg_t *arg_p
                    , uint8_t sz
                    , typename MCODE_T::fmt_t const& fmt
                    , typename MCODE_T::val_t const *val_p
                    , typename MCODE_T::mcode_size_t   *code_p
                    )
{
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\n[read_one:  mode = " << std::dec << std::setw(2) << +p->arg_mode;
    std::cout << " bits: " << +p->has_data << "/" << +p->has_expr;
#endif

    // extract arg from machine code (dependent on validator)
    // NB: extract may look at arg mode.
    arg_p->set_mode(p->arg_mode);

    if (val_p)
    {
        fmt.extract(n, code_p, arg_p, val_p);

        // NB: don't allow extract to change mode
        arg_p->set_mode(p->arg_mode);
    }

    // extract additional info as required
    arg_p->extract(reader, sz, p);
#ifdef TRACE_ARG_SERIALIZE
    std::cout << " -> " << *arg_p << "] ";;
#endif
}

#undef TRACE_ARG_SERIALIZE
}


#endif
