#include "parser.h"
#include "error_handler.h"
#include "parser_context.h"
#include "kas_loc_impl.h"

#include "kas_token_impl.h"

#include "parser_def.h"
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{
    BOOST_SPIRIT_INSTANTIATE(stmt_x3, iterator_type, stmt_context_type)



    void kas_token::print(std::ostream& os) const
    {
        os << "[" << name();
        if (static_cast<kas_loc>(*this))
            os << " src=" << this->where();
        if (_fixed)
            os << " fixed=" << +_fixed;
        if (!_expr.empty())
            os << " expr=" << expr();
        os << "]";
    }

    template std::ostream& operator<<(std::ostream&, kas_token const&);
}

namespace kas::core
{
    // instiantiate `kas_error_t` diagnostic printer
    template void parser::kas_error_t::print(std::ostream&) const;
}

using namespace kas::parser::detail;
using namespace meta;
struct xxx
{
    xxx()
    {
        std::cout << "fmt_index = " << kas::parser::fmt_index() << std::endl;


        using fmt_name = _t<fmt_defn_name<>>;
        using name_0 = at_c<fmt_defn_names_l, 0>;

        std::cout << "fmt_defn_name: " << fmt_name() << std::endl;
    
        // get "fmt" index
        using fmt_index = find_index<fmt_defn_names_l, fmt_name>;
        std::cout << "fmt_defn_name_index: " << +fmt_index() << std::endl;

        print_type_name("fmt_name").name<fmt_name>();
        print_type_name("name_0  ").name<name_0  >();

        using is_same = std::is_same<fmt_name, name_0>;
        std::cout << "is_same(bsd, name_0): " << is_same() << std::endl;

        print_type_name("parser_comment<>").name<parser_comment<>>();
        print_type_name("parser_comment<>::type").name<_t<parser_comment<>>>();
        //using comment = at<parser_comment<>, fmt_index>;
        //print_type_name("comment").name<comment>();

    }
} _xxx;

