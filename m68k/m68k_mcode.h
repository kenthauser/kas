#ifndef KAS_M68K_M68K_MCODE_H
#define KAS_M68K_M68K_MCODE_H

#include "m68k_stmt.h"
#include "target/tgt_mcode.h"

#include "kas_core/core_emit.h"
#include "kas_core/core_fits.h"

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::m68k
{

// override defaults for various sizes
struct m68k_mcode_size_t : tgt::tgt_mcode_size_t
{
    using mcode_size_t = uint16_t;
};


struct m68k_mcode_t : tgt::tgt_mcode_t<m68k_mcode_t, m68k_stmt_t, error_msg, m68k_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("M68K");

    // use default ctors
    using base_t::base_t;

};


}
#endif

