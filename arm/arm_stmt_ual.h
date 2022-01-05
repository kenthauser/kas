#ifndef KAS_ARM_ARM_STMT_UAL_H
#define KAS_ARM_ARM_STMT_UAL_H

#include "kas/kas_string.h"

namespace kas::arm::parser
{

// define ARM condition codes
struct arm_ccode_t

{
    arm_ccode_t();       // default ctor required for X3

    constexpr arm_ccode_t(const char *ccode, uint8_t value)
            : ccode{ccode[0], ccode[1]}, value(value) {}

    static auto& data()
    {
        // optimize for addition to x3() parser 
        static constexpr arm_ccode_t data[] = 
            {
                {"eq",  0}
              , {"ne",  1}
              , {"cs",  2}
              , {"hs",  2}      // alt name
              , {"cc",  3}
              , {"lo",  3}      // alt name
              , {"mi",  4}
              , {"pl",  5}
              , {"vs",  6}
              , {"vc",  7}
              , {"hi",  8}
              , {"ls",  9}
              , {"ge", 10}
              , {"lt", 11}
              , {"gt", 12}
              , {"le", 13}
              , {"al", 14}      // rejected if `NO_AL` bit set
            };
        return data;
    }

    static const char *name(uint8_t value)
    {
        // for lookup, use brute force
        for (auto& e : data())
            if (e.value == value)
                return e.ccode;
        return "??";
    }

    // make each ccode a 32-bit value...
    char    ccode[3];
    uint8_t value;
};

// parse arm opcode suffix (after ccode)
// NB: put values in array to allow conversion to index for `stmt_info`

// define "type" to match against types in `arm_insn_common`
// XXX should pick up from `arm_mcode.h`. 
enum arm_sfx_enum : uint8_t { SFX_NONE, SFX_B, SFX_T, SFX_H, SFX_M, SFX_L };


// defin ARM instruction suffixes
struct arm_sfx_t
{
    // default constexpr ctor required (constexpr ctors are all or nothing)
    constexpr arm_sfx_t() {}

    constexpr arm_sfx_t(const char *name
                      , uint8_t arch
                      , arm_sfx_enum type
                      , uint8_t ldr
                      , uint8_t str
                      , uint8_t size = OP_SIZE_WORD) 
        : name{name[0], name[1]}
        , arch{arch}, type{type}, ldr{ldr}, str{str}, size(size) {}
    // put values in an array to allow conversion to index
    // code values depend if load or store instruction
    static auto& data()
    {
        static_assert(sizeof(arm_sfx_t) == 8
                    , "sizeof(arm_sfx_t) != single word");
    // Need const char arrays -- use KAS_STRING        
//#define STR(x) KAS_STRING(x)::value
#define STR(x) x
        static constexpr arm_sfx_t data[] = 
        {
        // NB: `symbols<>` requires lower case data for `no_case`
        // addressing Mode 3: miscellaneous loads and stores
        // ldr/str signed/unsigned byte/halfword/doubleword
        // NB: value of zero indicates invalid instruction
        // NB: LDRSHT is a ARMV6 insn, not supported by pre-UAL
           {STR("h")     , SZ_ARCH_ARM, SFX_H, 0xb0, 0xb0, OP_SIZE_HALF   }
         , {STR("sh")    , SZ_ARCH_ARM, SFX_H, 0xf0, 0x00, OP_SIZE_SHALF  }
         , {STR("sb")    , SZ_ARCH_ARM, SFX_H, 0xd0, 0x00, OP_SIZE_SBYTE  }
         , {STR("d")     , SZ_ARCH_ARM, SFX_H, 0xd1, 0xf0, OP_SIZE_DOUBLE }

        // ldr/str unsigned byte (set B-FLAG: bit 22)
          , {STR("b")    , SZ_ARCH_ARM, SFX_B, 4, 4, OP_SIZE_BYTE }

        // ldr/str unprivileged (set W-FLAG: bit 21) + B_FLAG 
         , {STR("t")     , SZ_ARCH_ARM, SFX_T, 2, 2 }
         , {STR("bt")    , SZ_ARCH_ARM, SFX_T, 6, 6, OP_SIZE_BYTE }

        // load/store multiple (set L-bit(20) / P-bit(24) / U-bit(23) )
        // store values shifted 20 bits
        // NB: i/d indicates increment/decrement, a/b indicates after/before
         , {STR("ia")    , SZ_ARCH_ARM, SFX_M, 0x01, 0x08 }    // 1/0/1    0/0/1
         , {STR("ib")    , SZ_ARCH_ARM, SFX_M, 0x19, 0x18 }    // 1/1/1    0/1/1
         , {STR("da")    , SZ_ARCH_ARM, SFX_M, 0x01, 0x00 }    // 1/0/0    0/0/0
         , {STR("db")    , SZ_ARCH_ARM, SFX_M, 0x11, 0x10 }    // 1/1/0    0/1/0

        // stack ops alternate name
        // NB: f/e indicates full/empty, d/a incicates descending/ascending
         , {STR("fd")    , SZ_ARCH_ARM, SFX_M, 0x01, 0x10 }    // 1/0/1    0/1/0
         , {STR("ed")    , SZ_ARCH_ARM, SFX_M, 0x19, 0x00 }    // 1/1/1    0/0/0
         , {STR("fa")    , SZ_ARCH_ARM, SFX_M, 0x01, 0x18 }    // 1/0/0    0/1/1
         , {STR("ea")    , SZ_ARCH_ARM, SFX_M, 0x11, 0x08 }    // 1/1/0    0/0/1

        // coprocessor suffix L
         , {STR("l")    , SZ_ARCH_ARM, SFX_L, 0x0, 0x0 }
        };
        return data;
#undef STR
    }
  
    // generate a 1-based index
    uint8_t index() const 
    {
        return (this - data()) + 1;
    }

    // convert a 1-based index;
    static arm_sfx_t const *get_p(uint8_t index)
    {
        if (index)
            return &data()[index-1];
        return {};
    }

    const char      name[3] {};     // two character suffix, pls null
    uint8_t         arch {};        // defn for arch
    arm_sfx_enum    type {};        // defn requirement
    uint8_t         ldr  {};
    uint8_t         str  {};
    uint8_t         size { OP_SIZE_WORD };
};

// expose defns as parsers
struct arm_ccode : x3::symbols<uint8_t>
{
    arm_ccode()
    {
        for (auto& p : arm_ccode_t::data())
            add(p.ccode, p.value);
    }

    static auto x3()
    {
        return x3::no_case[arm_ccode()];
    }

    static const char *name(uint8_t value) { return arm_ccode_t::name(value); }
};

// parse as pointer to facilitate storing index in `stmt_info`
struct arm_suffix : x3::symbols<arm_sfx_t const *>
{
    arm_suffix()
    {
        for (auto& p : arm_sfx_t::data())
            add(p.name, &p);
    }

    static auto x3()
    {
        return x3::no_case[arm_suffix()];
    }
};




}

#endif
