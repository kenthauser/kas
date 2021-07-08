#ifndef KAS_ARM_ARM_STMT_FLAGS_H
#define KAS_ARM_ARM_STMT_FLAGS_H

namespace kas::arm
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
enum arm_sfx_enum : uint8_t { SFX_NONE, SFX_B, SFX_T, SFX_H, SFX_M };

struct arm_sfx_t
{
    // default constexpr ctor required (constexpr ctors are all or nothing)
    constexpr arm_sfx_t() {}
    constexpr arm_sfx_t(const char *name
                      , arm_sfx_enum type
                      , uint8_t ldr = {}
                      , uint8_t str = {}) 
        : name{*name}, type{type}, ldr{ldr}, str{str} {}

    // put values in an array to allow conversion to index
    // code values depend if load or store instruction
    static arm_sfx_t const * const data()
    {
        static constexpr arm_sfx_t data[] = 
        {
        // NB: `symbols<>` requires lower case data for `no_case`
        // addressing Mode 3: miscellaneous loads and stores
        // ldr/str signed/unsigned byte/halfword/doubleword
           {"h"     , SFX_H, 0x20, 0x20 }
         , {"sh"    , SFX_H, 0x60, 0x60 }
         , {"sb"    , SFX_H, 0x40, 0x40 }
         , {"d"     , SFX_H, 0x40, 0x60 }

        // ldr/str unsigned byte
          , {"b"    , SFX_B }

        // ldr/str unprivileged
         , {"t"     , SFX_T, 8, 8 }
         , {"bt"    , SFX_T, 9, 9 }

        // load/store multiple
         , {"ia"    , SFX_M, 4, 4 }
         , {"ib"    , SFX_M, 5, 5 }
         , {"da"    , SFX_M, 6, 6 }
         , {"db"    , SFX_M, 7, 7 }

        // stack ops alternate name
         , {"fd"    , SFX_M, 4, 4 }
         , {"ed"    , SFX_M, 5, 5 }
         , {"fa"    , SFX_M, 6, 6 }
         , {"ea"    , SFX_M, 7, 7 }
         , {}                           // flag END
        };
        return data;
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

    const char      name[3] = {};
    arm_sfx_enum    type {};
    uint8_t         ldr  {};
    uint8_t         str  {};
};

// parse as pointer to facilitate storing index in `stmt_info`
struct arm_suffix : x3::symbols<arm_sfx_t const *>
{
    arm_suffix()
    {
        for (auto p = arm_sfx_t::data(); p->name[0]; ++p)
            add(p->name, p);
    }

    static auto x3()
    {
        return x3::no_case[arm_suffix()];
    }
};




}

#endif
