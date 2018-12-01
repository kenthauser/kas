//
#include "parser/parser.h"
#include "parser/error_handler_base.h"
#include "parser/parser_obj.h"

// #include "kas_core/core_data_insn.h"
#include "utility/print_type_name.h"
#include "kas_core/core_insn.h"
#include "kas_core/insn_container.h"
#include "kas_core/emit_string.h"
#include "kas_core/core_fits.h"
#include "kas_core/core_relax.h"
#include "dwarf/dwarf_impl.h"

#include "kas_core/emit_listing.h"

#include "kas_core/assemble.h"

#include "elf/elf_emit.h"


#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <functional>
#include <boost/spirit/home/x3/support/utility/testing.hpp>

namespace fs = boost::filesystem;
namespace testing = boost::spirit::x3::testing;

template <typename T>
std::string escaped_str(T&& in)
{
    std::string output;

    for (auto& c : in) {
        switch (c) {
            case L'\t':
                output.append("[\\t]");
                break;
            case L'\n':
                output.append("[\\n]");
                break;
            default:
                output.push_back(c);
                break;
        }
    }
    return output;
}


auto sub_path_ext(std::string const& input, const std::string& new_ext)
{
    auto s(input);
    auto i = s.rfind('.', s.length());

    if (i != std::string::npos)
        s.replace(i+1, std::string::npos, new_ext);
    return s;
}

auto parse = [](std::string const& source, fs::path input_path) -> std::string
{
    std::stringstream out;

    using kas::parser::iterator_type;
    using kas::parser::error_handler_type;
    iterator_type iter{std::begin(source)};
    iterator_type const end{std::end(source)};

    // std::stringstream parse_out;
    auto& parse_out = out;
    //auto& parse_out = std::cout;

    std::cout  << "\nparse begins: " << input_path << std::endl;
    kas::core::kas_assemble obj;

    obj.assemble(source.begin(), source.end(), &parse_out);
    //obj.assemble(source.begin(), source.end());

    // dump raw
    auto dump_raw = [&parse_out](auto& container)
        {
            container.proc_all_frags(
                [&parse_out](auto& insn_iter, auto& dot)
                {
                    auto& insn = *insn_iter;
#if 0 
                    // unpack location into file_num/first/last
                    auto where = parser::error_handler<Iter>::where(loc);
                    auto idx   = where.first;
                    auto first = where.second.begin();
                    auto last  = where.second.end();
#endif
                    //auto where = stmt_stream.where(stmt).second;
                    auto& loc = insn.loc;
                    //auto where = kas::parser::error_handler<Iter>::where(loc);
                    //parse_out << "in  : " << escaped_str(where) << std::endl;
                    parse_out << "raw : " << insn.print_raw() << std::endl;
                    parse_out << "fmt : " << insn.print_fmt() << '\n' << std::endl;
                });
        };

    kas::core::kas_assemble::INSNS::for_each(dump_raw);

#if 0    
    // dump tables
    kas::core::core_symbol::dump(out);
    kas::core::core_section::dump(out);
    kas::core::core_segment::dump(out);
    kas::core::core_fragment::dump(out);
    kas::dwarf::dl_data::dump(out);
#endif
#if 1
    kas::core::emit_listing<iterator_type> listing(parse_out);
    obj.emit(listing);
#endif
#if 1
    kas::elf::elf_emit elf_obj(ELFCLASS32, ELFDATA2MSB, EM_68K); 
    obj.emit(elf_obj);

    auto elf_path = sub_path_ext(input_path.c_str(), "elf");
    std::ofstream elf_out(elf_path, std::ios_base::binary);
    elf_obj.write(elf_out);
#endif
#if 0
    kas::core::core_symbol::dump(out);
    kas::core::core_section::dump(out);
    kas::core::core_segment::dump(out);
    kas::core::core_fragment::dump(out);
    kas::dwarf::dl_data::dump(out);
#endif
    //kas::core::kas_assemble::INSNS::for_each(dump_raw);

    // prepare for next round
    kas::core::kas_clear::clear();

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

#include "utility/print_type_name.h"

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
