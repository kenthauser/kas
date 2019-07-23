#ifndef KAS_M68K_M68K_MCODE_IMPL_H
#define KAS_M68K_M68K_MCODE_IMPL_H

#include "m68k_mcode.h"
#include "m68k_size_lwb.h"

namespace kas::m68k
{

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
    return code;
}

uint8_t m68k_mcode_t::extract_sz(mcode_size_t const *code_p) const
{
    auto& sz_fn = LWB_SIZES::value[defn().info >> 12];
    auto sz = sz_fn.extract(code_p);
    std::cout << "m68k_mcode_t::extract_sz: sz = " << m68k_sfx::suffixes[sz] << std::endl;
    return sz_fn.extract(code_p);
}

}

#endif
