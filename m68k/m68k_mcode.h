#ifndef KAS_M68K_M68K_MCODE_H
#define KAS_M68K_M68K_MCODE_H

#include "m68k_stmt.h"
#include "target/tgt_mcode.h"
//#include "parser/init_from_list.h"

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

    // XXX Possible mapping of OP_SIZE_QUAD...
    // NB: MMU `pmovew` can have an immediate value of quad. 
    // NB: use IMMED size of `OP_SIZE_VOID` to emit quad.
    , OP_SIZE_QUAD = OP_SIZE_VOID
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
    struct FMT_X;               // default opcode formatter
    struct m68k_insn_lwb;       // support complicated m68k opcode size methods
    struct m68k_opc_base;       // includes `cf_limit_3w` method
}

//struct m68k_val_t;      // forward declare `coldfire` support type
struct m68k_mcode_t : tgt::tgt_mcode_t<m68k_mcode_t, m68k_stmt_t, error_msg, m68k_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("M68K");

    // use default ctors
    using base_t::base_t;

    // override default types
    //using val_t       = m68k_val_t;
    using fmt_default = opc::FMT_X;
    using code_size_t = opc::m68k_insn_lwb;
    using opcode_t    = opc::m68k_opc_base;
    using op_size_t   = core::opcode::op_size_t;

    uint8_t     sz(stmt_info_t info) const;
    auto        code(stmt_info_t info) const -> decltype(base_t::code(info));
    stmt_info_t extract_info(mcode_size_t const *) const;

#if 1
};
#else
    // coldfire support (limit insns to 3 words)
    op_size_t& cf_limit_3w(core::opcode_data& data) const;
};

auto m68k_mcode_t::cf_limit_3w(core::opcode_data& data) const
    -> op_size_t&
{
        std::cout << "m68k_mcode_t::cf_limit_3w" << std::endl;
#if 0
        if (!hw::cpu_defs[hw::limit_3w{}])
        {
            std::cout << "m68k_val_t:limit_3w set" << std::endl;
            if (op_size.min > 6)
            {
                std::cout << "m68k_val_t::cf_limit_3w: limit_3w fails" << std::endl;
                //op_size.set_error();
                //return fits_result::NO_FIT;
            }
        }
#endif
        return data.size;
}
struct m68k_val_t : tgt::opc::tgt_validate<m68k_mcode_t>
{
    using base_t      = tgt_validate;
    using mcode_t     = typename base_t::mcode_t;
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;


    fits_result cf_limit_3w(fits_result result, op_size_t& op_size) const
    { 
        std::cout << "m68k_val_t::cf_limit_3w: size = " << op_size << std::endl;

        if (!hw::cpu_defs[hw::limit_3w{}])
        {
            std::cout << "m68k_val_t:limit_3w set" << std::endl;
            if (op_size.min > 6)
            {
                std::cout << "m68k_val_t::cf_limit_3w: limit_3w fails" << std::endl;
                //op_size.set_error();
                //return fits_result::NO_FIT;
            }
        }
        return result;
    }

    
};
#endif
}
#endif

