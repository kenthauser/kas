#ifndef KBFD_KBFD_RELOC_OPS_H
#define KBFD_KBFD_RELOC_OPS_H

#include "kbfd_reloc_ops_base.h"
#include "kbfd.h"

namespace kbfd
{


// define relocation ops as `types`
using K_REL_NONE     = KAS_STRING("K_REL_NONE");
using K_REL_ADD      = KAS_STRING("K_REL_ADD");
using K_REL_SUB      = KAS_STRING("K_REL_SUB");
using K_REL_GOT      = KAS_STRING("K_REL_GOT");
using K_REL_PLT      = KAS_STRING("K_REL_PLT");
using K_REL_COPY     = KAS_STRING("K_REL_COPY");
using K_REL_GLOB_DAT = KAS_STRING("K_REL_GLOB_DAT");
using K_REL_JMP_SLOT = KAS_STRING("K_REL_JMP_SLOT");

using ARM_REL_MOVW   = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT   = KAS_STRING("ARM_REL_MOVT");



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
    using reloc_op_fns::reloc_op_fns;
    update_t update(value_t data, value_t addend) const override { return data + addend; }  
};

struct k_rel_sub_t : reloc_op_fns
{
    using reloc_op_fns::reloc_op_fns;
    update_t update(value_t data, value_t addend) const override { return data - addend; } 
};

using reloc_ops_v = meta::list<
        K_REL_NONE
      , K_REL_ADD
      , K_REL_SUB
      , K_REL_GOT
      , K_REL_PLT
      , K_REL_COPY
      , K_REL_GLOB_DAT
      , K_REL_JMP_SLOT
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

template <> struct X_reloc_ops_v<K_REL_NONE> : meta::list<
        KBFD_ACTION<K_REL_NONE  , reloc_op_fns>
      , KBFD_ACTION<K_REL_ADD   , k_rel_add_t>
      , KBFD_ACTION<K_REL_SUB   , k_rel_sub_t>
      > {};

using reloc_ops_defn_tags = meta::push_front<target_tags, K_REL_NONE>;
using all_reloc_ops = all_defns<X_reloc_ops_v, reloc_ops_defn_tags>;

uint8_t reloc_get_action(const char *);

}
#endif
