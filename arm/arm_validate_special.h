#ifndef KAS_ARM_VALIDATE_SPECIAL_H
#define KAS_ARM_VALIDATE_SPECIAL_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "arm_validate.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"

namespace kas::arm::parser
{
namespace detail
{
    using arm_token_base_t =  x3::symbols<expression::e_fixed_t>;
    struct arm_section_type_x3 : arm_token_base_t
    {
        arm_section_type_x3()
        {
            add
                ("progbits"         , SHT_PROGBITS)
                ("nobits"           , SHT_NOBITS)
                ("note"             , SHT_NOTE)
                ("init_array"       , SHT_INIT_ARRAY)
                ("fini_array"       , SHT_FINI_ARRAY)
                ("preinit_array"    , SHT_PREINIT_ARRAY)
                ;
        }
    };

    
}
}




namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;
using arg_mode_t  = typename arm_mcode_t::arg_mode_t;

}

#endif
