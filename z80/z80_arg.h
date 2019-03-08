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
    , MODE_IMMEDIATE        // 4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      // 5 immediate arg (stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect

// Add "modes" for IX/IY as many modes (64) available & only two Index registers
// "Modes" are stored directly when args serialized. Allows prefix to be reconstructed
    , MODE_REG_IX           // 8
    , MODE_REG_IY           // 9
    , MODE_REG_INDIR_IX     // 10
    , MODE_REG_INDIR_IY     // 11
    , MODE_REG_OFFSET_IX    // 12
    , MODE_REG_OFFSET_IY    // 13

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
    z80_arg_t(std::pair<expr_t, z80_arg_mode> const&);

    op_size_t size(expression::expr_fits const& fits = {});
    void emit(core::emit_base& base, unsigned size) const;

    template <typename Inserter>
    bool serialize(Inserter& inserter, bool& completely_saved);
    
    template <typename Reader>
    void extract(Reader& reader, bool has_data, bool has_expr);

    bool is_const() const;
    void set_mode(z80_arg_mode mode);
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
        prefix = {};
    }

    static inline uint8_t prefix;
    z80_reg_t   reg  {};
};

}

#endif
