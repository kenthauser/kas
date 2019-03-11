#if 0
#include "m68k.h"
#include "m68k_stmt.h"
#include "m68k_arg_size.h"

#include "m68k_reg_defns.h"
//#include "m68k_reg_impl.h"

//#include "m68k_reg_adder.h"

#include "m68k_insn_types.h"
#include "m68k_insn_adder.h"

#include "insns_m68000.h"
#include "insns_m68020.h"
#include "insns_m68040.h"
#include "insns_m68881.h"
#include "insns_cpu32.h"
#include "insns_coldfire.h"

#include "m68k_insn_impl.h"

#include "target/tgt_reg_impl.h"
#include "target/tgt_regset_impl.h"

// meta program to instantiate defns from type list
#include "parser/sym_parser.h"
#endif
namespace kas::m68k::parser
{
#if 0
    namespace x3 = boost::spirit::x3;
    using namespace x3;

    using kas::parser::iterator_type;
    using kas::parser::context_type;

    //////////////////////////////////////////////////////////////////////////
    //  Assembler Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////

#if 0
    auto const m68k_reg_parser_def = defn::reg_parser.x3_deref();

#else
    const auto m68k_reg_parser_def = kas::parser::sym_parser_t<
                                          typename m68k_reg::defn_t
                                        , defn::m68k_all_reg_l
                                        , tgt::tgt_reg_adder<m68k_reg, defn::m68k_reg_aliases_l>
                                        >().x3_deref();
#endif

    m68k_reg_parser_p m68k_reg_parser = "m68k reg";
    BOOST_SPIRIT_DEFINE(m68k_reg_parser);
    BOOST_SPIRIT_INSTANTIATE(
        m68k_reg_parser_p, iterator_type, context_type)
    
    m68k_insn_parser_type m68k_insn_parser_p = "m68k opcode";
    using insns = all_defns_flatten<opc::m68k_insn_defn_list
                                , opc::m68k_insn_defn_groups
                                , meta::quote<meta::_t>>;

    // x3 parser type for all insn names
    using m68k_insn_x3 = kas::parser::sym_parser_t<opc::m68k_insn_defn, insns>;

    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    auto const m68k_insn_parser_p_def = m68k_insn_x3{}.x3();

    BOOST_SPIRIT_DEFINE(m68k_insn_parser_p);

    BOOST_SPIRIT_INSTANTIATE(
        m68k_insn_parser_type, iterator_type, context_type)

    m68k_insn_parser_type const& m68k_insn_parser()
    {
        static m68k_insn_parser_type _p;
        return _p;
    }
#endif
}
#if 0
// instantiate printers
namespace kas::tgt
{
    template auto m68k::m68k_reg_set::binop(const char, tgt_reg_set const&) -> derived_t&;
    template auto m68k::m68k_reg_set::binop(const char, core::core_expr&)   -> derived_t&;
    template const char *m68k::m68k_reg::validate(int) const;
    // template  void print_expr<std::ostream>(m68k::m68k_reg const&, std::ostream&);
    template  void m68k::m68k_reg_set::print<std::ostream>(std::ostream&) const;
    template  void m68k::m68k_reg_set::print<std::ostringstream>(std::ostringstream&) const;
}
#endif
