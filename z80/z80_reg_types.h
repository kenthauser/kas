#ifndef KAS_Z80_Z80_REG_TYPES_H
#define KAS_Z80_Z80_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare register constants
// Derive  register types from `target` CRTP base types
//
////////////////////////////////////////////////////////////////////////////


#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"

namespace kas::z80 
{

// Declare Register "Classes" for Z80
enum { RC_NONE, RC_GEN, RC_DBL, RC_IDX, RC_SP, RC_AF, RC_I, RC_R, RC_CC, NUM_RC };

// Z80 register type definition is regular
struct z80_reg_t : tgt::tgt_reg<z80_reg_t>
{
    using hw_tst         = hw::hw_tst;
    using reg_defn_idx_t = uint8_t;

    using base_t::base_t;
};

// reg_set holds [register + offset] values
struct z80_reg_set : tgt::tgt_reg_set<z80_reg_set, z80_reg_t>
{
    using base_t::base_t;
};

// declare a "reference" for register_set type
using z80_rs_ref = typename z80_reg_set::ref_loc_t;
}

namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for Z80 register tokens
    using z80_reg_parser_p = x3::rule<struct X_reg, z80_reg_t>;
    BOOST_SPIRIT_DECLARE(z80_reg_parser_p)
}


#endif
