#ifndef KAS_M68K_ERROR_MESSAGES_H
#define KAS_M68K_ERROR_MESSAGES_H

#include "expr/error_messages_base.h"

namespace kas::m68k
{

struct error_msg : expression::error_msg
{
    static constexpr auto ERR_invalid       = "X invalid arguments";
    static constexpr auto ERR_argument      = "X invalid argument";
    static constexpr auto ERR_missing       = "X requires argument(s)";
    static constexpr auto ERR_too_few       = "X too few arguments";
    static constexpr auto ERR_too_many      = "X too many arguments";
    static constexpr auto ERR_bad_size      = "X invalid argument size";
    static constexpr auto ERR_float         = "X floating point argument not allowed";

    static constexpr auto ERR_flt_inf       = "X no fixed point conversion for INF value";
    static constexpr auto ERR_flt_nan       = "X no fixed point conversion for NAN value";
    static constexpr auto ERR_flt_ovf       = "X floating point value exceeds maximum value";
    static constexpr auto ERR_flt_fixed     = "X floating point used for fixed immediate";

    static constexpr auto ERR_sfx_reqd      = "X must spcecify argument size";
    static constexpr auto ERR_sfx_none      = "X size suffix prohibited";
    static constexpr auto ERR_ccode_reqd    = "X condition code expected";
    static constexpr auto ERR_no_ccode      = "X condition code not allowed";
    static constexpr auto ERR_no_cc_tf      = "X condition codes T/F not allowed";
    static constexpr auto ERR_addr_mode     = "X invalid addr_mode";
    
    static constexpr auto ERR_regset        = "X invalid register set";
    static constexpr auto ERR_direct        = "X invalid direct";
    static constexpr auto ERR_indirect      = "X invalid indirect";
    static constexpr auto ERR_immediate     = "X invalid immediate";
};


}

#endif

