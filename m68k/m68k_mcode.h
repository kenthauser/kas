#ifndef KAS_M68K_M68K_MCODE_H
#define KAS_M68K_M68K_MCODE_H

#include "m68k_stmt.h"
#include "target/tgt_mcode.h"

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::m68k
{


// all m68k instructions have a specified "size" of operand
// enumerate them (align values with m68881 source register values)
enum m68k_op_size_t
{
      OP_SIZE_LONG      // 0
    , OP_SIZE_SINGLE    // 1
    , OP_SIZE_XTND      // 2
    , OP_SIZE_PACKED    // 3
    , OP_SIZE_WORD      // 4
    , OP_SIZE_DOUBLE    // 5
    , OP_SIZE_BYTE      // 6
    , OP_SIZE_VOID      // 7 NOT ACTUAL OP_SIZE
    , NUM_OP_SIZE
};

// declare "standard" co-processor ID values
enum m68k_cpid : int8_t
{
      M68K_CPID_MMU         // 0 = Memory management unit
    , M68K_CPID_FPU         // 1 = Floating point unit
    , M68K_CPID_MMU_040     // 2 = '040 MMU extensions
    , M68K_CPID_MOVE16      // 3 = move16 instructions
    , M68K_CPID_TABLE       // 4 = CPU_32/Coldfire table instructions
    , M68K_CPID_DEBUG = 15  // 15 = Debug instructions
};



// override defaults for various defn index sizes
struct m68k_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 6;
    using mcode_size_t = uint16_t;
    using mcode_idx_t  = uint16_t; 
    using name_idx_t   = uint16_t;
    using defn_idx_t   = uint16_t;
    using defn_info_t  = uint16_t;
};

// forward declare m68k default mcode arg formatter
// forward declare m68k code size_fn
namespace opc
{
    struct FMT_X;
    struct m68k_insn_lwb;
}

struct m68k_mcode_t : tgt::tgt_mcode_t<m68k_mcode_t, m68k_stmt_t, error_msg, m68k_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("M68K");

    // use default ctors
    using base_t::base_t;

    // override default types
    using fmt_default = opc::FMT_X;
    using code_size_t = opc::m68k_insn_lwb;

};


}
#endif

