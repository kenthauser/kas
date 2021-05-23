#ifndef KAS_ARM_ARM_STMT_FLAGS_H
#define KAS_ARM_ARM_STMT_FLAGS_H

namespace kas::arm
{

// parse arm opcode suffix (after ccode)
enum arm_sfx_enum : uint8_t { SFX_NONE, SFX_B, SFX_T, SFX_H, SFX_M };

// value is: SFX_ENUM, LD_VALUE, ST_VALUE (sizeof = 32 bits)
struct arm_sfx_t : alignas_t<arm_sfx_t, uint32_t>
{
    using base_t::base_t;

    arm_sfx_t(arm_sfx_enum type, uint8_t ldr, uint8_t str) 
        : type(type), ldr(ldr), str(str) {}

    arm_sfx_enum    type;
    uint8_t         ldr;
    uint8_t         str;
};

struct arm_suffix : x3::symbols<arm_sfx_t>
{
    arm_suffix()
    {
        // NB: `symbols<>` requires lower case data for `no_case`
        add
        // ldr/str signed/unsigned byte/halfword/doubleword
           ("h"     ,  {SFX_H, 0, 0}) 
           ("sh"    ,  {SFX_H, 1, 1})
           ("sb"    ,  {SFX_H, 2, 2})
           ("d"     ,  {SFX_H, 3, 3})

        // ldr/str unsigned byte
            ("b"    , { SFX_B, 10, 10 })

        // ldr/str unprivileged
           ("t"     ,  {SFX_T, 8, 8})
           ("bt"    ,  {SFX_T, 9, 9})

        // load/store multiple
           ("ia"    ,  {SFX_M, 4, 4})
           ("ib"    ,  {SFX_M, 5, 5})
           ("da"    ,  {SFX_M, 6, 6})
           ("db"    ,  {SFX_M, 7, 7})

        // stack ops alternate name
           ("fd"    ,  {SFX_M, 4, 4})
           ("ed"    ,  {SFX_M, 5, 5})
           ("fa"    ,  {SFX_M, 6, 6})
           ("ea"    ,  {SFX_M, 7, 7})
           ;
    }

    static auto x3()
    {
        return x3::no_case[arm_suffix()];
    }
};



// parse condition codes
struct arm_ccode : x3::symbols<uint8_t>
{
    arm_ccode()
    {
        // NB: `symbols<>` requires lower case data for `no_case`
        add ("eq",  0)
            ("ne",  1)
            ("cs",  2)
            ("hs",  2)
            ("cc",  3)
            ("lo",  3)
            ("mi",  4)
            ("pl",  5)
            ("vs",  6)
            ("vc",  7)
            ("hi",  8)
            ("ls",  9)
            ("ge", 10)
            ("lt", 11)
            ("lt", 12)
            ("le", 13)
            ("al", 14)      // rejected if `NO_AL` bit set
            ;
    }

    static auto x3() 
    {
        return x3::no_case[arm_ccode()];
    }
};



}

#endif
