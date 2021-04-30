#ifndef KAS_PROTO_UC_PROTO_UC_MCODE_H
#define KAS_PROTO_UC_PROTO_UC_MCODE_H

#include "PROTO_LC_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::PROTO_LC
{
// all PROTO_LC instructions have a specified "size" of operand
// enumerate them (NB: align with processor definitions if possible)
enum PROTO_LC_op_size_t
{
      OP_SIZE_WORD      // 0
    , OP_SIZE_BYTE      // 1
    , NUM_OP_SIZE
};

#if 0
// EXAMPLE: override defaults for various defn index sizes
struct PROTO_LC_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 6;
    using mcode_size_t = uint16_t;
    using mcode_idx_t  = uint16_t; 
    using name_idx_t   = uint16_t;
    using defn_idx_t   = uint16_t;
    using defn_info_t  = uint16_t;
};
#endif

struct PROTO_LC_mcode_t : tgt::tgt_mcode_t<PROTO_LC_mcode_t, PROTO_LC_stmt_t, error_msg
                                    // EXAMPLE: , PROTO_LC_mcode_size_t
                                    >
{
    using BASE_NAME = KAS_STRING("PROTO_UC");
    //using mcode_t   = PROTO_LC_mcode_t;

    // use default ctors
    using base_t::base_t;

    // EXAMPLE: define override to CRTP base class methods

};

}
#endif

