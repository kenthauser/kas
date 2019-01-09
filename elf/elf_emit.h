#ifndef KAS_ELF_ELF_EMIT_H
#define KAS_ELF_ELF_EMIT_H

#include "elf_stream.h"
#include "kas_core/core_emit.h"

namespace kas::elf
{

struct elf_emit : core::emit_base
{
    template <typename...Ts>
    elf_emit(Ts&&...args)
        : stream{std::forward<Ts>(args)...}
        , emit_base{stream}
        {}

    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always known
        insn.emit(*this, dot_p);
    }

    template <typename OS>
    auto write(OS& os)
    {
        return stream.file().write(os);    
    }

private:
    elf_stream stream;
};
}



#endif
