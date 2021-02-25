#ifndef KAS_CORE_CORE_RELOC_H
#define KAS_CORE_CORE_RELOC_H

// `core_reloc` holds pending relocations
// 
// `core_emit` issues "relocations" before it emits the base value
// to be "relocated". `kbfd` need the value to be relocated before
// it applies relocations.
//
// Thus `core_reloc` holds the base relocation action (a `kbfd_reloc`),
// the location offset (a small positive offset, normally zero), 
// and the relocation addend to be relocated (an `expr_t` + constant)

#include "expr/expr.h"
#include "kbfd/kbfd_reloc.h"

namespace kas::core
{

struct emit_base;       // forward declaration of calling type

#define TRACE_CORE_RELOC
struct core_reloc
{
    // locally expose constants from `kbfd`
    static constexpr auto RFLAGS_PC_REL = kbfd::kbfd_reloc::RFLAGS_PC_REL;

    core_reloc()
    {
#ifdef TRACE_CORE_RELOC
        std::cout << "core_reloc::default ctor" << std::endl;
#endif
        // only lookup `K_REL_ADD` once
        static kbfd::kbfd_reloc _proto { kbfd::K_REL_ADD() };
        reloc = _proto;
    }

    core_reloc(kbfd::kbfd_reloc reloc, int64_t addend = {}, uint8_t offset = {})
        : reloc(std::move(reloc)), addend(addend), offset(offset)
    {
#ifdef TRACE_CORE_RELOC
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
    void operator()(core_addr_t   const&, kas_loc const * = {});
    void operator()(core_symbol_t const&, kas_loc const * = {});
    void operator()(core_expr_t   const&, kas_loc const * = {});
    void operator()(parser::kas_diag_t const&, kas_loc const *);

    // emit relocs & apply to base value (if required)
    void emit       (emit_base&, parser::kas_error_t&);
    void put_reloc  (emit_base&, parser::kas_error_t&, core_section  const&);
    void put_reloc  (emit_base&, parser::kas_error_t&, core_symbol_t const&);
    void apply_reloc(emit_base&, parser::kas_error_t&);

    // return true iff `relocs` emited OK
    static bool done(emit_base& base);

    // hold info about relocation
    kbfd::kbfd_reloc    reloc;
    uint8_t             offset  {};
    
    // hold info about value (ie the "addend") to be relocated
    int64_t              addend      {};
    core_section  const *section_p   {};
    core_symbol_t const *sym_p       {};
    core_expr_t   const *core_expr_p {};

    // support values
    parser::kas_diag_t const *diag_p {};
    parser::kas_loc const *loc_p     {};
};

}

#endif
