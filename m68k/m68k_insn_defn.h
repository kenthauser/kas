#ifndef KAS_M68K_M68K_INSN_DEFN_H
#define KAS_M68K_M68K_INSN_DEFN_H


#include "target/tgt_insn_defn.h"


namespace kas::target::opc
{

template <>
template <typename SIZE_OBJ, typename SZ>
auto tgt_insn_defn<m68k::m68k_mcode_t>::names(SIZE_OBJ const& obj, SZ const& sz_instance)
{
}


}


#endif
