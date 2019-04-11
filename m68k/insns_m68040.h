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
, defn<sz_v, STR("move16"), OP<0xf620'8000, m68040>, FMT_0_28, POST_INCR, POST_INCR>
, defn<sz_v, STR("move16"), OP<0xf620,      m68040>, FMT_0,    POST_INCR, DIR_LONG>
, defn<sz_v, STR("move16"), OP<0xf630,      m68040>, FMT_0,    ADDR_INDIR, DIR_LONG>
, defn<sz_v, STR("move16"), OP<0xf628,      m68040>, FMT_X_0,  DIR_LONG, POST_INCR>
, defn<sz_v, STR("move16"), OP<0xf638,      m68040>, FMT_X_0,  DIR_LONG, ADDR_INDIR>
    >;


#undef STR

using m68k_gen_v = list<list<>
                  , m68k_move16_v
              >;
}

namespace kas::m68k::opc
{
    template <> struct m68k_defn_list<OP_M68K_040> : gen_040::m68k_gen_v {};
}

#endif
