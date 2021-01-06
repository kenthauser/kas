#ifndef KAS_CORE_EMIT_OBJECT_H
#define KAS_CORE_EMIT_OBJECT_H

// emit_object
//
// Generate a proper backend-object to allow generation of object file
//
// Cobble together an object from kbfd::elf_format, kbfd::elf_object,
// and core::elf_stream to perform object file creation
//
// NB: member declaration order is object creation order

#include "kbfd_stream.h"
#include "core_emit.h"
#include "kbfd/kbfd_object.h"
#include "kbfd/kbfd_format.h"

namespace kas::core
{

namespace detail
{
    // *** Declare object format ***
    // NB: Must override via include
    template <typename = void> struct obj_format: meta::id<void> {};
}

struct emit_object : core::emit_base
{
#if 0
    template <typename ELF_FORMAT>
    emit_object(ELF_FORMAT const& fmt)
        : elf_obj(fmt)
        , stream(elf_obj)
        , emit_base{stream}
        {}
#else
    emit_object(kbfd::kbfd_object& obj)
        : obj(obj)
        , stream(obj)
        , emit_base(stream)
        {}
#endif

    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always known
        insn.emit(*this, dot_p);
    }

    // default: trampoline to object format method
    template <typename ELF_FORMAT, typename OS>
    auto write(ELF_FORMAT const& fmt, OS& os)
    {
        return fmt.write(obj, os);    
    }

private:
    kbfd::kbfd_object obj;
    kbfd_stream       stream;         // must be declared after `obj`
};
}



#endif
