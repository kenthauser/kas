#include "utility/print_type_name.h"

#include "m68k.h"
#include "m68k_mcode.h"

#include "m68k_reg_defns.h"

#include "mit_moto_names.h"
#include "target/tgt_insn_adder.h"

#include "m68k/insns_m68000.h"
//#include "m68k/insns_m68020.h"
//#include "m68k/insns_m68040.h"
//#include "m68k/insns_m68000.h"
//#include "m68k/insns_m68851.h"
//#include "m68k/insns_m68881.h"
//#include "m68k/insns_cpu32.h"
//#include "m68k/insns_coldfire.h"

#include "m68k_arg_impl.h"
#include "m68k_arg_size.h"
#include "m68k_arg_serialize.h"
//#include "m68k_opcode_emit.h"

// parse m68k instruction + args
#include "mit_moto_parser_def.h"

#include "mit_moto_names.h"
#include "mit_arg_ostream.h"
#include "moto_arg_ostream.h"

// boilerplate: tgt_impl & sym_parser (for insn & reg names)
#include "parser/sym_parser.h"
#include "target/tgt_reg_impl.h"
#include "target/tgt_regset_impl.h"
#include "target/tgt_stmt_impl.h"
#include "target/tgt_insn_impl.h"
#include "target/tgt_insn_eval.h"

#include <typeinfo>
#include <iostream>

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
                                , defn::m68k_all_reg_l
                                , tgt::tgt_reg_adder<m68k_reg_t, defn::m68k_reg_aliases_l>
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
    using insns = all_defns_flatten<opc::m68k_insn_defn_list
                                , opc::m68k_insn_defn_groups
                                , meta::quote<meta::_t>>;
#if 1
    using m68k_insn_defn         = typename m68k_mcode_t::defn_t;
    using m68k_insn_adder        = typename m68k_mcode_t::adder_t;
    using m68k_insn_sym_parser_t = sym_parser_t<m68k_insn_defn, insns, m68k_insn_adder>;
#endif

#if 1
    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    m68k_insn_sym_parser_t insn_sym_parser;

    // parser for opcode names
    m68k_insn_x3 m68k_insn_parser {"m68k opcode"};
    
    auto const m68k_insn_parser_def = insn_sym_parser.x3();
    BOOST_SPIRIT_DEFINE(m68k_insn_parser);

    // instantiate parsers
    BOOST_SPIRIT_INSTANTIATE(m68k_insn_x3, iterator_type, stmt_context_type)
    BOOST_SPIRIT_INSTANTIATE(m68k_stmt_x3, iterator_type, stmt_context_type)
#endif
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
    // name types used to instantiate the CRTP templates: reg, reg_set, stmt
    using reg_t     = m68k::m68k_reg_t;
    using reg_set_t = m68k::m68k_reg_set;
    using arg_t     = m68k::m68k_arg_t;
    using insn_t    = m68k::m68k_insn_t;
    
    // instantiate reg routines referenced from expression parsers
    template const char *tgt_reg<reg_t>::validate(int) const;

    // instantiate reg_set routines referenced from expression parsers
    template      tgt_reg_set<reg_set_t, reg_t>::tgt_reg_set(reg_t const&, char);
    template auto tgt_reg_set<reg_set_t, reg_t>::base_t::binop(const char, tgt_reg_set const&) -> derived_t&;
    template auto tgt_reg_set<reg_set_t, reg_t>::binop(const char, core::core_expr const&)   -> derived_t&;
    template auto tgt_reg_set<reg_set_t, reg_t>::binop(const char, int)   -> derived_t&;
    
    // instantiate routines referenced from stmt parsers
    template core::opcode *tgt_stmt<insn_t, arg_t>::gen_insn(core::opcode::data_t&);
    template std::string   tgt_stmt<insn_t, arg_t>::name() const;

    // instantiate printers
    template void tgt_reg_set<reg_set_t, reg_t>::print<std::ostream>(std::ostream&) const;
    template void tgt_reg_set<reg_set_t, reg_t>::print<std::ostringstream>(std::ostringstream&) const;
}
#if 0
    struct _xxx
    {
        _xxx()
        {
            tgt::opc::tgt_opc_general<m68k_mcode_t> opc_gen;
            tgt::opc::tgt_opc_list   <m68k_mcode_t> opc_list;
            opc::FMT_LIST fmt_list;
            opc::FMT_X    fmt_x;
            
            opc::REG_GEN  reg_gen;

            print_type_name{"insns"}.name<insns>();
            print_type_name{"m68k_insn_defn"}.name<m68k_insn_defn>();
            print_type_name{"m68k_insn_adder"}.name<m68k_insn_adder>();
            print_type_name{"opc_gen"} (opc_gen);
            print_type_name{"opc_list"}(opc_list);
            print_type_name{"fmt_list"}(fmt_list);
            print_type_name{"fmt_x"}(fmt_x);
            print_type_name{"reg_gen"}(reg_gen);

            print_type_name{"m68k_insn_sym_parser_t"}.name<m68k_insn_sym_parser_t>();

            using xlate_list = typename m68k_insn_sym_parser_t::xlate_list;
            print_type_name{"xlate_list"}.name<xlate_list>();

            std::cout << "\nxlate_list[...] size = " << meta::size<xlate_list>() << std::endl;
            print_type_name{"elem 0"}.name<meta::at_c<xlate_list, 0>>();
            print_type_name{"elem 1"}.name<meta::at_c<xlate_list, 1>>();
            print_type_name{"elem 2"}.name<meta::at_c<xlate_list, 2>>();
            print_type_name{"elem 3"}.name<meta::at_c<xlate_list, 3>>();

            std::cout << std::endl;

            using all_types_defns = typename m68k_insn_sym_parser_t::all_types_defns;
            print_type_name{"all_types_defns"}.name<all_types_defns>();
            std::cout << "\nall_types_defns[...]  size = ";
            std::cout << meta::size<all_types_defns>() << std::endl;
            print_type_name{"elem 0"}.name<meta::at_c<all_types_defns, 0>>();
            print_type_name{"elem 1"}.name<meta::at_c<all_types_defns, 1>>();
            print_type_name{"elem 2"}.name<meta::at_c<all_types_defns, 2>>();
            print_type_name{"elem 3"}.name<meta::at_c<all_types_defns, 3>>();
        }
    } ;//_xxx;
#endif

