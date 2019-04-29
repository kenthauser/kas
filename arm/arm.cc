// Arch definitions for ARM processor

#include "expr/expr.h"
#include "parser/parser.h"
#include "parser/sym_parser.h"
#include "target/tgt_mcode_defn.h"
#include "target/tgt_reg_defn.h"

#include "utility/print_type_name.h"

// per-arch customizations 
#include "arm_mcode.h"

// register definitions
#include "arm_reg_defn.h"

// instruction definitions
#include "insns_arm.h"

// parse instruction + args
#include "arm_parser.h"

// arch impl files
#include "arm_arg_impl.h"
#include "arm_mcode_impl.h"

namespace kas::arm::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // Register Parser Definition
    //////////////////////////////////////////////////////////////////////////
   
    // generate symbol parser for register names
    using reg_name_parser_t = sym_parser_t<
                                  typename arm_reg_t::defn_t
                                , reg_defn::reg_l
                                , tgt::tgt_reg_adder<arm_reg_t
                                                  , reg_defn::reg_aliases_l
                                                  >
                                >;

    // define parser instance for register name parser
    arm_reg_x3 reg_parser {"arm reg"};
    auto reg_parser_def = reg_name_parser_t().x3_deref();
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(arm_reg_x3 , iterator_type, expr_context_type)

    //////////////////////////////////////////////////////////////////////////
    // Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////
   
    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::arm_insn_defn_list
                                  , opc::arm_insn_defn_groups
                                  , meta::quote<meta::_t>
                                  >;

    using arm_insn_parser_t = sym_parser_t<typename arm_mcode_t::defn_t, insns>;


    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    arm_insn_parser_t insn_sym_parser;

    // parser for opcode names
    arm_insn_x3 arm_insn_parser {"arm opcode"};
    
    auto const arm_insn_parser_def = insn_sym_parser.x3();
    BOOST_SPIRIT_DEFINE(arm_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(arm_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(arm_stmt_x3, iterator_type, stmt_context_type)
}

// before including `tgt_impl`, define `ARCH_MCODE` in namespace `kas::tgt`
// This instantiates all CRTP templated methods in `tgt` types

namespace kas::tgt
{
    using ARCH_MCODE = arm::arm_mcode_t;
}

#include "target/tgt_impl.h"

