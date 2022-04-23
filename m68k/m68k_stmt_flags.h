#ifndef KAS_M68K_M68K_STMT_FLAGS_H
#define KAS_M68K_M68K_STMT_FLAGS_H

namespace kas::m68k
{
// 
// declare "condition code" parser for instructions
//

struct m68k_sfx
{
    // parse opereration size "suffix"
    // suffixes (all single byte)
    static constexpr const char *suffixes   = "lsxpwdbv"; 
    static constexpr const char *parse_sfxs = "lsxpwdb"; 

    static auto x3()
    {
        // x3 bug: won't accept `const char *` only literal KBH 2019/07/16
        //return x3::char_(parse_sfxs);
        return x3::char_("lsxpwdb");
    }

    static const char sfx(uint8_t code)
    {
        if (code > 7)
            throw std::logic_error{"m68k_sfx::sfx: code = " + std::to_string(code)};
        return suffixes[code];
    }
};

// allow three characters to define condition code
struct ccode_defn
{   
#if 0
    template <unsigned N>
    constexpr ccode_defn(const char (&name_)[N] const, uint8_t value_)
        : ccode_defn(name_, value_, std::make_index_sequence<N-1>() {}

    template <unsigned N, std::size_t...Is>
    constexpr ccode_defn(const char (&name_)[N] const, uint8_t value_
                       , std::index_sequence<Is...>)
        : name { name_[Is]... }, value(value_) {}
#else
        constexpr ccode_defn(const char (&name)[2], uint8_t value)
        : name { name[0], name[1] }, value(value) {}
        
        constexpr ccode_defn(const char (&name)[3], uint8_t value)
        : name { name[0], name[1], name[2] }, value(value) {}
        
        constexpr ccode_defn(const char (&name)[4], uint8_t value)
        : name { name[0], name[1], name[2] }, value(value) {}
        
        constexpr ccode_defn(const char (&name)[5], uint8_t value)
        : name { name[0], name[1], name[2], name[3] }, value(value) {}
#endif
    const char name[4];     // name of condition code (1-3 chars)
    uint8_t    value;       // value of condition code for name
};

// general processor condtion codes
// XXX fix ctor/TMP to allow single character strings
static constexpr ccode_defn m68000_codes [] =
    { { "t"   ,  0 }
    , { "f\0"   ,  1 }
    , { "hi"  ,  2 }
    , { "ls"  ,  3 }
    , { "cc"  ,  4 }       // mit name
    , { "hs"  ,  4 }       // motorola name
    , { "cs"  ,  5 }       // mit name
    , { "le"  ,  5 }       // motorola name
    , { "ne"  ,  6 }
    , { "eq"  ,  7 }
    , { "vc"  ,  8 }
    , { "vs"  ,  9 }
    , { "pl"  , 10 }
    , { "mi"  , 11 }
    , { "ge"  , 12 }
    , { "lt"  , 13 }
    , { "gt"  , 14 }
    , { "le"  , 15 }
    };

// floating point ccodes 
static constexpr ccode_defn m68881_codes [] =
    { { "f\0"   ,  0 }
    , { "eq"  ,  1 }
    , { "ogt" ,  2 }
    , { "oge" ,  3 }
    , { "olt" ,  4 }
    , { "ole" ,  5 }
    , { "ogl" ,  6 }
    , { "or"  ,  7 }
    , { "un"  ,  8 }
    , { "ueq" ,  9 }
    , { "ugt" , 10 }
    , { "uge" , 11 }
    , { "ult" , 12 }
    , { "ule" , 13 }
    , { "ne"  , 14 }
    , { "t\0"   , 15 }
    , { "sf"  , 16 }
    , { "seq" , 17 }
    , { "gt"  , 18 }
    , { "ge"  , 19 }
    , { "lt"  , 20 }
    , { "le"  , 21 }
    , { "gl"  , 22 }
    , { "gle" , 23 }
    , { "ngle", 24 }
    , { "ngl" , 25 }
    , { "nle" , 26 }
    , { "nlt" , 27 }
    , { "nge" , 28 }
    , { "ngt" , 29 }
    , { "sne" , 30 }
    , { "st"  , 31 }
    };

// mmu condition codes
static constexpr ccode_defn m68851_codes [] =
    { { "bs",  0 }
    , { "bc",  1 }
    , { "ls",  2 }
    , { "lc",  3 }
    , { "ss",  4 }
    , { "sc",  5 }
    , { "as",  6 }
    , { "ac",  7 }
    , { "ws",  8 }
    , { "wc",  9 }
    , { "is", 10 }
    , { "ic", 11 }
    , { "gs", 12 }
    , { "gc", 13 }
    , { "cs", 14 }
    , { "cc", 15 }
    };

// m68K_CPID_MMU = 0
// m68k_CPID_FPU = 1
// m68k_CPID_MMU_040 = 2
struct m68k_ccode
{
    enum { M68K_CC_GEN, M68K_CC_FPU, M68K_CC_MPU, NUM_CC };
    
    // DEFNS: first for `floating point`, second for `general`
    // NB: not in `enum` order
    struct ccode
    {
        const char *name;
        int8_t fp_code  {-1};
        int8_t gen_code {-1};
    };

    static constexpr ccode ccodes[] =
    {
      // floating point ccodes first
      // NB: clang won't allow `-1`s to default
        { "f"   , 0,    1 }
      , { "eq"  , 1,    7 }
      , { "ogt" , 2, -1 }
      , { "oge" , 3, -1 }
      , { "olt" , 4, -1 }
      , { "ole" , 5, -1 }
      , { "ogl" , 6, -1 }
      , { "or"  , 7, -1 }
      , { "un"  , 8, -1 }
      , { "ueq" , 9, -1 }
      , { "ugt" , 10, -1 }
      , { "uge" , 11, -1 }
      , { "ult" , 12, -1 }
      , { "ule" , 13, -1 }
      , { "ne"  , 14,   6 }
      , { "t"   , 15,   0 }
      , { "sf"  , 16, -1 }
      , { "seq" , 17, -1 }
      , { "gt"  , 18,   14 }
      , { "ge"  , 19,   12 }
      , { "lt"  , 20,   13 }
      , { "le"  , 21,   15 }
      , { "gl"  , 22, -1 }
      , { "gle" , 23, -1 }
      , { "ngle", 24, -1 }
      , { "ngl" , 25, -1 }
      , { "nle" , 26, -1 }
      , { "nlt" , 27, -1 }
      , { "nge" , 28, -1 }
      , { "ngt" , 29, -1 }
      , { "sne" , 30, -1 }
      , { "st"  , 31, -1 }
   // general ccode only
      , { "hi"  , -1    , 2 }
      , { "ls"  , -1    , 3 }
      , { "cc"  , -1    , 4 }       // mit name
      , { "hs"  , -1    , 4 }       // motorola name
      , { "cs"  , -1    , 5 }       // mit name
      , { "le"  , -1    , 5 }       // motorola name
      , { "vc"  , -1    , 8 }
      , { "vs"  , -1    , 9 }
      , { "pl"  , -1    , 10 }
      , { "mi"  , -1    , 11 }
    };

    // assemble via lookup
    using ccode_value_t = std::array<int8_t, NUM_CC>;
    static auto x3() 
    {
        x3::symbols<ccode_value_t> parser;

        auto p = m68851_codes;
        std::cout << "m68851_codes[0] = " << p->name << ", value = " << +p->value << std::endl;

        // NB: tuple data insert ordered per `enum`
        for (auto& c : ccodes)
        {
            //auto parser.add(c.name, { int8_t(c.gen_code + 1), int8_t(c.fp_code + 1)});
            auto& ccodes = parser.at(c.name);
            ccodes[0] = c.gen_code + 1;
            ccodes[1] = c.fp_code  + 1;
        }

        return parser;
    }

    static int8_t code(ccode_value_t const& ccode, uint8_t cc_type)
    {
        return ccode.at(cc_type) - 1;     // throws if out-of-range
    }

    // disassemble via search
    static const char *name(uint8_t code, uint8_t cc_type = M68K_CC_GEN)
    {
        int8_t ccode::*member;

        switch (cc_type)
        {
            default:
                throw std::logic_error{"m68k_ccode::ccode: ccode"};
            case M68K_CC_GEN:
                member = &ccode::gen_code;
                break;
            case M68K_CC_FPU:
                member = &ccode::fp_code;
                break;
        }

        for (auto& c : ccodes)
            if (c.*member == code)
                return c.name;
                
        return {};
    }

};
}
#endif

