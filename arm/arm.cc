// Arch definitions for ARM

#include "utility/print_type_name.h"

#include "expr/expr.h"
#include "parser/parser.h"
#include "target/tgt_mcode_defn.h"
#include "target/tgt_reg_defn.h"
#include "parser/sym_parser.h"

// per-arch includes
#include "arm_mcode.h"
#include "arm_reg_defn.h"

// instruction definitions
#include "insns_arm5.h"
#include "insns_arm6.h"
#include "insns_thumb.h"
#include "insns_thumb32.h"

// directives implementation
#include "arm_opc_directives.h"
#include "arm_opc_eabi.h"

// parse instruction + args
#include "arm_parser_def.h"

// arch impl files
#include "arm_stmt_impl.h"
#include "arm_mcode_impl.h"
#include "arm_arg_impl.h"
#include "arm_arg_serialize.h"
#include "target/tgt_directives_impl.h"

// instantiate global values controlling assembly
namespace kas::arm::hw
{
    cpu_defs_t      cpu_defs{};
}

#ifdef TARGET_HAS_REG_PFX
// processing for register prefixes
namespace kas::arm
{
    arm_reg_prefix arm_reg_t::reg_pfx { PFX_ALLOW };
    template arm_addr_map_t<
}
#endif

namespace kas::arm::parser::bnf
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // ARM Register Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // generate symbol parser for register names
    using reg_name_parser_t = sym_parser_t<
                                  typename arm_reg_t::defn_t
                                , reg_defn::arm_all_reg_l
                                , tgt::tgt_reg_adder<arm_reg_t
                                                  , reg_defn::arm_reg_aliases_l
                                                  >>;
    // define parser instance for register name parser
    arm_reg_x3 reg_parser {"arm reg"};

    using tok_arm_reg = typename arm_reg_t::token_t;
    auto const raw_reg_parser = x3::no_case[reg_name_parser_t(hw::cpu_defs).x3()];
    auto const reg_parser_def = parser::token<tok_arm_reg>[raw_reg_parser];
    
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(arm_reg_x3        , iterator_type, expr_context_type)

    
    //////////////////////////////////////////////////////////////////////////
    // ARM Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::arm_insn_defn_list
                                  , opc::arm_insn_defn_groups
                                  , meta::quote<meta::_t>>;

    using arm_insn_defn         = typename arm_mcode_t::defn_t;
    using arm_insn_adder        = typename arm_mcode_t::adder_t;
    using arm_insn_sym_parser_t = sym_parser_t<arm_insn_defn, insns, arm_insn_adder>;

    arm_insn_sym_parser_t insn_sym_parser(hw::cpu_defs);

    // parser for opcode names
    arm_insn_x3 arm_insn_parser {"arm opcode"};
   
    // use `x3()` if insn has no suffix. use `x3_raw() if insn suffix allowed
    using tok_arm_insn = typename arm_insn_t::token_t;
    auto const raw_insn_parser = x3::no_case[arm_insn_sym_parser_t(hw::cpu_defs).x3_raw()];
    auto const arm_insn_parser_def = parser::token<tok_arm_insn>[raw_insn_parser];
    
    BOOST_SPIRIT_DEFINE(arm_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(arm_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(arm_stmt_x3, iterator_type, stmt_context_type)

    //////////////////////////////////////////////////////////////////////////
    // ARM Directive Parser Definition
    //////////////////////////////////////////////////////////////////////////

    using arm_dir_op_parser_t = sym_parser_t<tgt_directive_t, arm_ops>;
    arm_dir_op_x3 arm_dir_op_parser {"arm directives"};

    auto const arm_dir_op_parser_def = 
            x3::no_case[x3::lexeme['.' >> arm_dir_op_parser_t().x3()]];

    BOOST_SPIRIT_DEFINE(arm_dir_op_parser)

    BOOST_SPIRIT_INSTANTIATE(arm_dir_op_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(arm_dir_x3   , iterator_type, stmt_context_type)
}

namespace kas::tgt
{
    // for target instantiatiation
    using ARCH_MCODE = arm::arm_mcode_t;
}

// instantiate target types
#include "target/tgt_impl.h"
