// expr/expr.cc: instantiate expression parser.

#include "expr.h"
#include "expr_fits.h"
#include "expr_parser_def.h"
#include "expr_print_impl.h"
#include "expr_op_impl.h"

namespace kas::expression::parser
{
    using kas::parser::iterator_type;
    using kas::parser::expr_context_type;

    // instantiate expr parser with standard context
    BOOST_SPIRIT_INSTANTIATE(
        expr_type, iterator_type, expr_context_type)

}

namespace kas::expression::ast
{
    std::ostream& operator<< (std::ostream& os, expr_t const& ast)
    {
        ast.apply_visitor(x3::make_lambda_visitor<void>([&](auto& node)
             { print::print_expr(node, os); }));
        return os;
    }
}


// define preprocessor symbol to get variant info
#ifdef PRINT_EXPR_INFO

//
// print info about the expr_t type
// make a nice "list<>" & print types.
// iter the list & find largest type.
//

#include "utility/print_type_name.h"

namespace {
    using namespace kas::expression;
    using namespace kas::expression::ast::detail;

    struct xxx {
        struct largest_type
        {
            template <typename T1, typename T2>
            using invoke = if_<less<sizeof_<T1>, sizeof_<T2>>
                              , T2, T1>;
        };

        xxx()
        {
            using largest = fold<variant_types, nil_, largest_type>;

            print_type_name{"expr_t types"}.name<variant_types>();
            print_type_name{"ref_wrapped_types"}.name<ref_wrapped_types>();
            std::cout << "total types: " << variant_types::size();
            std::cout << ", largest type ("  << sizeof_<largest>::value;
            print_type_name{")"}.name<largest>();
            std::cout << "sizeof(expr_t) = " << sizeof(expr_t) << std::endl;


            flt_info<float>      ("float      ");
            flt_info<double>     ("double     ");
            flt_info<long double>("long double");
        }

        template <typename T>
        void flt_info(const char *name) const
        {
            auto exp_digits = []
                {
                    auto n  = std::numeric_limits<T>::max_exponent;
                         n -= std::numeric_limits<T>::min_exponent;

                    auto i = 1;
                    while (n >>= 1)
                        i++;

                    return i;
                };

            std::cout << name << ":";
            std::cout << " ieee754 = " << std::boolalpha << std::numeric_limits<T>::is_iec559;
            std::cout << " sizeof = " << sizeof(T);
            std::cout << " mantissa bits = " << std::numeric_limits<T>::digits;
            std::cout << " exponent bits = " << exp_digits();
            std::cout << " max_exp10 = " << std::numeric_limits<T>::max_exponent10;
            std::cout << std::endl;
        }

    } _x;
}
#endif
