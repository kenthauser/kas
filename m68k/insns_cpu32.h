#ifndef KAS_M68K_INSNS_CPU32_H
#define KAS_M68K_INSNS_CPU32_H

#include "m68k_insn_common.h"

namespace kas::m68k::opc::ns_cpu32
{

#define STR KAS_STRING

using m68k_cpu32_v = list<list<>
// processor control instructions
, defn<sz_v , STR("bgnd"), OP<0x4afa, cpu32>>
, defn<sz_W , STR("lpstop"), OP<0xf800'01c0, cpu32> , void, IMMED>

// 020 instructions supported by CPU32
, defn<sz_lwb, STR("cmp2"), OP<0x00c0'0000, cpu32>, FMT_0RM_28RM, CONTROL, GEN_REG>
, defn<sz_lwb, STR("chk2"), OP<0x00c0'0800, cpu32>, FMT_0RM_28RM, ALTERABLE, GEN_REG>
, defn<sz_vl , STR("extb"), OP<0x49c0, cpu32>, FMT_0, DATA_REG>
, defn<sz_l  , STR("link"), OP<0x4808, cpu32>, FMT_0, ADDR_REG, IMMED>

// table lookup instructions
, defn<sz_lwb, STR("tbls"),  OP<0xf800'0900, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_28, CONTROL_ALTER, DATA_REG>
, defn<sz_lwb, STR("tblsn"), OP<0xf800'0b00, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_28, CONTROL_ALTER, DATA_REG>
, defn<sz_lwb, STR("tbls"),  OP<0xf800'0800, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_28, PAIR, DATA_REG>
, defn<sz_lwb, STR("tblsn"), OP<0xf800'0a00, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_28, PAIR, DATA_REG>
, defn<sz_lwb, STR("tblu"),  OP<0xf800'0100, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_28RM, CONTROL_ALTER, DATA_REG>
, defn<sz_lwb, STR("tblun"), OP<0xf800'0500, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_28, CONTROL_ALTER, DATA_REG>
, defn<sz_lwb, STR("tblu"),  OP<0xf800'0000, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_28, PAIR, DATA_REG>
, defn<sz_lwb, STR("tblun"), OP<0xf800'0400, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_28, PAIR, DATA_REG>
>;

#undef STR
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_CPU32> : ns_cpu32::m68k_cpu32_v {};
}

#endif
