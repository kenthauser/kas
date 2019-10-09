// Arch definitions for M68K & Coldfire processors

#include "expr/expr.h"
#include "parser/parser.h"
#include "parser/sym_parser.h"
#include "target/tgt_mcode_defn.h"
#include "target/tgt_reg_defn.h"

#include "utility/print_type_name.h"

// per-arch includes
#include "m68k_mcode.h"
#include "m68k_reg_defn.h"

// instruction definitions
#include "insns_m68000.h"
#if 1
#include "insns_m68020.h"
#include "insns_m68040.h"
//#include "insns_m68851.h"
#include "insns_m68881.h"
#include "insns_cpu32.h"
//#include "insns_coldfire.h"
#endif

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
#include "expr/literal_types_impl.h"

#include "mit_arg_ostream.h"
#include "moto_arg_ostream.h"

namespace kas::m68k::hw
{
    cpu_defs_t cpu_defs;
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
                                                  >
                                >;

    // define parser instance for register name parser
    m68k_reg_x3 reg_parser {"m68k reg"};
    auto reg_parser_def = reg_name_parser_t().x3_deref();
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(m68k_reg_x3 , iterator_type, expr_context_type)

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

    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    m68k_insn_sym_parser_t insn_sym_parser;

    // parser for opcode names
    m68k_insn_x3 m68k_insn_parser {"m68k opcode"};
    
    auto const m68k_insn_parser_def = insn_sym_parser.x3_raw();
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
    using ARCH_MCODE = m68k::m68k_mcode_t;
}

#include "target/tgt_impl.h"
