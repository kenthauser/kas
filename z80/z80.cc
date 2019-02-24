
#include "z80.h"
#include "z80_mcode.h"

#include "z80_reg_defn.h"

//#include "z80_reg_adder.h"
//#include "z80_insn_types.h"

#include "target/tgt_insn_adder.h"
#include "insns_z80.h"
#include "z80_arg_impl.h"

// parse z80 instruction + args
//#include "z80_parser.h"


// boilerplate: tgt_impl & sym_parser (for insn & reg names)
#include "parser/sym_parser.h"
#include "target/tgt_reg_impl.h"
#include "target/tgt_regset_impl.h"
#include "target/tgt_stmt_impl.h"
#include "target/tgt_insn_impl.h"

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
                                , reg_l
                                , tgt::tgt_reg_adder<z80_reg_t, reg_aliases_l>
                                >;

    // define parser instance for register name parser
    z80_reg_x3 reg_parser {"z80 reg"};
    auto reg_parser_def = reg_name_parser_t().x3_deref();
    BOOST_SPIRIT_DEFINE(reg_parser)

    // instantiate parser `type` for register name parser
    BOOST_SPIRIT_INSTANTIATE(z80_reg_x3 , iterator_type, context_type)

#if 0
    //////////////////////////////////////////////////////////////////////////
    // Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////
   
    // combine all `insn` defns into single list & create symbol parser 
    using insns = all_defns_flatten<opc::z80_insn_defn_list
                                , opc::z80_insn_defn_groups
                                , meta::quote<meta::_t>>;

    using z80_insn_defn         = typename z80_mcode_t::defn_t;
    using z80_insn_sym_parser_t = sym_parser_t<z80_insn_defn, insns>;

    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    z80_insn_sym_parser_t insn_sym_parser;

    // parser for opcode names
    z80_insn_x3 z80_insn_parser {"z80 opcode"};
    
    //auto const z80_insn_parser_def = insn_sym_parser.x3();
    //BOOST_SPIRIT_DEFINE(z80_insn_parser);

#if 0 
    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(z80_insn_x3, iterator_type, context_type)
    BOOST_SPIRIT_INSTANTIATE(z80_stmt_x3, iterator_type, context_type)
#endif
#endif
}

namespace kas::tgt
{
    // name types used to instantiate the CRTP templates: reg, reg_set, stmt
    using reg_t     = z80::z80_reg_t;
    using reg_set_t = z80::z80_reg_set;
    using arg_t     = z80::z80_arg_t;
    using insn_t    = z80::z80_insn_t;
    
    // instantiate reg routines referenced from expression parsers
    template const char *tgt_reg<reg_t>::validate(int) const;

    // instantiate reg_set routines referenced from expression parsers
    template      tgt_reg_set<reg_set_t, reg_t>::tgt_reg_set(reg_t const&, char);
    template auto tgt_reg_set<reg_set_t, reg_t>::base_t::binop(const char, tgt_reg_set const&) -> derived_t&;
    template auto tgt_reg_set<reg_set_t, reg_t>::binop(const char, core::core_expr const&)   -> derived_t&;
    template auto tgt_reg_set<reg_set_t, reg_t>::binop(const char, int)   -> derived_t&;
    
    // instantiate routines referenced from stmt parsers
    template core::opcode *tgt_stmt<insn_t, arg_t>::eval(core::opcode::data_t&);
}

// instantiate printers
namespace kas::tgt
{
    // template  void print_expr<std::ostream>(z80::z80_reg const&, std::ostream&);
    template  void z80::z80_reg_set::print<std::ostream>(std::ostream&) const;
    template  void z80::z80_reg_set::print<std::ostringstream>(std::ostringstream&) const;
}
