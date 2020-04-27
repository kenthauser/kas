#ifndef KAS_M68K_ARG_DEFN_H
#define KAS_M68K_ARG_DEFN_H

#include "m68k_reg_types.h"
#include "target/tgt_arg.h"

#include "m68k_extension_t.h"
#include "kas_core/opcode.h"
#include "parser/kas_position.h"

namespace kas::m68k
{
// Declare `stmt_m68k` parsed instruction argument
// NB: must be able to determine emitted size & cpu mode
// of all args using only `mode` (+ extension word for indirects)
// Thus, multiple "modes" are used for branch sizes. Also,
// "DISP_LONG" mode translates into "index" or "disp" depending on
// size of arg.
enum m68k_arg_mode : uint8_t
{
// Directly supported modes
// NB: DATA_REG thru IMMED must have values match M68K machine code defn
      MODE_DATA_REG         // 0 Data regsiter direct
    , MODE_ADDR_REG         // 1 Address register direct
    , MODE_ADDR_INDIR       // 2 Address register indirect
    , MODE_POST_INCR        // 3 address register only
    , MODE_PRE_DECR         // 4 address register only
    , MODE_ADDR_DISP        // 5 word offset
    , MODE_INDEX            // 6 address register index
    , MODE_DIRECT_SHORT     // 7-0 16-bit direct address
    , MODE_DIRECT_LONG      // 7-1 32-bit direct address
    , MODE_PC_DISP          // 7-2 PC + word offset
    , MODE_PC_INDEX         // 7-3 PC + index
    , MODE_IMMEDIATE        // 7-4 immediate (int or float)
// Additional support modes
    , MODE_ADDR_DISP_LONG   // 12: address displacement with long arg
    , MODE_DIRECT           // 13: uncategorized direct arg
    , MODE_DIRECT_ALTER     // 14: direct: PC_REL not allowed
    , MODE_DIRECT_PCREL     // 15: direct: PC_REL indicated
    , MODE_REG              // 16: m68k register
    , MODE_REGSET           // 17: m68k register set
    , MODE_PAIR             // 18: register pair (multiply/divide/cas2)
    , MODE_BITFIELD         // 19: bitfield instructions
    , MODE_IMMED_QUICK      // 20: immed arg stored in opcode
    , MODE_REG_QUICK        // 21: movec: mode_reg stored in opcode
    , MODE_MOVEP            // 22: special for MOVEP insn
// Branch displacement sizes
    , MODE_BRANCH_BYTE      // 23: store displacment in insn
    , MODE_BRANCH_WORD      // 24: single displacment word
    , MODE_BRANCH_LONG      // 25: two displacement words

// Support "modes"
    , MODE_ERROR            // 26: set error message
    , MODE_NONE             // 27: when parsed: indicates end-of-args
    , NUM_ARG_MODES

// MODES which must be defined for compatibilty with `tgt_arg` ctor
// never allocated. Do not need to include in `NUM_ARG_MODES`
    , MODE_INDIRECT
    , MODE_REG_INDIR 
    , MODE_REG_OFFSET 
};

// support for coldfire MAC. 
enum m68k_arg_subword : uint8_t
{
      REG_SUBWORD_FULL  = 0
    , REG_SUBWORD_LOWER
    , REG_SUBWORD_UPPER
    , REG_SUBWORD_MASK      // not really subword, but MAC related
};

using kas::parser::kas_token;

struct token_reg : kas_token {};

struct token_missing  : kas_token {};

// `REG_T` & `REGSET_T` args allow `MCODE_T` to lookup types
// forward declare `m68k_stmt_info_t` from m68k_stmt.h
struct m68k_arg_t : tgt::tgt_arg_t<m68k_arg_t
                                 , m68k_arg_mode
                                 , m68k_reg_t
                                 , m68k_reg_set_t
                                 , struct m68k_stmt_info_t
                                 >
{
    // `extension_t` updated during evalaution along with mode
    using arg_writeback_t = m68k_extension_t *;
    
    // inherit basic ctors
    using base_t::base_t;
    
    // direct, immediate, register pair, or bitfield
    m68k_arg_t(m68k_arg_mode mode, kas_token e = {}, kas_token outer = {})
            :  outer(outer.expr()), base_t(mode, e)
            {}

    // indirect & index values are constructed in `m68k_parser_support.h`
    // and inited via copy elision

    // override `sz_info' in `impl`
    static const tgt::tgt_immed_info sz_info[];

    // override `size`
    op_size_t size(uint8_t sz, expression::expr_fits const *fits_p = {}, bool *is_signed = {});

    // support for `access-mode` validation
    uint16_t am_bitset() const;

    // serialize arg into `insn data` area
    template <typename Inserter, typename WB_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, WB_INFO *);
    
    // extract serialized arg from `insn data` area
    template <typename Reader>
    void extract(Reader& reader, uint8_t sz, arg_serial_t *);

    // restore arg to `extracted` value for new iteration of `size`
    template <typename ARG_INFO>
    void restore(ARG_INFO const*, m68k_extension_t const *);

    // emit arg & relocations based on mode & info
    // `m68k` has various special modes which must be interpreted
    // NB: emit_immed & emit_float not overridden
    void emit(core::emit_base& base, uint8_t sz);


    // true if all `expr` and `outer` are registers or constants 
    bool is_const () const;

    // XXX don't let register appear as constants (? why needed ?)
    // also seems to block IMMEDIATE
    template <typename T>
    T const* get_p() const
    {
        switch (mode())
        {
            case MODE_DIRECT:
                return expr.get_p<T>();
            default:
                return nullptr;
        }
    }

    // validate if arg suitable for target
    kas::parser::kas_error_t ok_for_target(uint8_t sz);

    expr_t           outer;             // for '020 PRE/POST index addess modes
    m68k_arg_subword reg_subword {};    // for coldfire H/L subword access
    m68k_extension_t ext{};             // m68k extension word (index modes)
#if 1
    // hardware formatted variables
    uint8_t cpu_mode() const;           // machine code words
    uint8_t cpu_reg()  const;
    uint8_t reg_num  {};
    m68k_ext_size_t cpu_ext;
#endif
    m68k_arg_mode mode_normalize() const;
    mutable uint16_t  _am_bitset{};
#if 1
    mutable op_size_t _arg_size{-1};
#endif
};

// implementation in m68k.cc for debugging parser
extern std::ostream& operator<<(std::ostream& os, m68k_arg_t const& arg);
}

#endif
