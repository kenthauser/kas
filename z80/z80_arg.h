#ifndef KAS_Z80_ARG_DEFN_H
#define KAS_Z80_ARG_DEFN_H

#include "z80_reg_types.h"
#include "target/tgt_arg.h"
//#include "kas_core/opcode.h"        // declares emit
//#include "parser/kas_position.h"


namespace kas::z80
{

//using kas::parser::kas_token; 
//struct token_missing  : kas_token {};

// Declare argument "modes"
enum z80_arg_mode : uint8_t
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


struct z80_arg_t : tgt::tgt_arg_t<z80_arg_t, z80_arg_mode>
{
    // inherit default & error ctors
    using base_t::base_t;
    //using arg_mode_t = z80_arg_mode;
    
    // defn needed by `base_t`
    static constexpr auto MODE_NONE = z80_arg_mode::MODE_NONE;

    // direct, indirect, and immediate ctor
    z80_arg_t(std::pair<expr_t, int> const&);

    op_size_t size(expression::expr_fits const& fits = {});
    void emit(core::emit_base& base, unsigned size) const;

    template <typename Inserter>
    bool serialize(Inserter& inserter, bool& val_ok);
    
    template <typename Reader>
    void extract(Reader& reader, bool has_data, bool has_expr);

    // true if all are registers or constants 
    bool is_const() const
    {
        switch (mode())
        {
            case MODE_REG:
            case MODE_REG_IX:
            case MODE_REG_IY:
            case MODE_REG_INDIR:
            case MODE_REG_INDIR_IX:
            case MODE_REG_INDIR_IY:
                return true;
            default:
                break;
        }
        return expr.get_fixed_p();
    }

    void set_mode(uint8_t mode);
    void set_expr(expr_t& e);

    // validate if arg suitable for target
    template <typename...Ts>
    const char *ok_for_target(Ts&&...) const;

    void print(std::ostream&) const;

    // XXX in base_t??
    friend std::ostream& operator<<(std::ostream& os, z80_arg_t const& t)
    {
        t.print(os);
        return os;
    }

    // clear the "prefix"
    static void reset()
    { 
        std::cout << "z80_arg_t::reset() called" << std::endl;
        prefix = {};
    }

    static inline uint8_t prefix;
    z80_reg_t   reg  {};
};

}

#endif
