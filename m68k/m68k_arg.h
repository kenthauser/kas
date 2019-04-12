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
    , MODE_IMMED            // 7-4 immediate (int or float)
// Additional support modes
    , MODE_DIRECT           // 12: uncategorized direct arg
    , MODE_DIRECT_ALTER     // 13: direct: PC_REL not allowed
    , MODE_REG              // 14: m68k register
    , MODE_REGSET           // 15: m68k register set
    , MODE_PAIR             // 16: register pair (multiply/divide/cas2)
    , MODE_BITFIELD         // 17: bitfield instructions
    , MODE_INDEX_BRIEF      // 18: brief mode index (word)
    , MODE_PC_INDEX_BRIEF   // 19: brief PC + index (word)
    , MODE_IMMED_QUICK      // 20: immed arg stored in opcode
    , MODE_REG_QUICK        // 21: movec: mode_reg stored in opcode
    , MODE_MOVEP            // 22: special for MOVEP insn

// Declare Immediate argument types: in `m68k_size_t` order
    , MODE_IMMED_BASE       // First of MODE_IMMED_*
    , MODE_IMMED_LONG = MODE_IMMED_BASE
    , MODE_IMMED_SINGLE
    , MODE_IMMED_XTND
    , MODE_IMMED_PACKED
    , MODE_IMMED_WORD
    , MODE_IMMED_DOUBLE
    , MODE_IMMED_BYTE
    , MODE_IMMED_VOID

// Support "modes"
    , MODE_ERROR            // set error message
    , MODE_NONE             // when parsed: indicates missing
    , NUM_ARG_MODES
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

struct m68k_arg_t : tgt::tgt_arg_t<m68k_arg_t, m68k_arg_mode>
{
    // inherit basic ctors
    using base_t::base_t;
    
    // direct, immediate, register pair, or bitfield
    m68k_arg_t(m68k_arg_mode mode, expr_t e = {}, expr_t outer = {})
            :  outer(outer), base_t(mode, std::move(e))
            {}

    // override `size`
    op_size_t size(expression::expr_fits const& fits = {});

    // support for `access-mode` validation
    uint16_t am_bitset() const;

    // declare size of immed args
    // NB: names of arg modes (OP_SIZE_*) is in `m68k_mcode.h`
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              {  4 }        // 0: LONG
            , {  4, 2 }     // 1: SINGLE
            , { 12, 6 }     // 2: XTND
            , { 12, 7 }     // 3: PACKED
            , {  2 }        // 4: WORD
            , {  8, 4 }     // 5: DOUBLE
            , {  2, {}, 1 } // 6: BYTE
            , {  0 }        // 7: VOID
        };

    template <typename Inserter>
    bool serialize(Inserter& inserter, uint8_t sz, bool& completely_saved);
    
    template <typename Reader>
    void extract(Reader& reader, uint8_t sz, bool has_data, bool has_expr);

    void emit(m68k_mcode_t const&, core::emit_base& base, unsigned bytes) const;

    // true if all `disp` and `outer` are registers or constants 
    bool is_const () const
    {
        auto do_const = [](auto const& e) -> bool
            {
                // `is_const` implies insn ready to emit.
                if (e.template get_p<m68k_reg_t>())
                    return true;
                if (e.template get_p<m68k_reg_set>())
                    return true;
                if (e.get_fixed_p())
                    return true;
                // if (e.template get_p<e_float_t>())
                //     return true;
                return false;
            };
        return do_const(expr) && do_const(outer);
    }

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

    // hardware formatted variables
    uint8_t cpu_mode() const;           // machine code words
    uint8_t cpu_reg()  const;
    uint8_t reg_num  {};
    m68k_ext_size_t cpu_ext;

    m68k_arg_mode mode_normalize() const;
    mutable uint16_t  _am_bitset{};
    mutable op_size_t _arg_size{-1};
};

// implementation in m68k.cc for debugging parser
extern std::ostream& operator<<(std::ostream& os, m68k_arg_t const& arg);
}

#endif
