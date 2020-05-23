// Arch definitions for Z80 processor

#include "expr/expr.h"
#include "parser/parser.h"
#include "parser/sym_parser.h"
#include "target/tgt_mcode_defn.h"
#include "target/tgt_reg_defn.h"

#include "utility/print_type_name.h"

// per-arch customizations 
#include "z80_mcode.h"

// register definitions
#include "z80_reg_defn.h"

// instruction definitions
#include "insns_z80.h"

// parse instruction + args
#include "z80_parser.h"

// arch impl files
#include "z80_arg_impl.h"
#include "z80_mcode_impl.h"

#include "elf/elf_convert_elf.h"

namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // Register Parser Definition
    //////////////////////////////////////////////////////////////////////////
   
    // generate symbol parser for register names
    using reg_name_parser_t = sym_parser_t<
                                  typename z80_reg_t::defn_t
                                , reg_defn::reg_l
                                , tgt::tgt_reg_adder<z80_reg_t
                                                  , reg_defn::reg_aliases_l
                                                  >
                                >;
    // define parser instance for register name parser
    z80_reg_x3 reg_parser {"z80 reg"};
    auto const raw_reg_parser = x3::no_case[reg_name_parser_t().x3()];
    auto const reg_parser_def = parser::token<tok_z80_reg>[raw_reg_parser];

    BOOST_SPIRIT_DEFINE(reg_parser)
    
    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(z80_reg_x3 , iterator_type, expr_context_type)

    //////////////////////////////////////////////////////////////////////////
    // Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////
   
    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::z80_insn_defn_list
                                  , opc::z80_insn_defn_groups
                                  , meta::quote<meta::_t>
                                  >;

    using z80_insn_parser_t = sym_parser_t<typename z80_mcode_t::defn_t, insns>;


    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    z80_insn_parser_t insn_sym_parser;

    // parser for opcode names
    z80_insn_x3 z80_insn_parser {"z80 opcode"};
    
    auto const z80_insn_parser_def = insn_sym_parser.x3();
    BOOST_SPIRIT_DEFINE(z80_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(z80_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(z80_stmt_x3, iterator_type, stmt_context_type)
}

// before including `tgt_impl`, define `ARCH_MCODE` in namespace `kas::tgt`
// This instantiates all CRTP templated methods in `tgt` types

namespace kas::tgt
{
    using ARCH_MCODE = z80::z80_mcode_t;
}

#include "target/tgt_impl.h"

