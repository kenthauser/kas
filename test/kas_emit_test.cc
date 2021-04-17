//
#include "parser/parser.h"
#include "parser/error_handler_base.h"
#include "parser/parser_obj.h"

#include "utility/print_type_name.h"
#include "kas_core/core_insn.h"
#include "kas_core/insn_container.h"

#include "kas_core/core_fits.h"
#include "kas_core/core_relax.h"
#include "dwarf/dwarf_impl.h"

#include "kas_core/emit_kbfd.h"
#include "kas_core/emit_listing.h"

#include "kas_core/assemble.h"
#include "machine_out.h"

#include "kbfd/kbfd.h"
#include "kbfd/kbfd_format_elf_ostream.h"     // ostream host format

#include "kas_core/opc_segment.h"



#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <functional>
//#include <boost/spirit/home/x3/support/utility/testing.hpp>
#include "testing.hpp"

namespace fs = boost::filesystem;
namespace testing = boost::spirit::x3::testing;

auto sub_path_ext(std::string s, const std::string& new_ext)
{
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
    //auto& parse_out = out;
    auto& parse_out = std::cout;

    std::cout  << "\nparse begins: " << input_path << std::endl;

    // create source object
    kas::parser::parser_src src;
    src.push(source.begin(), source.end(), input_path.c_str());
    
    // need object format before assembling
    auto& obj_fmt  = *kbfd::get_obj_format(KAS_KBFD_TARGET());
    kbfd::kbfd_object kbfd_obj(obj_fmt);
    
    // create assembler object & assemble source
    kas::core::kas_assemble obj(kbfd_obj);
    obj.assemble(src, &parse_out);
    //obj.assemble(src);
    
    // dump raw
    auto dump_container = [&parse_out](auto& container)
        {
            container.proc_all_frags(
                [&parse_out](auto& insn_iter, auto& dot)
                {
                    kas::core::core_insn insn = *insn_iter;
                    kas::parser::parser_src src;
                    
                    auto& loc = insn.loc();
                    parse_out << "loc : " << loc.get() << std::endl;
                    if (loc)
                    {
                        auto where = loc.where();
                        parse_out << "in  : " << where << std::endl;
                    }
                    parse_out << "raw : " << insn.raw() << std::endl;
                    parse_out << "fmt : " << insn.fmt() << '\n' << std::endl;
                });
        };

    // dump 
    auto dump_raw = [&parse_out](auto& container)
        {
            parse_out << "new container: insn count = ";
            parse_out << std::dec << container.insns.size() << std::endl;
            unsigned n{};
            std::remove_reference_t<decltype(container)>::value_type::reinit();
            for (auto&& raw : container.insns)
            {
                parse_out << std::setw(4) << ++n;
                parse_out << "data: ";
                parse_out << " opc = "   << raw.opc_index();
                parse_out << " index = " << raw.index();
                parse_out << " cnt = "   << raw.cnt();
                parse_out << " size = "  << raw.size();
                parse_out << " loc = "   << raw.loc().get();
                parse_out << " fixed = " << raw.fixed.fixed;
                parse_out << std::endl;

                raw.advance(raw);
            };
        };

//    kas::core::kas_assemble::INSNS::for_each(dump_raw);
//    kas::core::kas_assemble::INSNS::for_each(dump_container);

#if 0
    // dump tables
    kas::core::core_symbol::dump(out);
    kas::core::core_section::dump(out);
    kas::core::core_segment::dump(out);
    kas::core::core_fragment::dump(out);
    kas::dwarf::dl_data::dump(out);
#endif
#if 1
    {
        parse_out << "RAW:" << std::endl;
        kas::core::emit_raw raw(kbfd_obj, parse_out);
        obj.emit(raw);
    }
#endif
#if 1
    // create object output (binary data)
    // get dest for object output
    {
        auto kbfd_path = sub_path_ext(input_path.c_str(), "kbfd");
        std::ofstream o_file(kbfd_path
                           , std::ios_base::binary | std::ios_base::trunc);
        kas::core::emit_kbfd binary(kbfd_obj, o_file);
        obj.emit(binary);
    }
#else
    // create object output (binary data)
    // get dest for object output
    {
        auto kbfd_path = sub_path_ext(input_path.c_str(), "kbfd");
        std::ofstream o_file(kbfd_path
                           , std::ios_base::binary | std::ios_base::trunc);
        kas::core::emit_binary binary(o_file);
        obj.emit(binary);
    }
#endif
#if 1
#if 1
    {
        parse_out << "LISTING:" << std::endl;
        kas::core::emit_listing<iterator_type> listing(kbfd_obj, parse_out);
        obj.emit(listing);
    }
#else
    {
        parse_out << "LISTING:" << std::endl;
        kas::core::emit_listing<iterator_type> listing(parse_out);
        obj.emit(listing);
    }
#endif
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
