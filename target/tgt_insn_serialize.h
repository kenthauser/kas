#ifndef KAS_TARGET_TGT_INSN_SERIALIZE_H
#define KAS_TARGET_TGT_INSN_SERIALIZE_H

// Serialize the `insn` and `args`
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
// The routine `z80_insert_args` serializes argument information.
//
// void `z80_insert_args(Inserter, args, opcode_p, fmt const&)
//
//  - Inserter   : data_inserter for 
//  - args       : container of `z80_arg_t`
//  - opcode_p   : pointer to array of words to store formatted opcode
//  - fmt const& : formatter for this opcode (or for `list`) 
// 
///////////////////////////////////////////////////////////////////
//
// Deserialization of data performed by
//
// <pair> z80_read_args(Reader, opcode_p, fmt const&)
//
//  - Inserter   : data_inserter for 
//  - opcode_p   : pointer to array of words to store formatted opcode
//  - fmt const& : formatter for this opcode (or for `list`)
//
// Return type is pair of
//
//  - args const& : container of const `z80_arg_t` instances
//  - info_p      : opaque information needed to update arg
//
// To update a stored argument (only arg_mode) use
//
// void z80_arg_update(info_p, unsigned n, arg&, fmt const&, opcode_p)
//
//  - info_p      : opaque handle returned by deserialze
//  - n           : arg index (zero-based)
//  - arg&        : new argument data
//  - fmt const&  : as for deserialize
//  - opcode_p    : as for deserialize
// 
///////////////////////////////////////////////////////////////////


#include "tgt_data_inserter.h"
#include "tgt_arg_inserter.h"
#include "kas_core/opcode.h"


namespace kas::tgt::opc
{
template <typename Inserter, typename MCODE_T, typename ARGS_T>
void tgt_insert_args(Inserter& inserter
                    , MCODE_T const& m_code
                    , ARGS_T&& args
                    , typename MCODE_T::stmt_info_t stmt_info
                    )
{
    using arg_t = typename MCODE_T::arg_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    constexpr auto ARGS_PER_INFO = detail::tgt_arg_info<MCODE_T>::ARGS_PER_INFO;
   
    // insert base "code" (a appropriately sized zero) & use pointer for arg inserter
    auto code_p = inserter(m_code.code(stmt_info).data(), m_code.code_size());

    auto& fmt         = m_code.fmt();
    auto  sz          = m_code.sz(stmt_info);
    auto& vals        = m_code.vals();
    auto val_iter     = vals.begin();
    auto val_iter_end = vals.end();

    unsigned n = 0;
    detail::arg_info_t *p;

    // save arg_mode & info about extensions (constants or expressions)
    for (auto& arg : args)
    {
        typename MCODE_T::val_t const *val_p;
        const char *val_name;

        // need `arg_info` scrach area. create one for modulo numbered args
        // (creates one for first argument)
        if (auto idx_n = n % ARGS_PER_INFO) 
        {
            // not modulo -- just increment
            ++p;
        } 
        else
        {
            // insert new arg_info
            auto& info = detail::tgt_arg_info<MCODE_T>::cast(inserter(0, sizeof(mcode_size_t)));
            p = info.begin();
        }

        // do work: pass validator if present
        // NB: may be more args than validators. LIST format only saves a few args in `code`
        if (val_iter != val_iter_end)
        {
            val_name = val_iter.name();
            val_p = &*val_iter++;
        }
        else
            val_p = {};

        // if validator present, be sure it can hold type
        // XXX only required for "LIST" format.
        if (val_p)
        {
            expr_fits fits{};
            if (val_p->ok(arg, sz, fits) != fits.yes)
                val_p = nullptr;
        }

        if (!val_p)
            val_name = "*NONE*";
        
        std::cout << "tgt_insert_args: " << +n 
                  << " mode = " << +arg.mode() 
                  << " arg = " << arg 
                  << " val = " << val_name
                  << std::endl;
        detail::insert_one<MCODE_T>(inserter, n, p, arg, sz, fmt, val_p, code_p);
        ++n;
    }

    // if non-modulo number of args, flag end in `arg_info` area
    if (auto idx_n = n % ARGS_PER_INFO)
        p[idx_n].arg_mode = arg_t::MODE_NONE;
}



// deserialize arguments: for format, see above
template <typename Reader, typename MCODE_T>
auto tgt_read_args(Reader& reader, MCODE_T const& m_code)
{
    // deserialize into static array.
    // add extra `static_arg` as an end-of-list flag.
    // NB: make c[], not std::array to prevent re-instatantion
    // of common read/write routines (eg: `eval_list`)
    constexpr auto ARGS_PER_INFO = detail::tgt_arg_info<MCODE_T>::ARGS_PER_INFO;
    using  mcode_size_t = typename MCODE_T::mcode_size_t;
    using  arg_t        = typename MCODE_T::arg_t; using  arg_info     = detail::tgt_arg_info<MCODE_T>; 
    using  stmt_info_t  = typename MCODE_T::stmt_info_t;

    static arg_t     static_args[MCODE_T::MAX_ARGS+1];
    static arg_info *static_info[MCODE_T::MAX_ARGS/ARGS_PER_INFO+1];
    

    // initialize static array (default constructs to empty)
    for (auto& arg : static_args) arg = {};
    arg_t::reset();

    // get working pointers into static arrays
    auto arg_p  = std::begin(static_args);
    auto info_p = std::begin(static_info);

    // get "opcode" info
    auto  code_p = reader.get_fixed_p(m_code.code_size());
    stmt_info_t stmt_info{};

    // read & decode arguments until empty
    auto& fmt         = m_code.fmt();
    auto  sz          = m_code.sz(stmt_info);
    auto& vals        = m_code.vals();
    auto val_iter     = vals.begin();
    auto val_iter_end = vals.end();

    // read until end flag: eg: MODE_NONE or reader.empty()
    detail::arg_info_t *p;
    for (unsigned n = 0; true ;++n, ++arg_p)
    {

        // last static_arg is end-of-list flag, should never reach it.
        if (arg_p == std::end(static_args))
            throw std::runtime_error {"tgt_read_args: MAX_ARGS exceeded"};

        //std::cout << std::endl;
        
        // deserialize `arg_info` if needed (get `ARGS_PER_INFO` at a time...)
        if ((n % ARGS_PER_INFO) == 0)
        {
            if (reader.empty())
                break;
            // read info pointer into static area...
            *info_p = &arg_info::cast(reader.get_fixed_p(sizeof(mcode_size_t)));
            // get first in this array
            p = (*info_p)->begin();
            // ready for next static entry
            ++info_p;
        } 
        else
        {
            ++p;
            if (p->arg_mode == arg_t::MODE_NONE)
                break;
        }

        // do the work
        if (val_iter != val_iter_end)
            detail::extract_one<MCODE_T>(reader, n, p, arg_p, sz, fmt, &*val_iter++, code_p);
        else
            detail::extract_one<MCODE_T>(reader, n, p, arg_p, sz, fmt, nullptr, code_p);

        // std::cout << std::hex;
        // std::cout << "read_arg  : mode = " << (int)p->arg_mode << " p_mode = " << arg_p->mode;
        // std::cout << " reg_num = " << arg_p->reg_num;
        // std::cout << " arg = " << *arg_p;
    }
    
    return std::make_tuple(code_p, static_args, static_info);
}

// update opcode with new mode
template <typename MCODE_T>
inline void tgt_arg_update(
        MCODE_T const& m_code,       // machine code
        unsigned n,                 // argument number
        typename MCODE_T::arg_t& arg,        // argument reference
        void  *update_handle,       // opaque value generated in `read_args`
        typename MCODE_T::mcode_size_t *code_p  // opcode data location
        )
{
    constexpr auto ARGS_PER_INFO = detail::tgt_arg_info<MCODE_T>::ARGS_PER_INFO;
    using arg_info_t   = detail::tgt_arg_info<MCODE_T>; 
    auto info_p = static_cast<arg_info_t**>(update_handle);

    // get format & validator
    auto& fmt  = m_code.fmt();
    auto& vals = m_code.vals();
    auto val_iter     = vals.begin();
    auto val_iter_end = vals.end();

    if (n < vals.size())
        std::advance(val_iter, n);
    else
        std::advance(val_iter, vals.size());

    // update saved `arg.mode`...
    auto p = info_p[n/ARGS_PER_INFO]->begin() + (n%ARGS_PER_INFO);
#if 0
    std::cout << "arg_update: " << arg;
    std::cout << " " << (int)p.arg_mode << " -> " << arg.mode << std::endl;
#endif
    p->arg_mode = arg.mode();    // so we know next time.

    // ...and update the saved data 
    // do work: pass validator if present
    if (val_iter != val_iter_end)
        fmt.insert(n, code_p, arg, &*val_iter);
    else
        fmt.insert(n, code_p, arg, nullptr);
}
}
#endif
