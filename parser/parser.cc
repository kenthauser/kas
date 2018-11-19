#include "parser.h"
#include "parser_def.h"
#include "stmt_print.h"
#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{
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

#if 1
    template <typename OS>
    OS& operator<<(OS& os, stmt_t& stmt)
    {
        auto   fn = print::stmt_print(os, stmt.vptr->name());
        stmt.vptr->print_args(fn);
        return os;
    }
#endif
    template <typename...Ts>
    opcode& stmt_t::operator()(Ts&&...args)
    {
        return vptr->gen_insn(std::forward<Ts>(args)...);
    }

    template std::ostream& operator<<(std::ostream&, stmt_t&);
    template opcode& stmt_t::operator()(opcode::Inserter&, opcode::fixed_t&, opcode::op_size_t&);
}

#if 0
namespace kas::parser::ast
{
    // XXX explicit instantiation should go into test runner...
    // NB: stmt_t is not const because it uses `get_args`
    std::ostream& operator<< (std::ostream& os, stmt_t& ast) {
        auto fn = [&](auto& node) { print_object(os, node); };
        auto X_fn = [&](auto& node)
            { 
                auto& obj = node.get();
        
        //std::cout << "X_parser_stmt::get: type = ";
        //std::cout << boost::typeindex::type_id_runtime(obj).pretty_name();
        //std::cout << std::endl;
                auto fn = print::stmt_print(os, obj.name());
                obj.print_args(fn);
            };
        ast.apply_visitor(x3::make_lambda_visitor<void>(X_fn));
        return os << std::endl;
    }

}
#endif

#if 0
namespace {
    using namespace kas::parser;
    
    struct xxx {
    
        xxx()
        {
            print_type_name("label_parsers").name<label_parsers>();
            print_type_name("label_tuple_v").name<decltype(label_tuple_v)>();
            print_type_name("label_tuple_t").name<label_tuple_t>();

            label_tuple_t tpl;
            auto t_0 = std::get<0>(tpl);
            print_type_name("label_tuple_t[0]").name<decltype(t_0)>();

            const auto v_0 = t_0()();

            print_type_name("label_tuple_t[0]()").name<decltype(v_0)>();

        }

    } _x;
}
#endif
