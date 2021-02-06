#ifndef KBFD_KBFD_RELOC_OPS_H
#define KBFD_KBFD_RELOC_OPS_H

#include "kbfd.h"
#include "kbfd_reloc_ops_base.h"

namespace kbfd
{

// define standard relocation ops as `types`
using K_REL_NONE     = KAS_STRING("K_REL_NONE");
using K_REL_ADD      = KAS_STRING("K_REL_ADD");
using K_REL_SUB      = KAS_STRING("K_REL_SUB");
using K_REL_GOT      = KAS_STRING("K_REL_GOT");
using K_REL_PLT      = KAS_STRING("K_REL_PLT");
using K_REL_COPY     = KAS_STRING("K_REL_COPY");
using K_REL_GLOB_DAT = KAS_STRING("K_REL_GLOB_DAT");
using K_REL_JMP_SLOT = KAS_STRING("K_REL_JMP_SLOT");

// need `type` for generic relocation ops. Pick `K_REL_NONE`
using reloc_ops_defn_tags = meta::push_front<target_tags, K_REL_NONE>;

template <> struct reloc_ops_v<K_REL_NONE> : meta::list<
        KBFD_ACTION<K_REL_NONE  , reloc_op_fns>
      , KBFD_ACTION<K_REL_ADD   , k_rel_add_t>
      , KBFD_ACTION<K_REL_SUB   , k_rel_sub_t>
      > {};

}
#endif
