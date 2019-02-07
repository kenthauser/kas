#include "parser.h"
#include "parser_def.h"
//#include "stmt_print.h"
//#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{
#if 0
BOOST_SPIRIT_INSTANTIATE(
    parser_type, iterator_type, context_type)

std::string kas_loc::where() const
{
    auto w = error_handler_type::where(loc);
    return { w.second.begin(), w.second.end() };
}


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

template <typename OS>
OS& operator<<(OS& os, kas_token const& tok)
{
    return os << "[token: src=\"" << std::string(tok) << "\"]";
}

template std::ostream& operator<<(std::ostream&, kas_token const&);
#endif

#if 0
template <typename OS>
OS& operator<<(OS& os, stmt_t& stmt)
{
    auto   fn = print::stmt_print(os, stmt.vptr->name());
    stmt.vptr->print_args(fn);
    return os;
}

template <typename...Ts>
opcode& stmt_t::operator()(Ts&&...args)
{
    return vptr->gen_insn(std::forward<Ts>(args)...);
}
#endif

//template std::ostream& operator<<(std::ostream&, stmt_t&);
//template opcode& stmt_t::operator()(opcode::Inserter&, opcode::fixed_t&, opcode::op_size_t&);
}
