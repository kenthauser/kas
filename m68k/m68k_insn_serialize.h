#ifndef KAS_M68K_INSN_SERIALIZE_H
#define KAS_M68K_INSN_SERIALIZE_H

// Serialize the `m68k_insn` and `m68k_args`
//
// Method: store arguments *in* opcode fields (ie mode/register), followed
// by `extension` word, followed by expressions. The `prefix` of actual
// argument data describes what follows. Two prefix formats are used:
//
//  1. For the `insn_list` format, the prefix is as follows:
//
//      opcode.fixed   = ok_bits (32-bits)
//      data.deque()   = word: insn_index
//                     = word: dummy opcode (zero)
//                     = ... arg data ...
//
//  2. For the `insn_resolved` format is as follows:
//
//      opcode.fixed   = word: opcode_index
//                     = word (or long) opcode_data
//      data.deque()   = ... arg data ...
//
// NB: for the case of `long` opcode data, both words are stored in
// the data.deque(). The inserter automocally groups both "words" together
//
///////////////////////////////////////////////////////////////////
//
// The routine `m68k_insert_args` serializes argument information.
//
// void `m68k_insert_args(Inserter, args, opcode_p, fmt const&)
//
//  - Inserter   : data_inserter for 
//  - args       : container of `m68k_arg_t`
//  - opcode_p   : pointer to array of words to store formatted opcode
//  - fmt const& : formatter for this opcode (or for `list`) 
// 
///////////////////////////////////////////////////////////////////
//
// Deserialization of data performed by
//
// <pair> m68k_read_args(Reader, opcode_p, fmt const&)
//
//  - Inserter   : data_inserter for 
//  - opcode_p   : pointer to array of words to store formatted opcode
//  - fmt const& : formatter for this opcode (or for `list`)
//
// Return type is pair of
//
//  - args const& : container of const `m68k_arg_t` instances
//  - info_p      : opaque information needed to update arg
//
// To update a stored argument (only arg_mode) use
//
// void m68k_arg_update(info_p, unsigned n, arg&, fmt const&, opcode_p)
//
//  - info_p      : opaque handle returned by deserialze
//  - n           : arg index (zero-based)
//  - arg&        : new argument data
//  - fmt const&  : as for deserialize
//  - opcode_p    : as for deserialize
// 
///////////////////////////////////////////////////////////////////


#include "m68k_arg_defn.h"
#include "m68k_data_inserter.h"
#include "m68k_arg_serialize.h"
#include "kas_core/opcode.h"


namespace kas::m68k::opc
{
template <typename Inserter, typename ARGS_T>
void m68k_insert_args(Inserter& inserter
                    , ARGS_T&& args
                    , detail::m68k_op_size_t* opcode_p
                    , m68k_opcode_fmt const& fmt
                    )
{
    using expression::expr_fits;
    using expression::fits_result;
    auto& ARGS_PER_INFO = detail::m68k_arg_info::ARGS_PER_INFO;

    unsigned n = 0;
    detail::arg_info_t *p;

    // save arg_mode & info about extensions (constants or expressions)
    for (auto& arg : args)
    {
        // need `arg_info` scrach area. create one for modulo numbered args
        if (auto idx_n = n % ARGS_PER_INFO) {
            // not modulo -- just increment
            ++p;
        } else {
            // need new arg_info
            auto& info = detail::m68k_arg_info::cast(inserter(0, M_SIZE_WORD));
            p = info.info;
        }

        // do work
        m68k_insert_one(inserter, n, arg, p, fmt, opcode_p);
        ++n;

        // std::cout << std::hex;
        // std::cout << "write_arg :            mode = " << (int)arg.mode;
        // std::cout << " reg_num = " << arg.reg_num;
        // std::cout << " arg = " << arg << std::endl;
    }

    // if non-modulo number of args, flag end in `arg_info` area
    if (auto idx_n = n % ARGS_PER_INFO)
        p[idx_n].arg_mode = MODE_NONE;
}



// deserialize m68k_arguments: for format, see above
template <typename M68K_Iter>
auto m68k_read_args(M68K_Iter& reader
                , m68k_opcode_fmt const& fmt
                , uint16_t *opcode_p)
{
    // deserialize into static array.
    // add extra `static_arg` as an end-of-list flag.
    // NB: make c[], not std::array to prevent re-instatantion
    // of common read/write routines (eg: `eval_list`)
    using detail::m68k_arg_info;
    auto& ARGS_PER_INFO = m68k_arg_info::ARGS_PER_INFO;

    static m68k_arg_t     static_args[m68k_insn_t::MAX_ARGS+1];
    static m68k_arg_info *static_info[m68k_insn_t::MAX_ARGS/ARGS_PER_INFO+1];

    // initialize static array (default constructs to empty)
    for (auto& arg : static_args) arg = {};

    // get working pointers into static arrays
    auto arg_p  = std::begin(static_args);
    auto info_p = std::begin(static_info);

    // read & decode arguments until empty
    detail::arg_info_t *p;
    for (unsigned n = 0;;++n, ++arg_p)
    {

        // last static_arg is end-of-list flag, should never reach it.
        if (arg_p == std::end(static_args))
            throw std::runtime_error {"m68k_read_args"};

        //std::cout << std::endl;
        
        // deserialize `arg_info` if needed (get `ARGS_PER_INFO` at a time...)
        if ((n % ARGS_PER_INFO) == 0) {
            if (reader.empty())
                break;
            // read info pointer into static area...
            *info_p = &detail::m68k_arg_info::cast(reader.get_fixed_p(M_SIZE_WORD));
            // get first in this array
            p = (*info_p)->info;
            // ready for next static entry
            ++info_p;
        } else {
            ++p;
            if (p->arg_mode == MODE_NONE)
                break;
        }

        // do the work
        m68k_extract_one(reader, n, arg_p, p, fmt, opcode_p);

        // std::cout << std::hex;
        // std::cout << "read_arg  : mode = " << (int)p->arg_mode << " p_mode = " << arg_p->mode;
        // std::cout << " reg_num = " << arg_p->reg_num;
        // std::cout << " arg = " << *arg_p;
    }
    
    return std::make_pair(static_args, static_info);
}

// update opcode with new reg/mode
inline void m68k_arg_update(
        unsigned n,             // argument number
        m68k_arg_t& arg,        // argument reference
        void  *update_handle,  // opaque value generated in `read_args`
        m68k_opcode_fmt const& fmt,
        uint16_t *opcode_p      // opcode location
        )
{
    auto& ARGS_PER_INFO = detail::m68k_arg_info::ARGS_PER_INFO;
    auto info_p = static_cast<detail::m68k_arg_info **>(update_handle);

    // update saved `arg.mode`...
    auto& p = info_p[n/ARGS_PER_INFO]->info[n%ARGS_PER_INFO];
#if 0
    std::cout << "m68k_arg_update: " << arg;
    std::cout << " " << (int)p.arg_mode << " -> " << arg.mode << std::endl;
#endif
    p.arg_mode = arg.mode;      // so we know next time.

    // ...and update the opcode
    fmt.insert(n, opcode_p, arg);
}
}
#endif
