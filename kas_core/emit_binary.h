#ifndef KAS_CORE_EMIT_BINARY_H
#define KAS_CORE_EMIT_BINARY_H

// emit_binary
//
// Generate backend-object to allow generation of object file.
//
// Cobble together an object from kbfd::elf_format, kbfd::elf_object,
// and core::elf_stream to perform object file creation
//
// NB: member declaration order is object creation order

#include "kbfd_stream.h"
#include "core_emit.h"

#include <fstream>

namespace kas::core
{

struct emit_binary : emit_base
{
    // ctor using ofstream&
    emit_binary(std::ostream& out)
        : stream(out), emit_base(stream)
        {}

    // ctor using path (allocate local ofstream object)
    emit_binary(std::string const& path)
        : emit_binary(path.c_str()) {}

    emit_binary(const char *path)
        : out_p(new std::ofstream(
                        path
                      , std::ios_base::binary | std::ios_base::trunc 
                      ))
        , stream(*out_p)
        , emit_base(stream)
        {}

    // dtor: close file if allocated
    ~emit_binary()
    {
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
    std::ofstream *out_p {};
    kbfd_stream    stream;
};
}



#endif
