#ifndef KAS_TARGET_TGT_INSN_SERIALIZE_H
#define KAS_TARGET_TGT_INSN_SERIALIZE_H

///////////////////////////////////////////////////////////////////
// 
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
// the data.deque(). The inserter groups both "words" together
//
///////////////////////////////////////////////////////////////////
//
// XXX remove `z80` reference
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
    using arg_t                 = typename MCODE_T::arg_t;
    using mcode_size_t          = typename MCODE_T::mcode_size_t;
    using tgt_arg_serial_data_t = detail::tgt_arg_serial_data_t<MCODE_T>;

    constexpr auto ARGS_PER_CHUNK = tgt_arg_serial_data_t::ARGS_PER_CHUNK;
   
    // insert base "code" (or zero from `insn_t::list_mcode`)
    // modify `mcode` to store `stmt_info` before writing
#ifdef TRACE_ARG_SERIALIZE
    {
        auto c = m_code.code(stmt_info);    // calculate "code"
        auto p = c.begin();                 // walk thru values
        auto n = m_code.code_size();

        std::cout << "tgt_insert_args: info.value() = " << std::hex << stmt_info.value() << std::endl;
        std::cout << "tgt_insert_args: base_size = " << +n;
        std::cout << ", base_code = " << std::hex;
        std::cout << std::setfill('0');
        while (n > 0)
        {
            std::cout << std::setw(sizeof(*p) * 2) << *p++;
            n -= sizeof(*p);
            if (n > 0) std::cout << "'";
        }
        std::cout << std::setfill(' ');
        std::cout << std::endl;
    }
#endif
    // insert "code" array
    // NB: `inserter` value_t is `mcode_size_t`, thus code_array is directly inserted
    auto code   = m_code.code(stmt_info);       // `code` is std::array<>
    auto code_p = inserter(code.data(), m_code.code_size());

    // retrieve formatters and validators to write args into code (as appropriate)
    auto& fmt          = m_code.fmt();          // arg formatting instructions
    auto  sz           = stmt_info.sz(m_code);  // byte/half/word/long, etc
    auto& vals         = m_code.vals();         // arg validation list
    auto  val_iter     = vals.begin();
    auto  val_iter_end = vals.end();

    auto immed_size = arg_t::immed_info(sz).sz_bytes;

    unsigned n = 0;                             // arg index
    detail::arg_serial_t *p;                    // current arg info data chunk

    // save arg_mode & info about extensions (constants or expressions)
    for (auto& arg : args)
    {
        typename MCODE_T::val_t const *val_p {};
        const char *val_name;       // NB: names availble via iter, not ptr

        // need `arg_info` scrach area. create one for modulo numbered args
        // (creates one for first argument)
        if (n % ARGS_PER_CHUNK) 
        {
            // not modulo -- just increment to next
            ++p;
        } 
        else
        {
            // insert new arg_info (NB: in host format)
            auto chunk_p = inserter.write(tgt_arg_serial_data_t{});
            p = chunk_p->begin();
        }

        // do work: pass validator if present
        // NB: may be more args than validators. LIST format only saves a few args in `code`
        if (val_iter != val_iter_end)
        {
#ifdef TRACE_ARG_SERIALIZE
            val_name = val_iter.name();
#endif
            val_p = &*val_iter++;
        }

        // NB: for LIST format, validator may be present (to store
        //     value in LIST base code), but arg may not pass.
        //     Run `fast` OK method to see if can store
        if (val_p)
            if (val_p->ok(arg, expr_fits{}) != expr_fits::yes)
                val_p = nullptr;

#ifdef TRACE_ARG_SERIALIZE
        if (!val_p)
            val_name = "*NONE*";
        
        std::cout << "tgt_insert_args: " << +n 
                  << " mode = " << +arg.mode() 
                  << " arg = " << arg 
                  << " val = " << val_name
                  << std::endl;
#endif
        detail::insert_one<MCODE_T>(inserter, n, arg, p, sz, fmt, val_p, code_p);
        ++n;
    }
    
    // NB: non-modulo args at end are inited to zero
    // if MODE_NONE != zero, flag end of args
    if constexpr (arg_t::MODE_NONE != 0 && ARGS_PER_CHUNK != 1)
        if (n % ARGS_PER_CHUNK)
            p[1].init_mode = arg_t::MODE_NONE;
}

// deserialize arguments: for format, see above
template <typename READER_T, typename MCODE_T>
auto tgt_read_args(READER_T& reader, MCODE_T const& m_code)
{
    // deserialize into static array.
    // add extra `static_arg` as an end-of-list flag.
    // NB: use `c-lang`[], not std::array to prevent re-instatantion
    // of common read/write routines (eg: `eval_list`)
    using  arg_t                  = typename MCODE_T::arg_t;
    using  tgt_arg_serial_data_t  = detail::tgt_arg_serial_data_t<MCODE_T>;
    using  stmt_info_t            = typename MCODE_T::stmt_info_t;
    constexpr auto ARGS_PER_CHUNK = tgt_arg_serial_data_t::ARGS_PER_CHUNK;

    // local statics to hold info on single insn
    static arg_t                 static_args[MCODE_T::MAX_ARGS+1];
    static detail::arg_serial_t *static_serial_t[MCODE_T::MAX_ARGS];

    // get "opcode" info
    auto code_p     = reader.get_fixed_p(m_code.code_size());
    auto stmt_info  = m_code.extract_info(code_p);
    auto sz         = stmt_info.sz(m_code);
    auto immed_size = arg_t::immed_info(sz).sz_bytes;

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "tgt_read_args: info = " << stmt_info;
    std::cout << ", immed_size = " << +immed_size << std::endl;
#endif

    // read & decode arguments until empty
    auto& fmt         = m_code.fmt();
    auto& vals        = m_code.vals();
    auto val_iter     = vals.begin();
    auto val_iter_end = vals.end();

    // reset per-insn arg state (required by Z80, normally optimized away)
    arg_t::reset();

    // read until end flag: eg: MODE_NONE or reader.empty()
    // get working pointer into static array
    auto arg_p             = std::begin(static_args);
    auto serial_t_inserter = std::begin(static_serial_t);
    detail::arg_serial_t *p;
   
    // begin by finding next `arg_serial_t *` instance and assigning to `p`
    for (unsigned n = 0; true ;++n, ++arg_p)
    {
        typename MCODE_T::val_t const *val_p {};
        const char *val_name;       // NB: names availble via iter, not ptr
        
        // last static_arg is end-of-list flag, should never reach it.
        if (arg_p == std::end(static_args))
            throw std::runtime_error {"tgt_read_args: MAX_ARGS exceeded"};

        // init arg (to MODE_NONE)
        *arg_p = {};

        // deserialize `arg_info` if needed (get `ARGS_PER_CHUNK` at a time...)
        if ((n % ARGS_PER_CHUNK) == 0)
        {
            if (reader.empty())
                break;
            
            // retrieve array `arg_info_t` & get pointer to first element
            auto arg_info_p = reader.read(tgt_arg_serial_data_t{});
            
            // retrieve & record serial data address
            *serial_t_inserter++ = p = arg_info_p->begin();
        } 
        else
        {
            *serial_t_inserter++ = ++p;
            if (p->init_mode == arg_t::MODE_NONE)
                break;
        }

        // found `arg_serial_t` instance. now do extraction work.
        // NB: may be more args than validators.
        // NB: LIST format only saves a few args in `code`
        if (val_iter != val_iter_end)
        {
#ifdef TRACE_ARG_SERIALIZE
            val_name = val_iter.name();
#endif
            val_p = &*val_iter++;       // NB: val_p zero inited
        }

#ifdef TRACE_ARG_SERIALIZE
        if (!val_p)
            val_name = "*NONE*";
        std::cout << "tgt_read_args: " << +n 
                  << " val = " << val_name
                  << std::endl;
#endif

        detail::extract_one<MCODE_T>(reader, n, *arg_p, p, sz, fmt, val_p, code_p);
    }
    return std::make_tuple(code_p, static_args, static_serial_t, stmt_info);
}

}
#endif
