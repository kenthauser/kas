#ifndef KAS_EXPR_EXPR_PRINT_H
#define KAS_EXPR_EXPR_PRINT_H

#include "expr/expr.h"
#include "utility/print_object.h"

namespace kas::expression::print
{
    // XXX probably need to re-write as specialized struct...
    template <typename T, typename OS>
    void print_expr(T const& value, OS& os)
    {
        print_object(os, value);
    }

    // default error type
    // XXX
    template <typename OS>
    void print_expr(kas::parser::kas_error_t const& err, OS& os) {
        // os << "Err: " << kas_diag::get(err.err_index).message;
        os << "Err: XXX"  << std::endl;
    }

    // print strings inside quotations
    template <typename OS>
    void print_expr(e_string_t const& text, OS& os) {
        os << '"' << text << '"';
    }

    // print native floating point types with decimal point
    template <typename OS>
    void print_expr(e_float_t const& f, OS& os) {
        os << std::showpoint << f;
    }

    // print native floating point types with decimal point
    template <typename OS>
    void print_expr(std::string const& s, OS& os) {
        os << "XXX \"" << s << '"';
    }

    // unwrap reference wrappers
    template <typename T, typename OS>
    inline void print_expr(std::reference_wrapper<T> const& t, OS& os)
    {
        print_expr(t.get(), os);
    }
}

#endif
