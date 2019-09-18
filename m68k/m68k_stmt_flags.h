#ifndef KAS_M68K_M68k_STMT_FLAGS_H
#define KAS_M68K_M68k_STMT_FLAGS_H

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

struct m68k_ccode
{
    enum { M68K_CC_GEN, M68K_CC_FP };
    
    // first for `floating point`, second for `general`
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
    using ccode_value_t = std::tuple<int8_t, int8_t>;
    static auto x3() 
    {
        x3::symbols<ccode_value_t> parser;

        for (auto& c : ccodes)
            parser.add(c.name, { c.gen_code, c.fp_code });

        return parser;
    }

    static int8_t code(ccode_value_t const& ccode, uint8_t cc)
    {
        switch (cc)
        {
            default:
                throw std::logic_error{"m68k_ccode::ccode: code"};
            case M68K_CC_GEN:
                return std::get<M68K_CC_GEN>(ccode);
            case M68K_CC_FP:
                return std::get<M68K_CC_FP>(ccode);
        }
    }

    // disassemble via search
    static const char *name(uint8_t code, uint8_t cc = M68K_CC_GEN)
    {
        int8_t ccode::*member;

        switch (cc)
        {
            default:
                throw std::logic_error{"m68k_ccode::ccode: ccode"};
            case M68K_CC_GEN:
                member = &ccode::gen_code;
                break;
            case M68K_CC_FP:
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
