#ifndef KAS_CORE_CORE_RELOC_H
#define KAS_CORE_CORE_RELOC_H

// `core_reloc` holds pending relocation
// 
// `core_emit` issues "relocations" before it emits the base value
// which is "relocated". `kbfd` need the value to be relocated before
// it applys relocations.
//
// Thus `core_reloc` holds the base relocation (a `kbfd_reloc`), 
// the location offset, the relocation addend, and the expression
// to be relocated. 

#include "expr/expr.h"
#include "kbfd/kbfd_reloc.h"

namespace kas::core
{

struct emit_base;       // forward declaration
struct core_reloc
{
    core_reloc() = default;
    core_reloc(kbfd::kbfd_reloc reloc, int64_t addend = {}, uint8_t offset = {})
        : reloc(reloc), addend(addend), offset(offset)
    {
#if 1
        std::cout << "core_reloc::ctor: ";
        std::cout << "reloc = "    << reloc;
        if (addend)
            std::cout << ", addend = " << addend;
        if (offset)
            std::cout << ", offset = " << +offset;
        std::cout << std::endl;
#endif
    }

    // methods to complete construction of object
    void operator()(expr_t const&);
    void operator()(core_symbol_t const&, kas_loc const *);
    void operator()(core_expr_t   const&, kas_loc const *);
    void operator()(core_addr_t   const&, kas_loc const *);
    void operator()(parser::kas_diag_t const&, kas_loc const *);

    // emit relocs & apply to base value
    void emit       (emit_base&, parser::kas_error_t&);
    void put_reloc  (emit_base&, parser::kas_error_t&, core_section  const&);
    void put_reloc  (emit_base&, parser::kas_error_t&, core_symbol_t const&);
    void apply_reloc(emit_base&, parser::kas_error_t&);

    // return true iff `relocs` emited OK
    static bool done(emit_base& base);

    // hold info about relocation
    kbfd::kbfd_reloc    reloc;
    uint8_t             offset       {};
    int64_t             addend       {};
    
    // hold info about value to be relocated
    core_symbol_t const *sym_p       {};
    core_expr_t   const *core_expr_p {};
    core_section  const *section_p   {};
    parser::kas_diag_t const *diag_p {};
    parser::kas_loc const *loc_p     {};
};

}

#endif
