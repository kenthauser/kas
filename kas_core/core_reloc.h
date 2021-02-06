#ifndef KAS_CORE_CORE_RELOC_H
#define KAS_CORE_CORE_RELOC_H

#include "expr/expr.h"
#include "kbfd/kbfd_reloc.h"
#include "utility/align_as_t.h"

namespace kas::core
{

struct emit_base;       // forward declaration
struct deferred_reloc_t
{
    // static flags structure in `emit_base`
    struct flags_t : kas::detail::alignas_t<flags_t, uint16_t>
    {
        using base_t::base_t;

        value_t  error : 1;
    };
    
    deferred_reloc_t() = default;
    deferred_reloc_t(kbfd::kbfd_reloc reloc, int64_t addend = {}, uint8_t offset = {})
        : reloc(reloc), addend(addend), offset(offset)
    {
#if 0
        std::cout << "deferred_reloc_t::ctor: addend = " << addend;
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

    kbfd::kbfd_reloc    reloc;
    int64_t             addend       {};
    core_symbol_t const *sym_p       {};
    core_expr_t   const *core_expr_p {};
    core_section  const *section_p   {};
    parser::kas_diag_t const *diag_p {};
    parser::kas_loc const *loc_p     {};
    uint8_t             width        {};     // XXX refactor out.
    uint8_t             offset       {};
};

}

#endif
