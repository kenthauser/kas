#ifndef KAS_M68K_M68K_MCODE_IMPL_H
#define KAS_M68K_M68K_MCODE_IMPL_H

#include "m68k_mcode.h"
#include "m68k_size_lwb.h"

namespace kas::m68k
{

// determine size of immediate arg
uint8_t m68k_mcode_t::sz(stmt_info_t info) const
{
    if (info.arg_size != OP_SIZE_VOID)
        return info.arg_size;

    // if void, check if single size specified
    auto defn_sz = defn().info & 0x7f;

    // don't bother with switch
    if (defn_sz == (1 << OP_SIZE_LONG))
        return OP_SIZE_LONG;
    else if (defn_sz == (1 << OP_SIZE_WORD))
        return OP_SIZE_WORD;
    else if (defn_sz == (1 << OP_SIZE_BYTE))
        return OP_SIZE_BYTE;

    return info.arg_size;
}

using LWB_SIZES = kas::parser::init_from_list<opc::m68k_insn_lwb, opc::LWB_SIZE_LIST>;

auto m68k_mcode_t::code(stmt_info_t info) const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    // put code into array
    auto code = base_t::code(info);
   
    // possibly map `sz` to integral size
    auto arg_sz = sz(info);

    // if size "supported", insert bits into code word
    // also don't insert `void` if only `void supported
    auto defn_info = defn().info;
    if ((defn_info & 0xff) != 0x80)
        if (defn_info & (1 << arg_sz))
        {
            auto& sz_fn = LWB_SIZES::value[defn_info >> 12];
            code[sz_fn.word()] |= sz_fn(arg_sz);
        }
    
    //std::cout << "m68k_mcode_t::code: cc = " << info.has_ccode << " ccode = " << info.ccode;
    //std::cout << " defn = " << std::hex << defn_info << std::endl;

    // insert codition code if required
    // NB: values from `m68k_insn_common.h`
    switch (defn_info & opc::SFX_IS_CC)
    {
        default:
        case 0:
            break;
        case 0x400:     // standard 68k condition code
            code[0] |= info.ccode << 8;
            break;
        case 0x800:     // floating point code
            code[0] |= info.ccode;
            break;
        case 0xc00:     // LIST format
         {
            if (!info.has_ccode)
                break;
            auto ccode = info.ccode;
            if (info.fp_ccode)
                ccode |= 0x20;
            else
                ccode |= 0x10;
            code[0] |= ccode << 6;
            break;
         }
    }

    return code;
}

auto m68k_mcode_t::extract_info(mcode_size_t const *code_p) const -> stmt_info_t
{
    stmt_info_t info;       // build return value
   
    // calculate SZ
    auto defn_info = defn().info;
    auto& sz_fn = LWB_SIZES::value[defn_info >> 12];     // 4 MSBs
    info.arg_size = sz_fn.extract(code_p);

    // calculate Condition Codes
    switch (defn_info & opc::SFX_IS_CC)
    {
        default:
        case 0:
            break;
        case 0x400:
            info.has_ccode = true;
            info.ccode = (code_p[0] >> 8) & 0xf;
            break;
        case 0x800:
            info.has_ccode = true;
            info.fp_ccode  = true;
            info.ccode     = code_p[0] & 0x1f;
            break;
        case 0xc00:
        {
            // raw format for LIST
            auto ccode = code_p[0] >> 6;
            if (ccode & 0x20)
                info.fp_ccode = true;
            else if (ccode & 0x10)
                ccode &= 0xf;
            else
                break;

            info.has_ccode = true;
            info.ccode = ccode & 0x1f;      // NB: mask not needed. Dest is 5 bits
            break;
        }
    }
    return info;
}

}

#endif
