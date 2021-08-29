#ifndef KAS_ARM_ARM_STMT_FLAGS_H
#define KAS_ARM_ARM_STMT_FLAGS_H

#include "kas/kas_string.h"

namespace kas::arm::parser
{

// parse condition codes
struct arm_ccode : x3::symbols<uint8_t>
{
    // put values in array to allow easy conversion from index -> string
    static constexpr std::pair<const char *, uint8_t> data[] = {
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

    arm_ccode()
    {
        for (auto& cc : data)
            add(cc.first, cc.second);
    }

    static auto x3() 
    {
        return x3::no_case[arm_ccode()];
    }

    static const char *name(uint8_t code)
    {
        // NB: Multiple ccodes map to same value. Create local table;
        static std::array<const char *, 16> names;
        
        // if first time, initalize map
        if (!names[0])
            for (auto& p : data)
                if (!names[p.second])
                    names[p.second] = p.first;

        if (code < names.size())
            return names[code];
        return "??";
    }
};

// parse arm opcode suffix (after ccode)
// NB: put values in array to allow conversion to index for `stmt_info`

// define "type" to match against types in `arm_insn_common`
enum arm_sfx_enum : uint8_t { SFX_NONE, SFX_B, SFX_T, SFX_H, SFX_M, SFX_L };

struct arm_sfx_t
{
#if 0
    // default constexpr ctor required (constexpr ctors are all or nothing)
    constexpr arm_sfx_t() {}

    template <typename NAME>
    constexpr arm_sfx_t(NAME name
                      , arm_sfx_enum type
                      , uint8_t ldr = {}
                      , uint8_t str = {}) 
        : name{name}, type{type}, ldr{ldr}, str{str} {}
#endif
    // put values in an array to allow conversion to index
    // code values depend if load or store instruction
    static auto& data()
    {
    // Need const char arrays -- use KAS_STRING        
#define STR(x) KAS_STRING(x)::value
//#define STR(x) x
        static constexpr arm_sfx_t data[] = 
        {
        // NB: `symbols<>` requires lower case data for `no_case`
        // addressing Mode 3: miscellaneous loads and stores
        // ldr/str signed/unsigned byte/halfword/doubleword
        // NB: value of zero indicates invalid instruction
           {STR("h")     , SFX_H, 0xb0, 0xb0, OP_SIZE_HALF  }
         , {STR("sh")    , SFX_H, 0xf0, 0x00, OP_SIZE_SHALF }
         , {STR("sb")    , SFX_H, 0xd0, 0x00, OP_SIZE_SBYTE }
         , {STR("d")     , SFX_H, 0xd1, 0xf0, OP_SIZE_QUAD  }

        // ldr/str unsigned byte (set B-FLAG: bit 22)
          , {STR("b")    , SFX_B, 4, 4, OP_SIZE_BYTE }

        // ldr/str unprivileged (set W-FLAG: bit 21) + B_FLAG 
         , {STR("t")     , SFX_T, 2, 2 }
         , {STR("bt")    , SFX_T, 6, 6, OP_SIZE_BYTE }

        // load/store multiple (set L-bit(20) / P-bit(24) /U-bit(23) )
        // store values shifted 4 bits
         , {STR("ia")    , SFX_M, 0x01, 0x08 }    // 1/0/1    0/0/1
         , {STR("ib")    , SFX_M, 0x19, 0x18 }    // 1/1/1    0/1/1
         , {STR("da")    , SFX_M, 0x01, 0x00 }    // 1/0/0    0/0/0
         , {STR("db")    , SFX_M, 0x11, 0x10 }    // 1/1/0    0/1/0

        // stack ops alternate name
         , {STR("fd")    , SFX_M, 0x01, 0x10 }    // 1/0/1    0/1/0
         , {STR("ed")    , SFX_M, 0x19, 0x00 }    // 1/1/1    0/0/0
         , {STR("fa")    , SFX_M, 0x01, 0x18 }    // 1/0/0    0/1/1
         , {STR("ea")    , SFX_M, 0x11, 0x08 }    // 1/1/0    0/0/1

        // coprocessor suffix L
         , {STR("l")    , SFX_L, 0x0, 0x0 }
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

//    const char      name[4] {};
   const char *name {};
    arm_sfx_enum    type {};
    uint8_t         ldr  {};
    uint8_t         str  {};
    uint8_t         size { OP_SIZE_WORD };
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
