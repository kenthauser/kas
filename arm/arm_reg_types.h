#ifndef KAS_ARM_ARM_REG_TYPES_H
#define KAS_ARM_ARM_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare register constants
// Derive  register types from `target` CRTP base types
//
////////////////////////////////////////////////////////////////////////////

#include "expr/expr_types.h"

#include "arm_hw_defns.h"
#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"

namespace kas::arm 
{

// forward declare CRTP register type
struct arm_reg_t;

// reg_set holds [register + offset] values
template <typename Ref>
struct arm_reg_set : tgt::tgt_reg_set<arm_reg_set<Ref>, arm_reg_t, Ref>
{
    using base_t = tgt::tgt_reg_set<arm_reg_set<Ref>, arm_reg_t, Ref>;
    using base_t::base_t;
};

// alias the "reference" used for for register_set type
using arm_rs_ref    = core::ref_loc_tpl<arm_reg_set>;
using arm_reg_set_t = typename arm_rs_ref::object_t;

// Declare Register "Classes" for ARM
enum { RC_NONE
     , RC_GEN
     , RC_FLT_SGL
     , RC_FLT_DBL
     , RC_FLT_QUAD
     , RC_CPU
     , NUM_RC
     };

// Declare "named" registers for ARM
enum { REG_CPU_APSR             // Application Program Status Register
     , REG_CPU_FPSCR            // Floating-point Status and Control Register
     , REG_CPU_PSR              // Program Status Register
     // system registers
     , REG_CPU_CPSR             // current program status register
     , REG_CPU_SPSR             // saved CPSR (before exception)
};

// ARM register type definition is regular
struct arm_reg_t : tgt::tgt_reg<arm_reg_t, KAS_STRING("ARM")
                              , hw::arm_hw_defs, arm_reg_set_t>
{
    using base_t::base_t;       // use inherited ctors
};

// add to `expr` expression types
using arm_reg_ref = core::ref_loc_t<arm_reg_t>;
}

#endif
