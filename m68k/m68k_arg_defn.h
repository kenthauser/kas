#ifndef KAS_M68K_ARG_DEFN_H
#define KAS_M68K_ARG_DEFN_H

#include "m68k_size_defn.h"
#include "m68k_extension_t.h"
#include "kas_core/opcode.h"
#include "parser/kas_position.h"
//#include "m68k_insn_info.h"

namespace kas::m68k
{
// Declare `stmt_m68k` parsed instruction argument
enum m68k_arg_mode : uint16_t
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
    , MODE_IMMED_QUICK      // 19: immed arg stored in opcode
    , MODE_REG_QUICK        // 20: movec: mode_reg stored in opcode
    , MODE_MOVEP            // 21: special for MOVEP insn

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
enum m68k_arg_subword : uint16_t
{
      REG_SUBWORD_FULL  = 0
    , REG_SUBWORD_LOWER
    , REG_SUBWORD_UPPER
    , REG_SUBWORD_MASK      // not really subword, but MAC related
};

using kas::parser::kas_token;

struct token_reg : kas_token
{
};

struct token_missing  : kas_token {};

struct m68k_arg_t : kas_token {
    using op_size_t = core::opc::opcode::op_size_t;

    // x3 parser requires default constructable
    m68k_arg_t() : mode(MODE_NONE) {}

    // error
    m68k_arg_t(const char *err, expr_t e = {})
            : mode(MODE_ERROR), err(err), disp(e)
            {}

    // direct, immediate, register pair, or bitfield
    m68k_arg_t(m68k_arg_mode mode, expr_t e = {}, expr_t outer = {})
            : mode(mode), disp(e), outer(outer)
            {
                if (auto p = e.get_p<m68k_reg>())
                    std::cout << "m68k_arg_t: ctor: " << p->name() << std::endl;
            }

    // for validate_min_max
    bool is_missing() const { return mode == MODE_NONE; }


    op_size_t size(expression::expr_fits const& fits = {});

    uint16_t am_bitset() const;

    template <typename T>
    T const* get_p() const
    {
        switch (mode)
        {
            case MODE_DIRECT:
                return disp.get_p<T>();
            default:
                return nullptr;
        }
    }

    // true if all `disp` and `outer` are registers or constants 
    bool is_const () const
    {
        auto do_const = [](auto const& e) -> bool
            {
                // `is_const` implies insn ready to emit.
                if (e.template get_p<m68k_reg>())
                    return true;
                if (e.template get_p<m68k_reg_set>())
                    return true;
                if (e.get_fixed_p())
                    return true;
                // if (e.template get_p<e_float_t>())
                //     return true;
                return false;
            };
        return do_const(disp) && do_const(outer);
    }

    // validate if arg suitable for target
    const char *ok_for_target(opc::m68k_size_t sz) const;

    uint16_t mode {};
    uint16_t reg_subword {};
    m68k_extension_t ext{};
    expr_t  disp;
    expr_t  outer;

    // hardware formatted variables
    short cpu_mode() const;
    short cpu_reg()  const;
    short reg_num  {};
    m68k_ext_size_t cpu_ext;
    const char *err{};
//    private:
    short mode_normalize() const;
    mutable uint16_t  _am_bitset{};
    mutable op_size_t _arg_size{-1};
};

// implementation in m68k.cc for debugging parser
extern std::ostream& operator<<(std::ostream& os, m68k_arg_t const& arg);

inline void ostream_m68k_args(std::ostream& os, m68k_arg_t const* arg_p)
{
    auto delim = ": ";
    while (arg_p->mode != MODE_NONE) {
        os << delim << *arg_p++;
        delim = ",";
    }
}

}

#endif
