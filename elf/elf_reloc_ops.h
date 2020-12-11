#ifndef KAS_ELF_ELF_RELOC_OPS_H
#define KAS_ELF_ELF_RELOC_OPS_H

namespace kas::elf
{

// generic relocation operations
enum kas_reloc_op : uint8_t
{
      K_REL_NONE
    , K_REL_ADD
    , K_REL_SUB
    , K_REL_GOT
    , K_REL_PLT
    , K_REL_COPY
    , K_REL_GLOB_DAT
    , K_REL_JMP_SLOT
    , NUM_KAS_RELOC_OP
};


// XXX move to ARM
// ARM specific relocation operations
enum arm_reloc_op : uint8_t
{
      ARM_REL_MOVW  = NUM_KAS_RELOC_OP
    , ARM_REL_MOVT
    , NUM_ARM_RELOC
};

#if 0
struct reloc_op_fns
{
    using read_fn_t   = int64_t(*)(int64_t data);
    using write_fn_t  = int64_t(*)(int64_t data, int64_t value);
    using update_fn_t = int64_t(*)(int64_t data, int64_t addend);

    constexpr reloc_op_fns(update_fn_t update_fn = {}
                         , write_fn_t  write_fn  = {}
                         , read_fn_t   read_fn   = {}
                         )
            : read_fn(read_fn), write_fn(write_fn), update_fn(update_fn) {}

    // prototype update functions
    static int64_t update_add(int64_t value, int64_t addend) { return value + addend; } 
    static int64_t update_sub(int64_t value, int64_t addend) { return value - addend; } 
    static int64_t update_set(int64_t value, int64_t addend) { return addend; } 

    // test if operation is defined
    constexpr operator bool() const { return update_fn; }

    update_fn_t update_fn;
    write_fn_t  write_fn;
    read_fn_t   read_fn;
};

#else

struct reloc_op_fns
{
    using value_t  = int64_t;
    struct update_t : std::pair<value_t, bool>
    {
        constexpr update_t(value_t data = {}, bool defined = true)
                    : std::pair<value_t, bool>(data, defined) {}
    };

    constexpr reloc_op_fns() {}

    // default: read/write return data/value un-modified
    virtual value_t  read  (value_t data)                 const { return data;  }
    virtual value_t  write (value_t data, value_t value)  const { return value; }

    // bool true if update defined
    virtual update_t update(value_t data, value_t addend) const { return {}; };
#if 0
    // prototype update functions
    static value_t update_add(value_t value, value_t addend) { return value + addend; } 
    static value_t update_sub(value_t value, value_t addend) { return value - addend; } 
    static value_t update_set(value_t value, value_t addend) { return addend; } 
#endif
};


#endif

namespace detail 
{
    // read 4 bits shifted 12
    // XXX 
    inline int64_t read_4_12(int64_t data)
    {
        auto value = (data >> 4) & 0xf000;
        return value | (data & 0xfff);
    }

    inline int64_t write_4_12(int64_t data, int64_t value)
    {
        auto mask = 0xf0fff;
        data  &=~ mask;
        value |= (value & 0xf000) << 4;
        return data | (value & mask);
    }
}
#if 0
using reloc_ops_t = std::pair<uint8_t, reloc_op_fns>;
constexpr reloc_ops_t reloc_ops [] =
{
     { K_REL_ADD    , { reloc_op_fns::update_add }}
   , { K_REL_SUB    , { reloc_op_fns::update_sub }}
   , { K_REL_COPY   , { reloc_op_fns::update_set }}
//   , { ARM_REL_MOVW , { reloc_op_fns::update_add, detail::write_4_12, detail::read_4_12 }}
};
#endif
// XXX unimplemented
using k_rel_none_t = reloc_op_fns;
using k_rel_got_t  = reloc_op_fns;
using k_rel_plt_t  = reloc_op_fns;
using k_rel_copy_t = reloc_op_fns;
using k_rel_glob_dat_t = reloc_op_fns;
using k_rel_jmp_slot_t = reloc_op_fns;

struct k_rel_add_t : reloc_op_fns
{
    update_t update(value_t data, value_t addend) const override { return data + addend; }  
};

struct k_rel_sub_t : reloc_op_fns
{
    update_t update(value_t data, value_t addend) const override { return data - addend; } 
};

using reloc_ops_v = meta::list<
        k_rel_none_t
      , k_rel_add_t
      , k_rel_sub_t
      , k_rel_got_t
      , k_rel_plt_t
      , k_rel_copy_t
      , k_rel_glob_dat_t
      , k_rel_jmp_slot_t
      >;

// much of this is taken care of by `init_from_list`
static constexpr k_rel_none_t k_rel_none;
static constexpr k_rel_add_t  k_rel_add;
static constexpr k_rel_sub_t  k_rel_sub;

constexpr reloc_op_fns const * const reloc_ops_p [reloc_ops_v::size()] =
    {
          &k_rel_none
        , &k_rel_add
        , &k_rel_sub
    };
}
#endif
