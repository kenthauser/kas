#ifndef PARSER_ERROR_HANDLER_H
#define PARSER_ERROR_HANDLER_H

#include "expr/expr.h"
#include "kas_position.h"

#include "error_handler.h"

#include <boost/spirit/home/x3/directive/expect.hpp>
#include <boost/spirit/home/x3/core/action.hpp>
#include <boost/spirit/home/x3/core/call.hpp>

#include <map>


namespace kas::parser
{
namespace x3 = boost::spirit::x3;

////////////////////////////////////////////////////////////////////////////
//  Our error handler
////////////////////////////////////////////////////////////////////////////
// X3 Error Handler Utility
// template <typename Iterator>
// using error_handler = x3::error_handler<Iterator>;

// tag used to get our error handler from the context
//using error_handler_tag = x3::error_handler_tag;

struct error_handler_base
{
    error_handler_base();

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(
        Iterator& first, Iterator const& last
      , Exception const& x, Context const& context);

    std::map<std::string, std::string> id_map;
};

////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////

inline error_handler_base::error_handler_base()
{
    id_map["expr"] = "Expression";
    id_map["expr_value"] = "Value";
    id_map["expr_key_value"] = "Key value pair";

    id_map["insn_eol"] = "End-Of-Line";
    id_map["statement"] = "Opcode or Label";

    id_map["dwarf_arg"] = "Expression";
}

template <typename Iterator, typename Exception, typename Context>
inline x3::error_handler_result
error_handler_base::on_error(
    Iterator& first, Iterator const& last
  , Exception const& exc, Context const& context)
{
    std::string which = exc.which();
    auto iter = id_map.find(which);
    if (iter != id_map.end())
        which = iter->second;

    std::string message = "Expecting: " + which;
    auto& error_handler = x3::get<error_handler_tag>(context).get();
    // XXX where is Iterator (x3/directive/expect.hpp)
    // error_handler(x.where(), message);
    kas_position_tagged loc;
    auto& err_first = exc.where();
    auto  err_last  = err_first;
     
  
    // here for expectation error. Either found "nothing" or wrong thing
    // if `is_blank`, then nothing. Set "first/last" as here.
    // if character, then wrong token. Skip to blank.
    if (!std::isblank(*err_first)) {
        for ( ; err_last != last; ++err_last)
            if (std::isblank(*err_last))
                break;
    }


    std::cout << "on_error: first, exc : " << std::string(err_first, err_last) << std::endl;
    std::cout << "on_error: msg = " << message << std::endl;

    error_handler.tag(loc, err_first, err_last);
    auto& diag = parser::kas_diag::error(message, loc);
    //std::cout << "on_error: diag = " << diag.message << std::endl;
    return x3::error_handler_result::fail;
}
}

#endif
