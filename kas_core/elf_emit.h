#ifndef KAS_CORE_ELF_EMIT_H
#define KAS_CORE_ELF_EMIT_H

// elf_emit.h
//
// Generate a proper backend-object to allow generation of object file
//
// Cobble together an object from elf::elf_format, elf::elf_object,
// and core::elf_stream to perform object file creation
//
// NB: member declaration order is object creation order

#include "elf_stream.h"
#include "core_emit.h"
#include "elf/elf_object.h"
#include "elf/elf_format.h"

namespace kas::core
{

struct elf_emit : core::emit_base
{

    template <typename ELF_FORMAT>
    elf_emit(ELF_FORMAT const& fmt)
        : elf_obj(fmt)
        , stream(elf_obj)
        , emit_base{stream}
        {}

    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always known
        insn.emit(*this, dot_p);
    }
#if 1
    template <typename OS>
    auto write(OS& os)
    {
        return elf_obj.write(os);    
    }
#endif
private:
    elf::elf_object elf_obj;
    elf_stream      stream;

};
}



#endif
