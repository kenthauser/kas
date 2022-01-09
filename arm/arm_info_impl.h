#ifndef KAS_ARM_ARM_INFO_IMPL_H
#define KAS_ARM_ARM_INFO_IMPL_H

#include "target/tgt_info_fn.h"
#include "arm_stmt_ual.h"
#include "arm_mcode.h"

namespace kas::arm::opc
{

struct arm_info_fn_base : arm_mcode_t::info_fn_t
{
    using base_t = typename arm_mcode_t::info_fn_t;
    using code_t = typename base_t::code_t;
    using arm_stmt_info_t = parser::arm_stmt_info_t;
    using arm_sfx_t       = parser::arm_sfx_t;

    // CCODE: 4 bits shifted 28
    void insert_ccode(code_t& code, stmt_info_t const& stmt_info) const
    {
        auto ccode = stmt_info.ccode;
        if (ccode == parser::arm_stmt_info_t::ARM_CC_OMIT)
            ccode = parser::arm_stmt_info_t::ARM_CC_ALL;

        code[0] |= ccode << (28 - 16);
    }
    
    void extract_ccode(mcode_size_t const *code_p, stmt_info_t& info) const
    {
        auto ccode = code_p[0] >> (28 - 16);
        if (ccode == parser::arm_stmt_info_t::ARM_CC_ALL)
            ccode = parser::arm_stmt_info_t::ARM_CC_OMIT;
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

#if 0
    // SFX: B_FLAG: 1 bit shifted 22
    void insert_bflag(code_t& code, stmt_info_t const& stmt_info) const
    {
        auto sfx_p = arm_sfx_t::get_p(stmt_info.sfx_index);
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
        auto sfx_p = arm_sfx_t::get_p(info.sfx_index);

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
#endif
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

struct arm_info_a32_c : arm_info_fn_base
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

struct arm_info_a32_cs : arm_info_fn_base
{
    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        return; // XXX
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

// insert upper word & lower word flags
// flags are inserted into to opcode via XOR (see toggling of L-Bit in SFX_H)
struct arm_info_a32_c_sfx : arm_info_fn_base
{
    bool is_ld(mcode_size_t const *code_p) const
    {
        // bit 20 is 1 for LOAD, 0 for STORE
        return code_p[0] & (1 << (20-16));
    }

    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        insert_ccode(code, stmt_info);
       
        // deal with suffix codes if present
        if (auto sfx_p = arm_sfx_t::get_p(stmt_info.sfx_index))
        {
            auto [msw, lsw] = table_to_code(is_ld(code.begin()), sfx_p);
            if (msw)
                code[0] ^= msw;
            if (lsw)
                code[1] ^= lsw;
        }
    }
    

    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        stmt_info_t info;
        extract_ccode(code_p, info);

        // if `mcode` requires suffix, extract it
        auto sfx_code = (defn_info.flags & SZ_DEFN_SFX_MASK) >> SZ_DEFN_SFX_SHIFT;
        bool sfx_req  = !!(defn_info.flags & SZ_DEFN_SFX_REQ);
        auto value = code_to_table(code_p, sfx_code);

        // value == zero -> no flags inserted
        // however, "required" suffix can have zero code
        if (value || sfx_req)
        {
            auto is_ldr = is_ld(code_p);
            for (auto& sfx : arm_sfx_t::data())
            {
                if (sfx.ldr != value && sfx.str != value)
                    continue;
                info.sfx_index = sfx.index();
                break;
            }

            // if (req_sfx)...error
        }
        return info;
    }
    
    // *** support routines ***

    // generate "upper" & "lower" values to be xor'd into ccode[]
    // use `sfx_p` table value to generate values
    auto table_to_code(bool l_bit, arm_sfx_t const *sfx_p) const
        -> std::pair<uint16_t, uint16_t>  
    {
        auto value = l_bit ? sfx_p->ldr : sfx_p->str;
        uint16_t msw{}, lsw{};

        switch (sfx_p->type)
        {
            case parser::SFX_B:
            case parser::SFX_T:
            case parser::SFX_M:
                msw = value << 4;
                break;

            case parser::SFX_H:
                msw = (value & 1) << 4;     // get L-bit
                lsw = (value & 0x60);
                break;
            default:
                break;      // unsupported suffix
        }
        return {msw, lsw};
    }

    // generate `sfx_p` table value from code for suffix type
    auto code_to_table(mcode_size_t const *code_p, unsigned type) const
        -> decltype(arm_sfx_t::ldr)
    {
        // declare result value
        decltype (arm_sfx_t::ldr) value{};

        // get msw word flags (bits 24-20) shifted 4 for all suffixes
        // P_FLAG (bit 24), U_FLAG (23), B_FLAG (22) W_FLAG (21), L_FLAG (20)
        auto ms_flags = (code_p[0] >> (20-16)) & 0x1f;

        // interesting flags depend on `arm_sfx_enum`
        switch (type)
        {
            case parser::SFX_B:
                return ms_flags & 4;    // just return `B` bit
            case parser::SFX_T:
                // validate P_FLAG == 0 && W_FLAG == 1
                if (ms_flags & 0x10)
                    break;
                return ms_flags & 6;
            case parser::SFX_M:
                break;
            case parser::SFX_H:
            {
                return (code_p[1] & 0x60) ^ (ms_flags & 1);
            }
                break;
            default:
                break;
        }
        return {};
    }
};



}

#endif
