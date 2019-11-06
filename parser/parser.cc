#include "parser.h"
#include "parser_config.h"
#include "error_handler.h"
//#include "parser_def.h"
//#include "stmt_print.h"
//#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{
 // XXX
 //   BOOST_SPIRIT_INSTANTIATE(stmt_x3, iterator_type, stmt_context_type)

std::string kas_loc::where() const
{
    auto w = error_handler_type::raw_where(loc);
    return { w.second.begin(), w.second.end() };
}

#if 0
template <typename REF>
template <typename OS>
void kas_diag<REF>::print(OS& os) const
{
    auto where = this->loc().where();
    os << level_msg() << message << " : " << escaped_str(where);
}

template void kas_diag<kas_error_t>::print<std::ostream&>(std::ostream&) const;
template void kas_diag<kas_error_t>::print<std::ostringstream&>(std::ostringstream&) const;
#endif

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

void kas_token::print(std::ostream& os) const
{
    os << "[" << name();
    os << ": src=\"" << std::string(*this) << "\"";
    if (!_expr.empty())
        os << ", expr=" << _expr;
    os << "]";
}

template std::ostream& operator<<(std::ostream&, kas_token const&);

//template std::ostream& operator<<(std::ostream&, stmt_t&);
//template opcode& stmt_t::operator()(opcode::Inserter&, opcode::fixed_t&, opcode::op_size_t&);

}
#if 0
namespace kas::parser
{
struct _xxx
{
    _xxx()
    {
        print_type_name{"parser_type_l<cpu>"}.name<typename detail::parser_type_l<defn_cpu>::type>();
        print_type_name{"parser_type_l<fmt>"}.name<typename detail::parser_type_l<defn_fmt>::type>();
        std::cout << std::endl;

        print_type_name{"label_parsers"}.name<label_parsers>();
        std::cout << std::endl;
        print_type_name{"stmt_parsers" }.name<stmt_parsers>();
        std::cout << std::endl;
        
        //print_type_name{"label_tuple_t"}.name<XXX_stmt_tuple_t>();
        print_type_name{"label_tuple"}(label_tuple);
        std::cout << std::endl;

        print_type_name{"stmt_tuple_t"}.name<XXX_stmt_tuple_t>();
        print_type_name{"stmt_tuple"}(XXX_stmt_tuple);
    }
} _x;
}
#endif
