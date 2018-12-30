#ifndef KAS_Z80_ARG_DEFN_H
#define KAS_Z80_ARG_DEFN_H

#include "kas_core/opcode.h"        // declares emit
#include "parser/kas_position.h"

namespace kas::z80
{
// Declare `stmt_z80` parsed instruction argument
enum z80_arg_mode : uint16_t
{
      MODE_NONE             // 0 when parsed: indicates missing: always zero
    , MODE_ERROR            // 1 set error message

// Directly supported modes
    , MODE_DIRECT           // 2 direct address (also accepted for immediate arg. sigh)
    , MODE_INDIRECT         // 3 indirect address
    , MODE_IMMEDIATE        // 4 immediate arg parser format
    , MODE_REG              // 5 register
    , MODE_REG_INDIR        // 6 register indirect

// Add "modes" for IX/IY as many modes (64) available & only two Index registers
// "Modes" are stored directly when args serialized. Allows prefix to be reconstructed
    , MODE_REG_IX           // 7
    , MODE_REG_IY           // 8
    , MODE_REG_INDIR_IX     // 9
    , MODE_REG_INDIR_IY     // 10
    , MODE_REG_OFFSET_IX    // 11
    , MODE_REG_OFFSET_IY    // 12

// Required enumeration
    , NUM_ARG_MODES
};


using kas::parser::kas_token; 
struct token_missing  : kas_token {};

struct z80_arg_t : kas_token {
    using op_size_t = core::opc::opcode::op_size_t;

    // x3 parser requires default constructable
    z80_arg_t() : _mode(MODE_NONE) {}

    // error
    z80_arg_t(const char *err, expr_t e = {})
            : _mode(MODE_ERROR), err(err), expr(e)
            {}

    // direct, immediate, register pair, or bitfield
    //z80_arg_t(z80_arg_mode mode, expr_t e = {});
    z80_arg_t(std::pair<expr_t, int> const&);

    auto mode() const { return _mode; }
    void set_mode(unsigned);
    void set_expr(expr_t);

    // for validate_min_max
    bool is_missing() const { return _mode == MODE_NONE; }


    op_size_t size(expression::expr_fits const& fits = {});
    void emit(core::emit_base& base, unsigned size) const;

    template <typename Inserter>
    bool serialize(Inserter& inserter, bool& val_ok);
    
    template <typename Reader>
    void extract(Reader& reader, bool has_data, bool has_expr);

    // true if all are registers or constants 
    bool is_const () const
    {
        switch (mode())
        {
            case MODE_REG:
            case MODE_REG_INDIR:
                return true;
            default:
                if (expr.get_fixed_p())
                    return true;
                break;
        }
        return false;
    }

    // validate if arg suitable for target
    template <typename...Ts>
    const char *ok_for_target(Ts&&...) const;

    void print(std::ostream&) const;

    // clear the "prefix"
    static void reset()  { 
        std::cout << "z80_arg_t::reset() called" << std::endl;
        prefix = 0; }

//    private:
    static inline uint8_t prefix;
    uint16_t    _mode {};
    z80_reg_t   reg  {};
    expr_t      expr {};
    const char *err  {};
};

// implementation in z80.cc for debugging parser
extern std::ostream& operator<<(std::ostream& os, z80_arg_t const& arg);

}

#endif
