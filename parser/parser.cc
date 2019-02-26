#include "parser.h"
#include "parser_def.h"
//#include "stmt_print.h"
//#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{
    BOOST_SPIRIT_INSTANTIATE(stmt_x3, iterator_type, context_type)

std::string kas_loc::where() const
{
    auto w = error_handler_type::where(loc);
    return { w.second.begin(), w.second.end() };
}

#if 0

template <typename Iter>
kas_position_tagged_t<Iter>::operator kas_loc&() const
{
    if (!handler)
        throw std::runtime_error("kas_position_tagged::kas_loc: zero handler");
    if (!loc)
        loc = handler->get_loc(*this);
    return loc;
}

template kas_position_tagged_t<iterator_type>::operator kas_loc&() const;
#endif

template <typename OS>
OS& operator<<(OS& os, kas_token const& tok)
{
    return os << "[token: src=\"" << std::string(tok) << "\"]";
}

template std::ostream& operator<<(std::ostream&, kas_token const&);

//template std::ostream& operator<<(std::ostream&, stmt_t&);
//template opcode& stmt_t::operator()(opcode::Inserter&, opcode::fixed_t&, opcode::op_size_t&);

}

namespace kas::parser
{
struct _xxx
{
    _xxx()
    {
        print_type_name{"stmt_parsers"}.name<stmt_parsers>();

        print_type_name{"stmt<cpu>"}.name<typename detail::parser_type_l<defn_cpu>::type>();
        print_type_name{"stmt<fmt>"}.name<typename detail::parser_type_l<defn_fmt>::type>();

        print_type_name{"stmt_tuple_t"}.name<XXX_stmt_tuple_t>();
        print_type_name{"stmt_tuple"}(XXX_stmt_tuple);
    }
} _x;

}
