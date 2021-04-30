#ifndef KAS_PROTO_UC_PROTO_UC_REG_TYPES_H
#define KAS_PROTO_UC_PROTO_UC_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare register constants
// Derive  register types from `target` CRTP base types
//
// Declare `x3` parser for `reg_t`
//
////////////////////////////////////////////////////////////////////////////

#include "expr/expr_types.h"

#include "PROTO_LC_hw_defns.h"
#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"

namespace kas::PROTO_LC 
{

// forward declare CRTP register type
struct PROTO_LC_reg_t;

// reg_set holds [register + offset] values
template <typename Ref>
struct PROTO_LC_reg_set : tgt::tgt_reg_set<PROTO_LC_reg_set<Ref>, PROTO_LC_reg_t, Ref>
{
    using base_t = tgt::tgt_reg_set<PROTO_LC_reg_set<Ref>, PROTO_LC_reg_t, Ref>;
    using base_t::base_t;
};

// alias the "reference" used for for register_set type
using PROTO_LC_rs_ref    = core::ref_loc_tpl<PROTO_LC_reg_set>;
using PROTO_LC_reg_set_t = typename PROTO_LC_rs_ref::object_t;

// EXAMPLE: Declare Register "Classes" for PROTO_UC
enum { RC_NONE, RC_GEN, RC_CPU };

// PROTO_UC register type definition is regular
struct PROTO_LC_reg_t : tgt::tgt_reg<PROTO_LC_reg_t, KAS_STRING("PROTO_UC")
                               , hw::PROTO_LC_hw_defs, PROTO_LC_reg_set_t>
{
    using reg_defn_idx_t = uint8_t;
    using base_t::base_t;       // use inherited ctors
};

// add to `expr` expression types
using PROTO_LC_reg_ref = core::ref_loc_t<PROTO_LC_reg_t>;
}

#endif
