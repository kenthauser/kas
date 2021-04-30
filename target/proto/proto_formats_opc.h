#ifndef KAS_PROTO_UC_PROTO_UC_FORMATS_OPC_H
#define KAS_PROTO_UC_PROTO_UC_FORMATS_OPC_H

#include "PROTO_LC_mcode.h"
#include "target/tgt_format.h"


namespace kas::PROTO_LC::opc
{
// derive PROTO_LC version of `tgt` classes
// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<PROTO_LC_mcode_t, Ts...>;

// use generic template to generate argument `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<PROTO_LC_mcode_t, N, T>;

// NB: use `target` default implementation
using fmt_gen    = tgt::opc::tgt_fmt_opc_gen   <PROTO_LC_mcode_t>;
using fmt_list   = tgt::opc::tgt_fmt_opc_list  <PROTO_LC_mcode_t>;
using fmt_branch = tgt::opc::tgt_fmt_opc_branch<PROTO_LC_mcode_t>;
}

#endif
