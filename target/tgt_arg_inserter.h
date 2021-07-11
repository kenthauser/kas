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

// declare meta-data value stored when serialized
// XXX struct arg_serial_t : alignas_t<arg_serial_t, uint16_t>
struct arg_serial_t : alignas_t<arg_serial_t, uint8_t>
{
    static constexpr std::size_t MODE_FIELD_SIZE = 5;

    // XXX arg_serial_t(value_t mode = {}) : init_mode(mode), cur_mode(mode) {}
    arg_serial_t(value_t mode = {}) : init_mode{mode} {}

    value_t init_mode : MODE_FIELD_SIZE;    // mode when serialized
    value_t has_reg   : 1;                  // register stored
    value_t has_data  : 1;                  // additional data stored
    value_t has_expr  : 1;                  // data stored as expression
// XXX    value_t cur_mode  : MODE_FIELD_SIZE;    // current mode
// XXX    value_t xtra_info : 3;                  // undefined arg data: must be const

    // XXX // update saved mode
    // XXX void operator()(unsigned n) { cur_mode = n; }
};

// store as many infos as will fit in `mcode` word
// if `mcode` smaller than `arg_serial_t`, use larger size
template <typename MCODE_T>
struct tgt_arg_serial_data_t
{
    // make sure `arg_mode_t` can fit in subfield
    static_assert(MCODE_T::arg_t::arg_mode_t::NUM_ARG_MODES
                    <= (1<< arg_serial_t::MODE_FIELD_SIZE)
                        , "too many `arg_mode` enums");

    
    // determine number of `chunks` per info
    // minimum 1 `arg_serial_t` per chunk
    constexpr static auto _ARGS_PER_MCODE =
            sizeof(typename MCODE_T::mcode_size_t) / sizeof(arg_serial_t);

    constexpr static auto ARGS_PER_CHUNK = _ARGS_PER_MCODE ? _ARGS_PER_MCODE : 1;
    
    auto begin()
    {
        return std::begin(info);
    }

private:
    // create "args_per_info" items 
    arg_serial_t  info[ARGS_PER_CHUNK];
};

//#define TRACE_ARG_SERIALIZE

template <typename MCODE_T, typename Inserter>
void insert_one (Inserter& inserter
               , unsigned n
               , typename MCODE_T::arg_t& arg   // ref to parsed instance
               , arg_serial_t *p                // ptr to serialized instance 
               , uint8_t sz
               , typename MCODE_T::fmt_t const&  fmt
               , typename MCODE_T::val_t const  *val_p
               , typename MCODE_T::mcode_size_t *code_p
               )
{
    // write arg data into machine code if possible (dependent on validator)
    // returns false if no validator
    bool completely_saved = fmt.insert(n, code_p, arg, val_p);
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg;
    std::cout << ": completely_saved = " << std::boolalpha << completely_saved;
    std::cout << ", has_validator = " << bool(val_p) << std::endl;
#endif

    // write arg data
    // NB: `serialize` can destroy arg.
    *p = arg.mode();
    if (!completely_saved)
    {
        p->has_reg  = !val_p;           // if no validator, check for register
        p->has_data = true;
        p->has_expr = arg.serialize(inserter, sz, p);
    }
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "write_one: " << arg;
    std::cout << ": mode = "   << std::dec << std::setw(2) << +p->init_mode;
    std::cout << " reg/data/expr = ";
    std::cout << +p->has_reg << "/" << +p->has_data << "/" << +p->has_expr;
    std::cout << std::endl;
#endif
}


// deserialize arguments: for format, see above
template <typename MCODE_T, typename Reader>
void extract_one(Reader& reader
                    , unsigned n
                    , typename MCODE_T::arg_t& arg
                    , arg_serial_t *p
                    , uint8_t sz
                    , typename MCODE_T::fmt_t const&  fmt
                    , typename MCODE_T::val_t const  *val_p
                    , typename MCODE_T::mcode_size_t *code_p
                    )
{
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\n[read_one:  mode = " << std::dec << std::setw(2) << +p->init_mode;
    std::cout << " reg/data/expr = ";
    std::cout << +p->has_reg  << "/" << +p->has_data << "/" << +p->has_expr;
#endif
    // extract arg from machine code (dependent on validator)
    // extract info from `opcode`
    arg.set_mode(p->init_mode);
    if (val_p)
        fmt.extract(n, code_p, arg, val_p);

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\nextract one 1: mode = " << +arg.mode();
    std::cout << ", init_mode = " << +p->init_mode;
    // XXX std::cout << ", cur_mode = " << +p->cur_mode << std::endl;
    std::cout << std::endl;
#endif

    // extract additional info as required
    // NB: extract may look at arg mode. Set to mode value when serialized
    arg.set_mode(p->init_mode);
    
    arg.extract(reader, sz, p);
    
#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\nextract one 2: mode = " << +arg.mode();
    std::cout << ", init_mode = " << +p->init_mode;
    // XXX std::cout << ", cur_mode = " << +p->cur_mode << std::endl;;
    std::cout << std::endl;
#endif

    // XXX // update mode to current value
    // XXX arg.set_mode(p->cur_mode);

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "\nextract one 3: mode = " << +arg.mode();
    std::cout << ", init_mode = " << +p->init_mode;
    std::cout << " -> " << arg << "] " << std::endl;
#endif
}

}


#endif
