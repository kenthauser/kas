// Arch definitions for PROTO_UC

#include "utility/print_type_name.h"

#include "expr/expr.h"
#include "parser/parser.h"
#include "target/tgt_mcode_defn.h"
#include "target/tgt_reg_defn.h"
#include "parser/sym_parser.h"

// per-arch includes
#include "PROTO_LC_mcode.h"
#include "PROTO_LC_reg_defn.h"

// instruction definitions
#include "insns_PROTO_LC.h"

// parse instruction + args
#include "PROTO_LC_parser_def.h"

// arch impl files
#include "PROTO_LC_stmt_impl.h"
#include "PROTO_LC_mcode_impl.h"
#include "PROTO_LC_arg_impl.h"

// instantiate global values controlling assembly
namespace kas::PROTO_LC::hw
{
    cpu_defs_t      cpu_defs{};
}

#ifdef TARGET_HAS_REG_PFX
// processing for register prefixes
namespace kas::PROTO_LC
{
    PROTO_LC_reg_prefix PROTO_LC_reg_t::reg_pfx { PFX_ALLOW };
}
#endif

namespace kas::PROTO_LC::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // Register Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // generate symbol parser for register names
    using reg_name_parser_t = sym_parser_t<
                                  typename PROTO_LC_reg_t::defn_t
                                , reg_defn::PROTO_LC_all_reg_l
                                , tgt::tgt_reg_adder<PROTO_LC_reg_t
                                                  , reg_defn::PROTO_LC_reg_aliases_l
                                                  >>;
    // define parser instance for register name parser
    PROTO_LC_reg_x3 reg_parser {"PROTO_LC reg"};

    using tok_PROTO_LC_reg = typename PROTO_LC_reg_t::token_t;
    auto const raw_reg_parser = x3::no_case[reg_name_parser_t(hw::cpu_defs).x3()];
    auto const reg_parser_def = parser::token<tok_PROTO_LC_reg>[raw_reg_parser];
    
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(PROTO_LC_reg_x3        , iterator_type, expr_context_type)

    
    //////////////////////////////////////////////////////////////////////////
    // Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::PROTO_LC_insn_defn_list
                                  , opc::PROTO_LC_insn_defn_groups
                                  , meta::quote<meta::_t>>;

    using PROTO_LC_insn_defn         = typename PROTO_LC_mcode_t::defn_t;
    using PROTO_LC_insn_adder        = typename PROTO_LC_mcode_t::adder_t;
    using PROTO_LC_insn_sym_parser_t = sym_parser_t<PROTO_LC_insn_defn, insns, PROTO_LC_insn_adder>;

    PROTO_LC_insn_sym_parser_t insn_sym_parser(hw::cpu_defs);

    // parser for opcode names
    PROTO_LC_insn_x3 PROTO_LC_insn_parser {"PROTO_LC opcode"};
   
    // use `x3()` if insn has no suffix. use `x3_raw() if insn suffix allowed
    using tok_PROTO_LC_insn = typename PROTO_LC_insn_t::token_t;
    auto const raw_insn_parser = x3::no_case[PROTO_LC_insn_sym_parser_t(hw::cpu_defs).x3_raw()];
    auto const PROTO_LC_insn_parser_def = parser::token<tok_PROTO_LC_insn>[raw_insn_parser];
    
    BOOST_SPIRIT_DEFINE(PROTO_LC_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(PROTO_LC_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(PROTO_LC_stmt_x3, iterator_type, stmt_context_type)
}

namespace kas::tgt
{
    // for target instantiatiation
    using ARCH_MCODE = PROTO_LC::PROTO_LC_mcode_t;
}

// instantiate target types
#include "target/tgt_impl.h"
