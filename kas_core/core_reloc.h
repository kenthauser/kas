#ifndef KAS_CORE_CORE_RELOC_H
#define KAS_CORE_CORE_RELOC_H

#include "utility/align_as_t.h"

namespace kas::core
{

enum kas_reloc_t : uint8_t
{
      K_REL_NONE
   // , K_REL_PCREL
    , K_REL_ADD
    , K_REL_SUB
    , K_REL_GOT     // whatever these are
    , K_REL_PLT
    , K_REL_COPY
    , K_REL_GLOB_DAT
    , K_REL_JMP_SLOT
    , NUM_KAS_RELOC
};

struct core_reloc
{
    static constexpr auto RFLAGS_PC_REL = 1;

    core_reloc() = default;
    constexpr core_reloc(uint8_t reloc, uint8_t bits, uint8_t pc_rel = false)
        : reloc(reloc)
        , bits(bits)
        , flags ( pc_rel ? RFLAGS_PC_REL : 0
                )
        {}
    
    // generate `std::map` key
    constexpr auto key() const
    {
       return std::make_tuple(reloc, bits, flags);
    }

    uint8_t reloc {};
    uint8_t bits  {};
    uint8_t flags {};
};

struct reloc_info_t
{
    reloc_info_t() = default;

    constexpr reloc_info_t(uint8_t num, const char *name, core_reloc reloc)
        : num(num), name(name), reloc(reloc) {}

    core_reloc  reloc;
    const char *name;
    uint8_t     num;
};

struct reloc_op_t
{
    using read_fn_t   = int64_t(*)(int64_t data);
    using update_fn_t = int64_t(*)(int64_t data, int64_t addend);
    using write_fn_t  = int64_t(*)(int64_t data, int64_t value);

    reloc_op_t() = default;
    constexpr reloc_op_t(kas_reloc_t op
                       , update_fn_t update_fn = update_add
                       , write_fn_t  write_fn  = write_dir
                       , read_fn_t   read_fn   = read_dir
                       )
            : op(op), read_fn(read_fn), write_fn(write_fn), update_fn(update_fn) {}

    // prototype read function
    static int64_t read_dir (int64_t data) { return data; }

    // prototype write function
    static int64_t write_dir(int64_t data, int64_t value)   { return value; } 

    // prototype update functions
    static int64_t update_add(int64_t data, int64_t addend) { return data + addend; } 
    static int64_t update_sub(int64_t data, int64_t addend) { return data - addend; } 
    static int64_t update_set(int64_t data, int64_t addend) { return addend; } 

    kas_reloc_t op;
    update_fn_t update_fn;
    write_fn_t  write_fn;
    read_fn_t   read_fn;
};


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
    deferred_reloc_t(core_reloc reloc, int64_t addend = {}, uint8_t offset = {})
        : reloc(reloc), addend(addend), offset(offset) {}

    // complete construction of object
    void operator()(expr_t const&);
    void operator()(core_symbol  const&);
    void operator()(core_expr    const&);
    void operator()(core_addr    const&);

    // emit relocs & apply to base value
    void emit(emit_base& base);
    void apply_reloc(emit_base& base, reloc_info_t const& info, int64_t& addend);

    // return true if `relocs` emited OK
    static bool done(emit_base& base);
    
    reloc_info_t const *get_info(emit_base& base) const;


    core_reloc          reloc;
    int64_t             addend    {};
    core_symbol  const *sym_p     {};
    core_expr    const *expr_p    {};
    core_section const *section_p {};
    kas_loc      const *loc_p     {};
    uint8_t             width     {};
    uint8_t             offset    {};
};

}

#endif
