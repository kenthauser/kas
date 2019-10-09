#ifndef KAS_EXPR_ERROR_MESSAGES_BASE_H
#define KAS_EXPR_ERROR_MESSAGES_BASE_H

namespace kas::expression
{

struct error_msg
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
    static constexpr auto ERR_addr_reg_byte = "X byte operation on address register";
    static constexpr auto ERR_m68020_addr   = "X m68020 addr";
    static constexpr auto ERR_regset        = "X invalid regster set";
    static constexpr auto ERR_direct        = "X invalid direct";
    static constexpr auto ERR_subreg        = "X invalid subregister access";
    static constexpr auto ERR_indirect      = "X invalid indirect";
    static constexpr auto ERR_immediate     = "X invalid immediate";
    static constexpr auto ERR_no_addr_reg   = "X addr reg required";
    static constexpr auto ERR_bad_pair      = "X invalid pair";
    static constexpr auto ERR_bad_bitfield  = "X invalid bitfield";
    static constexpr auto ERR_bad_offset    = "X invalid offset";
    static constexpr auto ERR_bad_width     = "X invalid width";
};


}

#endif

