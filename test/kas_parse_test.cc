
#include "parser/parser.h"
#include "parser/error_handler_base.h"

#include "parser/parser_obj.h"
#include "bsd/bsd_symbol.h"        // for clear()

#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <boost/spirit/home/x3/support/utility/testing.hpp>

namespace fs = boost::filesystem;
namespace testing = boost::spirit::x3::testing;

template <typename T>
auto escaped_str(T&& where)
{
	std::string output;

	for (auto& c : where) {
		switch (c) {
			case '\t':
				output.append("[\\t]");
				break;
			case '\n':
				output.append("[\\n]");
				break;
			default:
				output.push_back(c);
				break;
		}
	}
	return output;
}


auto parse = [](std::string const& source, fs::path input_path)-> std::string
{
    std::stringstream out;

    using kas::parser::iterator_type;
    using kas::parser::error_handler_type;
    iterator_type iter{std::begin(source)};
    iterator_type const end{std::end(source)};
        
    kas::core::kas_clear::clear();

    std::cout << "parsing: " << input_path.c_str() << '\n' << std::endl;

    // create parser object
    auto stmt_stream = kas::parser::kas_parser(kas::stmt() , std::cout);
    stmt_stream.add(source.begin(), source.end(), input_path.c_str());


    for (auto&& stmt : stmt_stream) {
    	auto where = stmt_stream.where(stmt).second;
        // `stmt` "moves" args. Can not "ostream" twice
    	//std::cout << "1 in :  " << escaped_str(where) << std::endl;
    	//std::cout << "1 out:  " << stmt << '\n' << std::endl;
         
    	out << "in :  " << escaped_str(where) << std::endl;
    	out << "out:  " << stmt << '\n' << std::endl;
    }
 	std::stringstream symtab;
	kas::core::core_symbol::dump(symtab);
	out << symtab.str();

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
