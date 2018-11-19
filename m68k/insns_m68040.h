#ifndef KAS_M68K_M68040_DEFNS_H
#define KAS_M68K_M68040_DEFNS_H

// new instructions for the '040 processor

// NB: the '060 processor has separate MMU commands
//     and deletes the `movep` instruction.


#include "m68k_insn_common.h"

namespace kas::m68k::opc::gen_040
{

#define STR KAS_STRING

///////////////////////////////////////////////////////////////////////////////
//
// `move16` instructions
//

using m68k_move16_v = list<list<>
// move16
, insn<sz_v, STR("move16"), OP<0xf620'8000, m68040>, FMT_0P_12P, POST_INCR, POST_INCR>
, insn<sz_v, STR("move16"), OP<0xf620,      m68040>, FMT_0P,     POST_INCR, DIR_LONG>
, insn<sz_v, STR("move16"), OP<0xf630,      m68040>, FMT_0S,     ADDR_INDIR, DIR_LONG>
, insn<sz_v, STR("move16"), OP<0xf628,      m68040>, FMT_X_0P,   DIR_LONG, POST_INCR>
, insn<sz_v, STR("move16"), OP<0xf638,      m68040>, FMT_X_0S,   DIR_LONG, ADDR_INDIR>
    >;


#undef STR

using m68k_gen_v = list<list<>
                  , m68k_move16_v
              >;
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_040> : gen_040::m68k_gen_v {};
}

#endif
