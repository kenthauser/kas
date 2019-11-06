#include "parser/parser.h"
#include "parser/error_handler_base.h"
//#include "parser/parser_type.h"

#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <boost/spirit/home/x3/support/utility/testing.hpp>


#include "utility/print_type_name.h"

namespace fs = boost::filesystem;
namespace testing = boost::spirit::x3::testing;

auto parse = [](std::string const& source, fs::path input_path)-> std::string
{
    std::stringstream out;

    using kas::parser::iterator_type;
    iterator_type iter(source.begin());
    iterator_type const end(source.end());

    // Our AST
    kas::expression::ast::expr_t ast;

    // Our error handler
    using boost::spirit::x3::with;
    using kas::parser::error_handler_type;
    using kas::parser::error_handler_tag;
    using kas::parser::error_diag_tag;
    using kas::parser::kas_error_t;
    error_handler_type error_handler(iter, end, input_path.c_str());
    kas_error_t diag;

    // Our parser
    auto const parser =
        // we pass our error handler to the parser so we can access
        // it later on in our on_error and on_sucess handlers
        with<error_handler_tag>(std::ref(error_handler))
        [
            with<error_diag_tag>(std::ref(diag))
            [
                kas::expr()
            ]
        ];

    // Go forth and parse!
    std::cout << "testing: " << input_path << std::endl;
    using boost::spirit::x3::ascii::blank;
    bool success = phrase_parse(iter, end, parser, blank, ast);

    if (success)
    {
        if (iter != end)
            error_handler(out, iter, "Error! Expecting end of input here:");
        else
            out << ast;
    }

    // std::cout << "Output: " << out.str() << std::endl;

    return out.str();
};

int num_files_tested = 0;
auto compare = [](fs::path input_path, fs::path expect_path)
{
   testing::compare(input_path, expect_path, parse);
   ++num_files_tested;
};

auto overwrite = [](fs::path input_path, fs::path expect_path)
{
    std::string output = parse(testing::load(input_path), input_path);
    boost::filesystem::ofstream file(expect_path);
    file << output;

    ++num_files_tested;
};

int main(int argc, char* argv[])
{
    bool do_overwrite = false;
    if (argc && !std::strcmp(argv[1], "--overwrite")) {
        argc--;
        argv++;
        do_overwrite = true;
    }

    if (argc < 2)
    {
       std::cout << "usage: " << fs::path(argv[0]).filename() << " path/to/test/files" << std::endl;
       return -1;
    }


    std::cout << std::unitbuf;

    std::cout << "===================================================================================================" << std::endl;
    std::cout << "Testing: " << fs::absolute(fs::path(argv[1])) << std::endl;
    int r = testing::for_each_file(fs::path(argv[1]),
                            do_overwrite ? overwrite : compare);
    if (r == 0)
        std::cout << num_files_tested << " files tested." << std::endl;
    std::cout << "===================================================================================================" << std::endl;
    return r;
}
