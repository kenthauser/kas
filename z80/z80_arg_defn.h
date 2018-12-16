#ifndef KAS_Z80_ARG_DEFN_H
#define KAS_Z80_ARG_DEFN_H

#include "kas_core/opcode.h"
#include "parser/kas_position.h"

namespace kas::z80
{
// Declare `stmt_z80` parsed instruction argument
enum z80_arg_mode : uint16_t
{
// Directly supported modes
      MODE_DIRECT           // direct address or immediate
    , MODE_INDIRECT         // indirect address
    , MODE_REG              // register
    , MODE_REG_INDIR        // register indirect
    , MODE_REG_OFFSET       // register indirect + offset

// Support "modes"
    , MODE_ERROR            // set error message
    , MODE_NONE             // when parsed: indicates missing
    , NUM_ARG_MODES
};


using kas::parser::kas_token; 
struct token_missing  : kas_token {};

struct z80_arg_t : kas_token {
    using op_size_t = core::opc::opcode::op_size_t;

    // x3 parser requires default constructable
    z80_arg_t() : mode(MODE_NONE) {}

    // error
    z80_arg_t(const char *err, expr_t e = {})
            : mode(MODE_ERROR), err(err), expr(e)
            {}

    // direct, immediate, register pair, or bitfield
    z80_arg_t(z80_arg_mode mode, expr_t e = {});

    // for validate_min_max
    bool is_missing() const { return mode == MODE_NONE; }


    op_size_t size(expression::expr_fits const& fits = {});

    // true if all are registers or constants 
    bool is_const () const
    {
        switch (mode)
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

//    private:
    uint16_t    mode {};
    z80_reg_t   reg  {};
    expr_t      expr {};
    const char *err  {};
};

// implementation in z80.cc for debugging parser
extern std::ostream& operator<<(std::ostream& os, z80_arg_t const& arg);

}

#endif
