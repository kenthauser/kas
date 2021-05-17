#ifndef KAS_Z80_Z80_REG_TYPES_H
#define KAS_Z80_Z80_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare register constants
// Derive  register types from `target` CRTP base types
//
// Declare `x3` parser for `reg_t`
//
////////////////////////////////////////////////////////////////////////////

#include "expr/expr_types.h"

#include "z80_hw_defns.h"
#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"

namespace kas::z80 
{

// forward declare CRTP register type
struct z80_reg_t;

// reg_set holds [register + offset] values
template <typename Ref>
struct z80_reg_set : tgt::tgt_reg_set<z80_reg_set<Ref>, z80_reg_t, Ref>
{
    using base_t = tgt::tgt_reg_set<z80_reg_set<Ref>, z80_reg_t, Ref>;
    using base_t::base_t;
};

// alias the "reference" used for for register_set type
using z80_rs_ref    = core::ref_loc_tpl<z80_reg_set>;
using z80_reg_set_t = typename z80_rs_ref::object_t;

// Declare Register "Classes" for Z80
enum { RC_NONE, RC_GEN, RC_DBL, RC_IDX, RC_SP, RC_AF, RC_I, RC_R, RC_CC, NUM_RC };
 

// Z80 register type definition is regular
struct z80_reg_t : tgt::tgt_reg<z80_reg_t, KAS_STRING("Z80")
                               , hw::z80_hw_defs, z80_reg_set_t>
{
    using reg_defn_idx_t = uint8_t;
    using base_t::base_t;       // use inherited ctors
};

// add to `expr` expression types
using z80_reg_ref = core::ref_loc_t<z80_reg_t>;
}

#endif
