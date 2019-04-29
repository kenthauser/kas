#ifndef KAS_ARM_ARM_REG_TYPES_H
#define KAS_ARM_ARM_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare register constants
// Derive  register types from `target` CRTP base types
//
// Declare `x3` parser for `reg_t`
//
////////////////////////////////////////////////////////////////////////////

#include "expr/expr_types.h"

#include "arm_hw_defns.h"
#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"

namespace kas::arm 
{

// Declare Register "Classes" for ARM
enum { RC_NONE
     , RC_GEN
     , RC_FLT_SGL
     , RC_FLT_DBL
     , RC_FLT_QUAD
     , RC_CPU
     , NUM_RC
     };

enum { REG_CPU_APSR             // Application Program Status Register
     , REG_CPU_FPSCR            // Floating-point Status and Control Register
     , REG_CPU_PSR              // Program Status Register
     // system registers
     , REG_CPU_CPSR             // current program status register
     , REG_CPU_SPSR             // saved CPSR (before exception)
};

// ARM register type definition is regular
struct arm_reg_set;
struct arm_reg_t : tgt::tgt_reg<arm_reg_t>
{
    using hw_tst         = hw::hw_tst;
    using reg_defn_idx_t = uint8_t;
    using reg_set_t      = arm_reg_set;
    
    using base_t::base_t;       // use inherited ctors
};

// reg_set holds [register + offset] values
struct arm_reg_set : tgt::tgt_reg_set<arm_reg_set, arm_reg_t>
{
    using base_t::base_t;
};

// alias the "reference" used for for register_set type
using arm_rs_ref = typename arm_reg_set::ref_loc_t;
}

// declare X3 parser for `reg_t`
namespace kas::arm::parser
{
    namespace x3 = boost::spirit::x3;
    
    // declare parser for ARM register tokens
    using arm_reg_x3 = x3::rule<struct X_reg, arm_reg_t>;
    BOOST_SPIRIT_DECLARE(arm_reg_x3)
}


#endif
