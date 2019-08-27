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

// The default `tgt_arg_t` has three fields: `mode`, `reg`, and `expr`
// The default `arg_info_t` initializes all three.
//
// The member names (ie has_*) is written from the perspective of the
// `deserializer`. They are also passed to the `serializer` by the
// `inserter` as a guide as to what must be saved.
//
// As parameters to the `serializer` the bits have the following meanings:
//
// `has_reg` : validator not present. All validators store register info in
//             `opcode`, so this bit indicates either a `LIST` validator didn't
//             support specific register, or the `arg_num` too high for `LIST`
//
// `has_data` : complement of the `completely saved` bit returned by `inserter`.
//              Generally calculated by `validator`
//              

struct arg_info_t
{
    using value_t = uint16_t;
    static constexpr std::size_t MODE_FIELD_SIZE = 5;

    value_t init_mode : MODE_FIELD_SIZE;    // mode when serialized
    value_t cur_mode  : MODE_FIELD_SIZE;    // current mode
    value_t has_reg   : 1;                  // register stored
    value_t has_data  : 1;                  // additional data stored
    value_t has_expr  : 1;                  // data stored as expression
    value_t xtra_info : 3;                  // undefined arg data: must be const
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
               , typename MCODE_T::stmt_info_t const& info
               , typename MCODE_T::fmt_t const&  fmt
               , typename MCODE_T::val_t const  *val_p
               , typename MCODE_T::mcode_size_t *code_p
               )
{
    // write arg data into machine code if possible (dependent on validator)
    // returns false if no validator
    bool completely_saved = fmt.insert(n, code_p, arg, val_p);
//#define TRACE_ARG_SERIALIZE
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg << ": completely_saved = " << std::boolalpha << completely_saved;
    std::cout << " has_validator = " << bool(val_p) << std::endl;
#endif

    // write arg data
    // NB: `serialize` can destroy arg.
    p->cur_mode = p->init_mode = arg.mode();
    p->has_reg  = !val_p;
    p->has_data = !completely_saved;
    p->has_expr = arg.serialize(inserter, info, p);

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg;
    std::cout << ": mode = "   << std::dec << std::setw(2) << +p->init_mode;
    std::cout << " bits: "     << +p->has_reg << "/" << +p->has_data << "/" << +p->has_expr;
    std::cout << std::endl;
#endif
}


// deserialize arguments: for format, see above
template <typename MCODE_T, typename Reader, typename WB_Info>
void extract_one(Reader& reader
                    , unsigned n
                    , WB_Info& wb_info
                    , typename MCODE_T::arg_t& arg
                    , uint8_t sz
                    , typename MCODE_T::fmt_t const& fmt
                    , typename MCODE_T::val_t const *val_p
                    , typename MCODE_T::mcode_size_t   *code_p
                    )
{
    auto p = wb_info.info_p;
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\n[read_one:  mode = " << std::dec << std::setw(2) << +p->init_mode;
    std::cout << " bits: " << +p->has_reg  << "/" << +p->has_data << "/" << +p->has_expr;
#endif

    // extract arg from machine code (dependent on validator)
    // extract info from `opcode`
    if (val_p)
        fmt.extract(n, code_p, arg, val_p);

    // extract additional info as required
    // NB: extract may look at arg mode. Set to mode when serialized
    arg.set_mode(p->init_mode);
    arg.extract(reader, sz, p, &wb_info.arg_wb_p);
    
    // update mode to current value
    arg.set_mode(p->cur_mode);

#ifdef TRACE_ARG_SERIALIZE
    if (wb_info.arg_wb_p)
    {
        std::cout << " arg_wb: " << wb_info.arg_wb_p;
        if constexpr (!std::is_same_v<decltype(wb_info.arg_wb_p), void *>)
            std::cout << " = " << *wb_info.arg_wb_p; 
    }
    std::cout << " -> " << arg << "] ";;
#endif
}

#undef TRACE_ARG_SERIALIZE
}


#endif
