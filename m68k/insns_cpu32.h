#ifndef KAS_M68K_INSNS_CPU32_H
#define KAS_M68K_INSNS_CPU32_H

#include "m68k_insn_common.h"

namespace kas::m68k::opc::ns_cpu32
{

#define STR KAS_STRING

using m68k_cpu32_v = list<list<>
// processor control instructions
, insn<sz_v , STR("bgnd"), OP<0x4afa, cpu32>>
, insn<sz_W , STR("lpstop"), OP<0xf800'01c0, cpu32> , void, IMMED>

// 020 instructions supported by CPU32
, insn<sz_lwb, STR("cmp2"), OP<0x00c0'0000, cpu32>, FMT_0RM_12RM, CONTROL, GEN_REG>
, insn<sz_lwb, STR("chk2"), OP<0x00c0'0800, cpu32>, FMT_0RM_12RM, ALTERABLE, GEN_REG>
, insn<sz_vl , STR("extb"), OP<0x49c0, cpu32>, FMT_0D, DATA_REG>
, insn<sz_l  , STR("link"), OP<0x4808, cpu32>, FMT_0A, ADDR_REG, IMMED>

// table lookup instructions
, insn<sz_lwb, STR("tbls"),  OP<0xf800'0900, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_12RM, CONTROL_ALTER, DATA_REG>
, insn<sz_lwb, STR("tblsn"), OP<0xf800'0b00, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_12RM, CONTROL_ALTER, DATA_REG>
, insn<sz_lwb, STR("tbls"),  OP<0xf800'0800, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_12RM, PAIR, DATA_REG>
, insn<sz_lwb, STR("tblsn"), OP<0xf800'0a00, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_12RM, PAIR, DATA_REG>
, insn<sz_lwb, STR("tblu"),  OP<0xf800'0100, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_12RM, CONTROL_ALTER, DATA_REG>
, insn<sz_lwb, STR("tblun"), OP<0xf800'0500, table, INFO_SIZE_NORM1>
                                        , FMT_0RM_12RM, CONTROL_ALTER, DATA_REG>
, insn<sz_lwb, STR("tblu"),  OP<0xf800'0000, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_12RM, PAIR, DATA_REG>
, insn<sz_lwb, STR("tblun"), OP<0xf800'0400, table, INFO_SIZE_NORM1>
                                        , FMT_TBLPAIR_12RM, PAIR, DATA_REG>
>;

#undef STR
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_CPU32> : ns_cpu32::m68k_cpu32_v {};
}

#endif
