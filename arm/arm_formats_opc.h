#ifndef KAS_ARM_ARM_FORMATS_OPC_H
#define KAS_ARM_ARM_FORMATS_OPC_H

#include "arm_mcode.h"
#include "target/tgt_format.h"


namespace kas::arm::opc
{
// derive arm version of `tgt` classes
// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<arm_mcode_t, Ts...>;

// use generic template to generate argument `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<arm_mcode_t, N, T>;

// NB: use `target` default implementation
using fmt_gen    = tgt::opc::tgt_fmt_opc_gen   <arm_mcode_t>;
using fmt_list   = tgt::opc::tgt_fmt_opc_list  <arm_mcode_t>;
using fmt_branch = tgt::opc::tgt_fmt_opc_branch<arm_mcode_t>;
}

#endif
