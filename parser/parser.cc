#include "parser.h"
#include "error_handler.h"
#include "parser_def.h"
#include "parser_context.h"
#include "kas_loc_impl.h"

#include "kas_token_impl.h"

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
