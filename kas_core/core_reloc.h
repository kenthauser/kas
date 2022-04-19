#ifndef KAS_CORE_CORE_RELOC_H
#define KAS_CORE_CORE_RELOC_H

// `core_reloc` holds pending relocations
// 
// `core_emit` accepts "relocations" before it emits the base value
// to be "relocated". `kbfd` need the value to be relocated before
// it applies relocations.
//
// Thus `core_reloc` holds the base relocation action (a `kbfd_reloc`),
// the location offset (a small positive offset, normally zero), 
// and the relocation addend to be relocated (an `expr_t` + constant)

// Relocation objects are generally created holding type and location
// of relocation, and then modified (via `call` operator) to hold
// value being relocated.

// All relocations also need to provide a `kas_position_tagged` object
// for any resulting error message


#include "expr/expr.h"
#include "kbfd/kbfd_reloc.h"

namespace kas::core
{

struct core_emit;       // forward declaration

struct core_reloc
{
    // locally expose constants from `kbfd`
    static constexpr auto RFLAGS_PC_REL = kbfd::kbfd_reloc::RFLAGS_PC_REL;
    static constexpr auto RFLAGS_SB_REL = kbfd::kbfd_reloc::RFLAGS_SB_REL;

    // control `core_reloc` operation
    static constexpr auto CR_EMIT_BARE   = 0x1; // emit reloc without "value"
    static constexpr auto CR_EMIT_SYM    = 0x2; // don't convert SYM -> addr
    static constexpr auto CR_EMIT_BRANCH = 0x4; // resolve as branch

    core_reloc()
    {
#if defined(TRACE_CORE_RELOC) 
        std::cout << "core_reloc::default ctor" << std::endl;
#endif
        // only lookup default reloc once
        static kbfd::kbfd_reloc _proto { kbfd::K_REL_ADD() };
        reloc = _proto;
    }

    core_reloc(kbfd::kbfd_reloc reloc
             , kas_position_tagged const *loc_p = {}
             , int64_t addend = {}
             , uint8_t offset = {}, uint8_t r_flags = {})
        : reloc(std::move(reloc)), loc_p(loc_p)
        , addend(addend), offset(offset), r_flags(r_flags)
    {
#if defined(TRACE_CORE_RELOC) 
        std::cout << "core_reloc::ctor: ";
        std::cout << "reloc = "    << reloc;
        if (addend)
            std::cout << ", addend = " << std::dec << addend;
        if (offset)
            std::cout << ", offset = " << +offset;
        if (loc_p)
            std::cout << ", loc = " << loc_p->src();
        std::cout << std::endl;
#endif
    }

    // methods to complete construction of object
    // NB: operator()(core_addr_t const&) selects appropriate base
    //     symbol for reloc. (based on backend)
    core_reloc& operator()(expr_t const&, uint8_t flags = {});
    core_reloc& operator()(core_addr_t   const&);
    core_reloc& operator()(core_symbol_t const&);
    core_reloc& operator()(core_expr_t   const&);
    core_reloc& operator()(parser::kas_diag_t const&);

    // cause `*this` to be emitted
    void emit       (core_emit&, int64_t& accum);

    // utilities for `reloc`
    kbfd::kbfd_target_reloc const *get_tgt_reloc(core_emit&);
    bool gen_reloc() const  { return section_p || sym_p; }
    void gen_diag(core_emit&, const char *msg, const char *desc = {}) const;
    
    // return true iff `relocs` emited OK
    static bool done(core_emit& base);


    // hold info about relocation
    kas_position_tagged const *loc_p {};
    kbfd::kbfd_reloc    reloc;
    uint8_t             offset  {};
    uint8_t             r_flags {};
    
    // hold info about value (ie the "addend") to be relocated
    int64_t              addend      {};
    // XXX int64_t              value       {};    // constant or section offset
    core_section  const *section_p   {};
    core_symbol_t const *sym_p       {};
    core_expr_t   const *core_expr_p {};

    // support values
    parser::kas_diag_t const *diag_p {};
};

}

#endif
