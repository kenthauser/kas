#ifndef KAS_PARSER_PARSER_STMT_H
#define KAS_PARSER_PARSER_STMT_H

#include "expr/expr.h"
#include "kas_position.h"
#include "stmt_print.h"
#include "literal_parser.h"
#include "kas_core/opcode.h"

#include <boost/mpl/string.hpp>
#include <functional>

/*
 * Declare the statements used by the parser itself.
 *
 * These are the
 *
 * 1) "empty" stmt: `x3` requires stmt expose default type
 * 2) "eoi"   stmt: generated at end-of-input
 *
 * Also support the `print` module.
 * Provide indirection to `print_stmt` as required`
 */


namespace kas::parser
{
namespace detail
{
    using namespace meta;
    using boost::mpl::string;

    // vector of types in variant
    template <typename tag = void> struct parser_type_l : list<> {};

    // vector of rules for statments 
    template <typename tag = void> struct parser_stmt_l : list<> {};

    // vector of rules for label
    template <typename tag = void> struct parser_label_l : list<> {};

    // declare empty strings for comment and stmt seperator default
    template <typename = void> struct stmt_separator_str : string<> {};
    template <typename = void> struct stmt_comment_str   : string<> {};

    // declare default "Parsers" for comment & stmt seperator
    // NB can't default "type", because that would instantiate
    // stmt_*_str templates
    template <typename INSN_SEPARATOR, typename = void>
    struct stmt_separator_p : literal_parser<INSN_SEPARATOR> {};

    template <typename INSN_COMMENT,   typename = void>
    struct stmt_comment_p   : literal_parser<INSN_COMMENT>   {};
}

using namespace ::kas::core::opc;

namespace print
{
    template <typename OS, typename...Ts>
    void print_stmt(OS&, Ts&&...);
}


using print_obj = print::stmt_print<std::ostream>;

template <typename Derived>
struct parser_stmt : kas_position_tagged
{
    using base_t    = parser_stmt<Derived>;
    using derived_t = Derived;
    using print_obj = print::stmt_print<std::ostream>;
    using opcode    = core::opcode;

    // inherit base class operators
    using kas_position_tagged::kas_position_tagged;
    using kas_position_tagged::operator=;

    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

    // primary entrypoints:
    const char *name() const
    {
        return "STMT";
    }

    void  print_args(print_obj const&) const
    {
        // no args
    }

    // XXX change to unimp?
    opcode *gen_insn(opcode::data_t& data)
    {
        return derived().gen_insn(data);
    }
};

////////////////////////////////////////////////////////////////////////
//
// statments generated by parser
//
////////////////////////////////////////////////////////////////////////

namespace detail
{
    template <typename OPC>
    struct stmt_diag : parser_stmt<stmt_diag<OPC>>
    {
        // w/o braces, clang drops core on name(). 2019/02/15 KBH
        static inline OPC opc{};

        stmt_diag(kas_error_t diag = {}) : diag(diag) {}

        const char *name() const
        {
            return opc.name();    // clang drops core w/o opc braces
        }

        void print_args(print_obj const& p_obj) const
        {
            if (diag)
                p_obj(diag);
        }

        opcode *gen_insn(core::insn_data& data)
        {
            // fixed area unused otherwise...
            data.fixed.diag = diag;
            return &opc;
        }
        
        kas_error_t diag;
    };
}

using stmt_empty = detail::stmt_diag<opc_nop<>>;
using stmt_eoi   = detail::stmt_diag<opc_nop<KAS_STRING("EOI")>>;
using stmt_error = detail::stmt_diag<opc_error>;


namespace detail
{
    // stmts defined by parser
    template<> struct parser_type_l<defn_parser> : list<
          stmt_empty        // default value for variant
        , stmt_eoi
        , stmt_error
        > {};

    // don't allow label_l to be empty
    template<> struct parser_label_l<defn_parser> : list<
          stmt_eoi
        > {};
}

}


#endif
