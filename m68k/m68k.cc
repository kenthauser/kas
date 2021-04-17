// Arch definitions for M68K & Coldfire processors

#include "utility/print_type_name.h"

#include "expr/expr.h"
#include "parser/parser.h"
#include "parser/sym_parser.h"

// per-arch includes
#include "m68k_mcode.h"
#include "m68k_reg_defn.h"

// instruction definitions
#include "insns_m68000.h"
#include "insns_m68020.h"
#include "insns_m68040.h"
#include "insns_cpu32.h"

#include "insns_m68881.h"
#include "insns_m68851.h"

#include "insns_coldfire.h"

// parse instruction + args
#include "mit_moto_parser_def.h"

// arch impl files
#include "mit_moto_names.h"
#include "m68k_stmt_impl.h"
#include "m68k_mcode_impl.h"
#include "m68k_arg_impl.h"
#include "m68k_extension_impl.h"
#include "m68k_arg_emit.h"
#include "m68k_arg_size.h"
#include "m68k_arg_serialize.h"

#include "mit_arg_ostream.h"
#include "moto_arg_ostream.h"

// instantiate global values controlling assembly
namespace kas::m68k::hw
{
    cpu_defs_t      cpu_defs{};
}

namespace kas::m68k
{
    m68k_reg_prefix m68k_reg_t::reg_pfx { PFX_ALLOW };
}

namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // Register Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // generate symbol parser for register names
    using reg_name_parser_t = sym_parser_t<
                                  typename m68k_reg_t::defn_t
                                , reg_defn::m68k_all_reg_l
                                , tgt::tgt_reg_adder<m68k_reg_t
                                                  , reg_defn::m68k_reg_aliases_l
                                                  >>;
    // define parser instance for register name parser
    m68k_reg_x3 reg_parser {"m68k reg"};

    using tok_m68k_reg = typename m68k_reg_t::token_t;
    auto const raw_reg_parser = x3::no_case[reg_name_parser_t(hw::cpu_defs).x3()];
    auto const reg_parser_def = parser::token<tok_m68k_reg>[raw_reg_parser];
    
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(m68k_reg_x3        , iterator_type, expr_context_type)
    BOOST_SPIRIT_INSTANTIATE(m68k_sized_fixed_x3, iterator_type, expr_context_type)

    
    //////////////////////////////////////////////////////////////////////////
    // Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////

    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::m68k_defn_list
                                , opc::m68k_defn_groups
                                , meta::quote<meta::_t>>;

    using m68k_insn_defn         = typename m68k_mcode_t::defn_t;
    using m68k_insn_adder        = typename m68k_mcode_t::adder_t;
    using m68k_insn_sym_parser_t = sym_parser_t<m68k_insn_defn, insns, m68k_insn_adder>;

    m68k_insn_sym_parser_t insn_sym_parser(hw::cpu_defs);

    // parser for opcode names
    m68k_insn_x3 m68k_insn_parser {"m68k opcode"};
   
    // use `x3()` if insn has no suffix. use `x3_raw() if insn suffix allowed
    using tok_m68k_insn = typename m68k_insn_t::token_t;
    auto const raw_insn_parser = x3::no_case[m68k_insn_sym_parser_t(hw::cpu_defs).x3_raw()];
    auto const m68k_insn_parser_def = parser::token<tok_m68k_insn>[raw_insn_parser];
    
    BOOST_SPIRIT_DEFINE(m68k_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(m68k_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(m68k_stmt_x3, iterator_type, stmt_context_type)
}

namespace kas::m68k
{
    // use MIT ostream format
    std::ostream& operator<<(std::ostream& os, m68k_arg_t const& arg)
    {
        return mit_arg_ostream(os, arg);
    }
}

namespace kas::tgt
{
    // for target instantiatiation
    using ARCH_MCODE = m68k::m68k_mcode_t;
}

// instantiate target types
#include "target/tgt_impl.h"
