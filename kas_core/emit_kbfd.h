#ifndef KAS_CORE_EMIT_KBFD_H
#define KAS_CORE_EMIT_KBFD_H

// emit_kbfd
//
// Use `kbfd` to generate object file
//
// Cobble together an object from kbfd::elf_format, kbfd::elf_object,
// and core::elf_stream to perform object file creation
//
// NB: member declaration order is object creation order

#include "stream_kbfd.h"
#include "core_emit.h"

#include <fstream>
#if 0
namespace kas::core
{

struct emit_binary : emit_base
{
    // ctor using ostream&
    emit_binary(std::ostream& out)
        : stream(out), emit_base(stream)
        {}

    // ctor using path (allocate ostream object)
    emit_binary(const char *path)
        : out_p(new std::ofstream(
                        path
                      , std::ios_base::binary | std::ios_base::trunc 
                      ))
        , stream(*out_p)
        , emit_base(stream)
    {
        std::cout << "emit_binary: allocating \"" << path << "\"" << std::endl;
    }

    // ctor: allow string for path
    emit_binary(std::string const& path)
        : emit_binary(path.c_str()) {}

    // dtor: close file if allocated
    ~emit_binary()
    {
        std::cout << "emit_binary: dtor" << std::endl;
        delete out_p;
    }

    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always known
        insn.emit(*this, dot_p);
    }

    // default: trampoline to object format method
    template <typename ELF_FORMAT, typename OS>
    auto write(ELF_FORMAT const& fmt, OS& os)
    {
        // XXX return fmt.write(obj, os);    
    }

private:
    // XXX this should be `std::unique_ptr`
    std::ofstream *out_p {};
    kbfd_stream    stream;
};
}

#endif


#endif
