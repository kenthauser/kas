
#include "z80.h"
#include "z80_mcode.h"

#include "z80_reg_defn.h"
//#include "z80_reg_impl.h"

//#include "z80_reg_adder.h"
//#include "z80_insn_types.h"

//#include "z80_insn_adder.h"
#include "insns_z80.h"
//#include "z80_insn_impl.h"
#include "z80_arg_impl.h"
#include "z80_parser.h"

#include "target/tgt_reg_impl.h"
#include "target/tgt_regset_impl.h"
//#include "target/tgt_insn_impl.h"

// meta program to instantiate defns from type list
//#include "parser/sym_parser.h"

namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;

    //////////////////////////////////////////////////////////////////////////
    //  Assembler Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////
    
    z80_reg_x3 z80_reg_parser = "z80 reg";
    const auto z80_reg_parser_def = kas::parser::sym_parser_t<
                                          typename z80_reg_t::defn_t
                                        , reg_l
                                        , tgt::tgt_reg_adder<z80_reg_t, reg_aliases_l>
                                        >().x3_deref();

    BOOST_SPIRIT_DEFINE(z80_reg_parser);
#if 0
    z80_insn_x3   z80_insn_parser = "z80 opcode";
    using insns = all_defns_flatten<opc::z80_insn_defn_list
                                , opc::z80_insn_defn_groups
                                , meta::quote<meta::_t>>;
#endif
#if 0
    // x3 parser type for all insn names
    using z80_insn_defn = tgt::opc::tgt_insn_defn<z80_mcode_t>;
    using z80_insn_x3 = kas::parser::sym_parser_t<z80_insn_defn, insns>;

    // XXX shoud stop parsing on (PARSER_CHARS | '.')
    auto const z80_insn_parser_p_def = z80_insn_x3{}.x3();

    BOOST_SPIRIT_DEFINE(z80_insn_parser_p);


    z80_insn_parser_type const& z80_insn_parser()
    {
        static z80_insn_parser_type _p;
        return _p;
    }
#endif
#if 0 
    // instantiate parsers
    using kas::parser::iterator_type;
    using kas::parser::context_type;

    BOOST_SPIRIT_INSTANTIATE(z80_reg_parser_x3 , iterator_type, context_type)
    BOOST_SPIRIT_INSTANTIATE(z80_insn_parser_x3, iterator_type, context_type)
    BOOST_SPIRIT_INSTANTIATE(z80_stmt_x3       , iterator_type, context_type)
#endif
}

#if 0
namespace kas::tgt
{
    template      z80::z80_reg_set::tgt_reg_set::tgt_reg_set(z80::z80_reg_t const&, char);
    template auto z80::z80_reg_set::base_t::binop(const char, tgt_reg_set const&) -> derived_t&;
    template auto z80::z80_reg_set::binop(const char, core::core_expr const&)   -> derived_t&;
    template auto z80::z80_reg_set::binop(const char, int)   -> derived_t&;
    template const char *z80::z80_reg_t::validate(int) const;
}
#else
namespace
{
    void _instantiate()
    {
        kas::z80::z80_reg_t reg{};
        kas::z80::z80_reg_set rs(reg);
        rs.operator-(reg);
        rs.operator/(reg);

        reg.validate(0);
    }

}
#endif
// instantiate printers
namespace kas::tgt
{
    // template  void print_expr<std::ostream>(z80::z80_reg const&, std::ostream&);
    template  void z80::z80_reg_set::print<std::ostream>(std::ostream&) const;
    template  void z80::z80_reg_set::print<std::ostringstream>(std::ostringstream&) const;
}
