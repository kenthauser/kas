#ifndef KAS_PARSER_PARSER_STMT_H
#define KAS_PARSER_PARSER_STMT_H

////////////////////////////////////////////////////////////////////////////
//
// parser_stmt.h: define object resulting from parsing line of assembly code
//
////////////////////////////////////////////////////////////////////////////
//
// The source code is parsed into "parser_stmt" objects as the first step
// in assembling the code. Several "parser_stmt" objects
// are defined to handler the various requirements of different assembler 
// statements. Examples are:
//
//  1. label definitions require only a "symbol" reference
//  2. "equ" definitions require both a "symbol" and a "value" references
//  3. "psuedo" definitions require a "opcode" and a list of "arguments"
//  4. "machine code" definitions vary depending on architecture
//
// The "parser_stmt" is a transient object: once a "parser_stmt" object
// is created, it is immediately translated to a "core_insn" object and
// added to the "insn_container". 
//
// Since only a single "parser_stmt" object is active at any point, a significant
// simplification is possible: instead of the CRTP + variant pattern which would be 
// required if multiple "parser_stmt" objects were active at one time, a simple
// virtual object pattern can be used with a pointer to a single static object
// of each derived type. 
// 
//
////////////////////////////////////////////////////////////////////////////

#include "expr/expr.h"
#include "kas_loc.h"
#include "kas_token.h"
#include "stmt_print.h"
#include "kas_core/opcode.h"

#include <boost/mpl/string.hpp>
#include <functional>

namespace kas::parser
{
namespace detail
{
    using namespace meta;

    // vector of types in variant
    template <typename tag = void> struct parser_type_l : list<> {};

    // vector of rules for statments 
    template <typename tag = void> struct parser_stmt_l : list<> {};

    // vector of rules for label
    template <typename tag = void> struct parser_label_l : list<> {};

    // declare empty strings for comment and stmt seperator default
    template <typename = void> struct stmt_separator_str : kas_string<> {};
    template <typename = void> struct stmt_comment_str   : kas_string<> {};
}

using namespace ::kas::core::opc;

namespace print
{
    template <typename OS, typename...Ts>
    void print_stmt(OS&, Ts&&...);
}

// forward declare "annotate_on_success".
struct annotate_on_success;

using print_obj = print::stmt_print<std::ostream>;

// ABC for parser statements
struct parser_stmt 
{
    using print_obj = print::stmt_print<std::ostream>;
    using opcode    = core::opcode;

    parser_stmt() = default;
    // XXX obsolete
    parser_stmt(const kas_position_tagged& loc) {}

    // primary entrypoints:
    virtual std::string name() const
    {
        return "STMT";
    }

    virtual void  print_args(print_obj const&) const
    {
        // no args
    }

    // NB: must be implemented in `Derived`
    virtual opcode *gen_insn(opcode::data_t& data) = 0;
};

////////////////////////////////////////////////////////////////////////
//
// statments generated by parser:
//
// 1. `nop`  stmt (nop or eoi)
// 2. `diag` stmt (diagnostic)
//
////////////////////////////////////////////////////////////////////////

namespace detail
{
    template <typename OPC>
    struct stmt_nop : parser_stmt
    {
        using base_t = parser_stmt;
        
        // w/o braces, clang drops core on name(). 2019/02/15 KBH
        static inline OPC opc{};

        // inherit default ctor
        using base_t::base_t;
        
        std::string name() const override
        {
            return opc.name();
        }

        base_t *operator()() 
        {
            static stmt_nop stmt;
            return &stmt;
        }

        opcode *gen_insn(core::opcode_data& data) override
        {
            return &opc;
        }
    };
    
    struct stmt_diag : parser_stmt
    {
        using base_t = parser_stmt;
        
        // w/o braces, clang drops core on name(). 2019/02/15 KBH
        static inline opc_error opc{};
    
        using base_t::base_t;
       
        // XXX clean up: don't need 3 ctors...
        stmt_diag(kas_error_t diag)
            : diag(diag), base_t(diag.get_loc()) {}
        
        stmt_diag(kas_diag_t const& err) : stmt_diag(err.ref()) {}
       
        stmt_diag(kas_token const& token)
        {
            diag = kas_diag_t::error("Invalid instruction", token).ref();
        }
        
        std::string name() const override
        {
            return opc.name();
        }

        void print_args(typename base_t::print_obj const& p_obj) const override
        {
            p_obj(diag);
        }

        base_t *operator()() 
        {
            static stmt_diag stmt;
            return &stmt;
        }


        opcode *gen_insn(core::opcode_data& data) override
        {
            // fixed area unused otherwise...
            data.fixed.diag = diag;
            return &opc;
        }
        
        kas_error_t diag;
    };
}

using stmt_empty = detail::stmt_nop<opc_nop<>>;
using stmt_eoi   = detail::stmt_nop<opc_nop<KAS_STRING("EOI")>>;
using stmt_error = detail::stmt_diag;


namespace detail
{
    // stmts defined by parser
    template<> struct parser_type_l<defn_parser> : list<
          stmt_empty        // default value for variant
        , stmt_eoi          // end-of-input
        , stmt_error        // undefined instruction
        > {};
}

}


#endif
