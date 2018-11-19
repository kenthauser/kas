
#include "kas_getopt.h"
#include "exec_options.h"
#include "kas_core/assemble.h"
#include "kas_core/emit_listing.h"
#include "elf/elf_emit.h"
#include <iostream>
#include <boost/filesystem.hpp>

using namespace kas;
namespace fs = boost::filesystem;

int main(int argc, char **argv)
{
    argv = exec::get_options(argc, argv);

    // XXX support: 1 source file
    if (argc != 1) {
        //exec::usage();
        std::cout << "as: usage " << "count = " << argc << std::endl;
        exit (1);
    }

    fs::path src_file{*argv};
    auto src_status = fs::status(src_file);
    if (!fs::exists(src_status)) {
        std::cout << "as: source file doesn't exist: " << src_file << std::endl;
        exit (1);
    }

    // XXX don't know if can read. Error's out in read path

    // create paths for output files
    fs::path obj_file{exec::exec_options.obj_file};

    // default listing file is based on object file
    // remove .o or .out extension before appending .lst
    fs::path obj_stem{obj_file};
    auto obj_ext = obj_stem.extension();
    if (obj_ext == ".out" || obj_ext == ".o")
        obj_stem.replace_extension();

    fs::path lst_file(obj_stem);
    lst_file += ".lst";
    fs::path dbg_file(obj_stem);
    dbg_file += ".debug";

    std::cout << "input : " << src_file << std::endl;
    std::cout << "output: " << obj_file << std::endl; 
    std::cout << "list  : " << lst_file << std::endl;

    // delete output files
    fs::remove(obj_file);
    fs::remove(lst_file);
    fs::remove(dbg_file);

    // read source to buffer
    std::ifstream src_stream(src_file.native(), std::ios::binary | std::ios::ate);
    auto size = src_stream.tellg();
    src_stream.seekg(0, std::ios::beg);

    std::vector<char> src_data(size);;
    std::noskipws(src_stream);

    src_stream.read(src_data.data(), size);
    
    // 
    std::ofstream parse_out(dbg_file.native());
    std::cout  << "\nparse begins: " << src_file << std::endl;
    kas::core::kas_assemble obj;

    obj.assemble(src_data.begin(), src_data.end(), &parse_out);

#if 1
    std::ofstream list_stream(lst_file.native(), std::ios::binary);
    kas::core::emit_listing<parser::iterator_type> listing(list_stream);
    obj.emit(listing);
    kas::core::core_symbol::dump(list_stream);
#endif
#if 1
    kas::elf::elf_emit elf_obj(ELFCLASS32, ELFDATA2MSB, EM_68K); 
    obj.emit(elf_obj);

    std::ofstream elf_out(obj_file.native(), std::ios_base::binary);
    elf_obj.write(elf_out);
#endif
    
    return 0;
}

