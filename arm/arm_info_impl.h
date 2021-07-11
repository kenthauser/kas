#ifndef KAS_ARM_ARM_INFO_IMPL_H
#define KAS_ARM_ARM_INFO_IMPL_H

#include "target/tgt_info_fn.h"
#include "arm_stmt_flags.h"
#include "arm_mcode.h"

namespace kas::arm::opc
{

struct arm_info_fn_base : arm_mcode_t::info_fn_t
{
    using base_t = typename arm_mcode_t::info_fn_t;
    using code_t = typename base_t::code_t;

    // CCODE: 4 bits shifted 28
    void insert_ccode(code_t& code, stmt_info_t const& stmt_info) const
    {
        auto ccode = stmt_info.ccode;
        if (ccode == arm_stmt_info_t::ARM_CC_OMIT)
            ccode = arm_stmt_info_t::ARM_CC_ALL;

        code[0] |= ccode << (28 - 16);
    }
    
    void extract_ccode(mcode_size_t const *code_p, stmt_info_t& info) const
    {
        auto ccode = code_p[0] >> (28 - 16);
        if (ccode == arm_stmt_info_t::ARM_CC_ALL)
            ccode = arm_stmt_info_t::ARM_CC_OMIT;
        info.ccode = ccode;
    }

    void mask_ccode(code_t& code) { code[0] |= 0xf << (28 - 16); }

    // SFX: S_FLAG : 1 bit shifted 20
    void insert_sflag(code_t& code, stmt_info_t const& stmt_info) const
    {
        if (stmt_info.has_sflag)
            code[0] |= 1 << (20 - 16);
    }

    void extract_sflag(mcode_size_t const *code_p, stmt_info_t& info) const
    {
        info.has_sflag = !!(code_p[0] & (1<<(20-16)));
    }
    
    void mask_sflag(code_t& code) { code[0] |= 1 << (20-16); }


    // SFX: B_FLAG: 1 bit shifted 22
    void insert_bflag(code_t& code, stmt_info_t const& stmt_info) const
    {
        auto sfx_p = arm_sfx_t::get_p(stmt_info.sfx_code);
        if (sfx_p && sfx_p->type == SFX_B)
            code[0] |= 1 << (22 - 16);
    }

    void extract_bflag(mcode_size_t const *code_p, stmt_info_t& info) const
    {
        // "find b-flag"
    }
    
    void mask_bflag(code_t& code) { code[0] |= 1 << (22 - 16); }
    
    // SFX_H: 2 bits shifted 5, plus L bit = 20,
    void insert_hflag(code_t& code, stmt_info_t const& info, bool l_flag) const
    {
        // get pointer to suffix code
        auto sfx_p = arm_sfx_t::get_p(info.sfx_code);

        // value based on LDR/STR instr code 
        auto value = l_flag ? sfx_p->ldr : sfx_p->str;
        
        // MSB goes to L-BIT (<< 20). Others or'd into LSByte
        if (value & 0x80)
            code[0] |= 1 << (20 - 16);

        code[0+1] |= value & 0x7f;
    }


    // SHF_T: P=0/W=1: P = 24, W = 21, B = 22
    // SFT_M: P/U/S/W = 24/23/22/21, NB: L=20
    // XXX S-bit handled as `arg mode`, L as base value only PUW as info

};

struct arm_info_list : arm_info_fn_base
{
    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        // XXX need so generalized "set code value" method...
        code[0] = stmt_info.value();// << 16;
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        return code_p[0];// >> 16;
    }
};

struct arm_info_a7_c : arm_info_fn_base
{
    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        insert_ccode(code, stmt_info);
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        stmt_info_t info;
        extract_ccode(code_p, info);
        return info;
    }

};

struct arm_info_a7_cs : arm_info_fn_base
{
    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        insert_ccode(code, stmt_info);
        insert_sflag(code, stmt_info);
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        stmt_info_t info;
        extract_ccode(code_p, info);
        extract_sflag(code_p, info);
        return info;
    }
};

}

#endif
