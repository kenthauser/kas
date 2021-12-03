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

struct core_emit;       // forward declaration of calling type

struct core_reloc
{
    // locally expose constants from `kbfd`
    static constexpr auto RFLAGS_PC_REL = kbfd::kbfd_reloc::RFLAGS_PC_REL;

    // control `core_reloc` operation
    static constexpr auto CR_EMIT_BARE = 0x1;

    core_reloc()
    {
#if defined(TRACE_CORE_RELOC) && 0
        std::cout << "core_reloc::default ctor" << std::endl;
#endif
        // only lookup default reloc once
        static kbfd::kbfd_reloc _proto { kbfd::K_REL_ADD() };
        reloc = _proto;
    }

    core_reloc(kbfd::kbfd_reloc reloc, int64_t addend = {}
             , uint8_t offset = {}, uint8_t r_flags = {})
        : reloc(std::move(reloc)), addend(addend), offset(offset), r_flags(r_flags)
    {
#if defined(TRACE_CORE_RELOC) && 0
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
    // NB: operator()(core_addr_t const&) selects appropriate base
    //     symbol for reloc. (based on backend)
    void operator()(expr_t const&, uint8_t flags = {});
    void operator()(core_addr_t   const&, kas_loc const * = {});
    void operator()(core_symbol_t const&, kas_loc const * = {});
    void operator()(core_expr_t   const&, kas_loc const * = {});
    void operator()(parser::kas_diag_t const&, kas_loc const *);

    // cause `*this` to be emitted
    void emit       (core_emit&, parser::kas_error_t&);

    // utilities for `reloc`
    kbfd::kbfd_target_reloc const *get_tgt_reloc(core_emit&);
    
    // utilities for `emit`
    void put_reloc  (core_emit&, parser::kas_error_t&, core_section  const&);
    void put_reloc  (core_emit&, parser::kas_error_t&, core_symbol_t const&);
    void put_reloc  (core_emit&, parser::kas_error_t&);
    void apply_reloc(core_emit&, parser::kas_error_t&);

    // return true iff `relocs` emited OK
    static bool done(core_emit& base);

    // hold info about relocation
    kbfd::kbfd_reloc    reloc;
    uint8_t             offset  {};
    uint8_t             r_flags {};
    
    // hold info about value (ie the "addend") to be relocated
    int64_t              addend      {};
    core_section  const *section_p   {};
    core_symbol_t const *sym_p       {};
    core_expr_t   const *core_expr_p {};

    // support values
    parser::kas_diag_t const *diag_p {};
    parser::kas_loc    const *loc_p  {};
};

}

#endif
