#ifndef KAS_CORE_CORE_EMIT_RELOC_H
#define KAS_CORE_CORE_EMIT_RELOC_H

#include "expr/expr.h"
#include "emit_stream.h"
#include "core_fragment.h"
#include "core_symbol.h"
#include "core_addr.h"
#include "utility/print_object.h"

#include <iostream>

#include "utility/print_type_name.h"

#include "core_emit.h"

namespace kas::core
{

void core_emit::emit_relocs()
{
    auto p = relocs.begin();    // pointer into reloc array
    write_cb = {};              // if need to store accum in `data` 
    accum    = {};
    use_rela = {};              // XXX need target-specific init.

    // ELF: if more that one reloc for single location, must use rela
    // NB: reloc_p is non-zero if `emit_reloc` method is driven.
    if ((reloc_p - p) > 1)
        use_rela = true;

    for (; p != reloc_p; ++p)
        p->emit(*this, accum);
}

void core_emit::put_reloc (core_reloc& r, core_symbol_t const& symbol)
{
    // if not `rela`, insert `accum` into `data` before emitting reloc
    // NB: if error in consuming `accum`, can fallback to `rela`
    if (accum && !use_rela)
        reloc_update_data(r);

    if (auto p = get_target_reloc(r))
        stream.put_symbol_reloc(e_chan, *p, symbol, r.addend, use_rela);
}

void core_emit::put_reloc(core_reloc& r, core_section const *section_p)
{
    // if not `rela`, insert `accum` into `data` before emitting reloc
    // NB: if error in consuming `accum`, can fallback to `rela`
    if (accum && !use_rela)
        reloc_update_data(r, !section_p);

    // emit `reloc` if required
    if (section_p || r.reloc.get().emit_bare())
        if (auto p = get_target_reloc(r))
            stream.put_section_reloc(e_chan, *p, section_p, r.addend, use_rela);
}

// if `force` is set, failure to encode generates diagnostic
void core_emit::reloc_update_data(core_reloc& r, bool force)
{
    std::cout << "core_emit::reloc_update_data" << std::endl;
    auto& ops = r.reloc.get();
    auto msg = ops.encode(accum);   // test for error

    if (msg)
    {
        // can't encode value in data.
        // if `relocation write pending, use rela.
        // else, raise error
        if (!force)
            use_rela = true;        // use alternate path
        else
            r.gen_diag(*this, msg);
    }
    else
    {
        ops.insert(data, accum);    // all is good...
    }
}


// XXX is this needed???
core_symbol_t const * 
    core_emit::reloc_select_base_sym(core_symbol_t const& sym) const
{
    return &sym;      // use symbol 
}

// used by `core_reloc::emit`. Trampoline to `kbfd` function
bool core_emit::should_resolve(core_reloc const& r
                             , core_symbol_t const& symbol) const
{
    // XXX should be passed: binding, type, visibility...
    // XXX ARM special: don't resolve if FUNCTION
    return !(symbol.kind() & STT_FUNC);
}

// XXX pass `core_reloc`. Emit DIAG if problem
kbfd::kbfd_target_reloc const * 
    core_emit::get_target_reloc(core_reloc& r)
{
    // if no width specified by reloc, use current width
    r.reloc.default_width(width);
    if (auto p = obj_p->get_reloc(r.reloc))
        return p;
    r.gen_diag(*this, "E invalid relocation");
    return {};
}



}   // namespace kas_core
#endif
