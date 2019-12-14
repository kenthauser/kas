// bsd/bsd.cc: instantiate parser elements

#include "bsd_parser_def.h"
#include "bsd_arg_impl.h"



namespace kas::bsd
{
    namespace parser
    {
        using namespace x3;
        using kas::parser::iterator_type;
        using kas::parser::stmt_context_type;

        // expression parsers
        BOOST_SPIRIT_INSTANTIATE(dot_parser_x3, iterator_type, expr_context_type)
        BOOST_SPIRIT_INSTANTIATE(sym_parser_x3, iterator_type, expr_context_type)
        
        // stmt parsers
        BOOST_SPIRIT_INSTANTIATE(stmt_comma_x3, iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_space_x3, iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_equ_x3  , iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_org_x3  , iterator_type, stmt_context_type)
        //BOOST_SPIRIT_INSTANTIATE(stmt_label_x3, iterator_type, stmt_context_type)

    }
#if 0
    // define bsd_arg printer
    std::ostream& operator<<(std::ostream& os, bsd_arg const& arg)
    {
        if (arg.token_idx == 0)
            os << "[EXPR = " << arg.expr;
        else
            os << "[" << bsd_token_names[arg.token_idx - 1];
            
        return os << ": src = \"" << std::string(arg.begin(), arg.end()) << "\"]";
    }
#endif

struct _xxx
{
    _xxx()
    {
        print_type_name{"bsd: int_"}(x3::int_);
        print_type_name{"bsd: uint_"}.name<decltype(x3::uint_)>();

        print_type_name{"bsd: at_ident"}(bsd::parser::at_ident);
        print_type_name{"bsd: at_num"}  (bsd::parser::at_num);
        print_type_name{"bsd: missing"} (bsd::parser::missing);

        auto const at_token_initial = x3::omit[x3::char_("@%#")];
        auto const at_ident         = (at_token_initial >> !x3::digit) > +parser::bsd_charset;
        auto const at_num           = (at_token_initial >>  x3::uint_) > !parser::bsd_charset;
        print_type_name{"bsd: at_token_initial"}(at_token_initial);
        print_type_name{"bsd: at_ident: parser"}(at_ident);
        print_type_name{"bsd: at_num:   parser"}(at_num);

        std::cout << std::endl;
        auto const at_initial_p = x3::as_parser(at_token_initial);
        auto const at_ident_p   = x3::as_parser(at_ident);
        auto const at_num_p     = x3::as_parser(at_num);

        auto const int_p = as_parser(x3::int_);
        print_type_name{"bsd: int_: as_parser"}(int_p);
        print_type_name{"bsd: int_: attribute_type"}.name<typename decltype(int_p)::attribute_type>();
        
        std::cout << std::endl;
        //print_type_name{"bsd: int_: value_type" }.name<typename decltype(int_p)::value_type>();
        print_type_name{"bsd: at_token_initial: type"}(at_initial_p);
        print_type_name{"bsd: at_token_initial: attribute_type"}.name<typename decltype(at_initial_p)::attribute_type>();
        
        std::cout << std::endl;
        print_type_name{"bsd: at_num: raw   "}(at_num);
        print_type_name{"bsd: at_num: parser"}(at_num_p);
        std::cout << std::endl;
        print_type_name{"bsd: at_num: left_type"}.name<typename decltype(at_num_p)::left_type>();
        std::cout << std::endl;
        print_type_name{"bsd: at_num: right_type"}.name<typename decltype(at_num_p)::right_type>();
        
        std::cout << std::endl;
        //print_type_name{"bsd: at_num: value_type"}.name<typename decltype(at_num_p)::value_type>();

       // print_type_name{"bsd: at_num:   type"}(at_num_p);
       // print_type_name{"bsd: at_num: attribute_type"}.name<typename decltype(at_num_p)::attribute_type>();
        using Iter    = parser::iterator_type;
        using Context = parser::stmt_context_type;

        using bsd_x = struct { int why; };

    #if 0
        print_type_name{"bsd: at_token_initial: type"}.name<at_initial_t>();
        print_type_name{"bsd: at_ident: type"}.name<at_ident_t>();
        print_type_name{"bsd: at_num:   type"}.name<at_num_t>();
    #endif

    }

} 
//_xxx
;
}
