#ifndef KAS_Z80_ERROR_MESSAGES_H
#define KAS_Z80_ERROR_MESSAGES_H

namespace kas::z80
{

struct error_msg
{
    static constexpr auto ERR_invalid       = "invalid arguments";
    static constexpr auto ERR_argument      = "invalid argument";
    static constexpr auto ERR_missing       = "missing argument";
    static constexpr auto ERR_addr_mode     = "invalid addr_mode";
    static constexpr auto ERR_addr_reg_byte = "byte operation on address register";
    static constexpr auto ERR_m68020_addr   = "m68020 addr";
    static constexpr auto ERR_regset        = "invalid regset";
    static constexpr auto ERR_direct        = "invalid direct";
    static constexpr auto ERR_subreg        = "invalid subregister access";
    static constexpr auto ERR_indirect      = "invalid indirect";
    static constexpr auto ERR_immediate     = "invalid immediate";
    static constexpr auto ERR_no_addr_reg   = "addr reg required";
    static constexpr auto ERR_bad_pair      = "invalid pair";
    static constexpr auto ERR_bad_bitfield  = "invalid bitfield";
    static constexpr auto ERR_bad_offset    = "invalid offset";
    static constexpr auto ERR_bad_width     = "invalid width";
};


}

#endif
