#ifndef KAS_M68K_M68K_MCODE_H
#define KAS_M68K_M68K_MCODE_H

#include "m68k_stmt.h"
#include "m68k_size_defn.h"
#include "target/tgt_mcode.h"

#include "kas_core/core_emit.h"
#include "kas_core/core_fits.h"

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::m68k
{


struct m68k_mcode_t;

// override defaults for various sizes
struct m68k_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 6;
    using mcode_size_t = uint16_t;
    using mcode_idx_t  = uint16_t; 
    using name_idx_t   = uint16_t;
    using defn_idx_t   = uint16_t;
    using tgt_size_t   = m68k::opc::m68k_size_t;
};


struct m68k_mcode_t : tgt::tgt_mcode_t<m68k_mcode_t, m68k_stmt_t, error_msg, m68k_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("M68K");

    // use default ctors
    using base_t::base_t;

};


}
#endif

