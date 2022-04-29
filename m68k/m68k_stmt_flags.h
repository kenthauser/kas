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
// actually FPU uses four -- extend `name` field
struct ccode_defn
{   
    template <unsigned N>
    constexpr ccode_defn(const char (&name_)[N], uint8_t value_)
        : ccode_defn(name_, value_, std::make_index_sequence<N>()) {}

    template <unsigned N, std::size_t...Is>
    constexpr ccode_defn(const char (&name_)[N], uint8_t value_
                       , std::index_sequence<Is...>)
        : name { name_[Is]... }, value(value_) {}
    
    const char name[5];     // name of condition code (1-4 chars)
    uint8_t    value;       // value of condition code for name
};

// general processor condtion codes
static constexpr ccode_defn m68000_codes [] =
    { { "t"   ,  0 }
    , { "f"   ,  1 }
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
    { { "f"   ,  0 }
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
    , { "t"   , 15 }
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
    // NB: These values *must* match CPID values in `m68k_insn_common.h`
    enum { M68K_CC_GEN, M68K_CC_FPU, M68K_CC_MMU, NUM_CC };
    
    // DEFNS: first for `floating point`, second for `general`
    // NB: not in `enum` order
    struct ccode
    {
        const char *name;
        int8_t value[NUM_CC];
    };
#if 0
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
#endif
    // assemble via lookup
    using ccode_value_t = std::array<int8_t, NUM_CC>;
#if 1
    static auto& parser()
    {
        static auto parser_p = new x3::symbols<ccode_value_t>;
        return *parser_p;
    }
#endif
    static x3::symbols<ccode_value_t> s_parser;
    static auto& x3() 
    {
        //x3::symbols<ccode_value_t> parser;
#if 0
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
#else
        auto& symbol_parser = parser();
        auto add_table = [&](ccode_defn const *p, ccode_defn const *end, unsigned cpid)
        {
            for (; p != end; p++)
            {
                auto& ccodes = symbol_parser.at(p->name);
                ccodes[cpid] = p->value + 1;
            }
        };
        
#endif
        add_table(m68000_codes, std::end(m68000_codes), M68K_CC_GEN);
        add_table(m68881_codes, std::end(m68881_codes), M68K_CC_FPU);
        add_table(m68851_codes, std::end(m68851_codes), M68K_CC_MMU);
        return symbol_parser;
    }

    static int8_t code(ccode_value_t const& ccode, uint8_t cc_type)
    {
#if 0
        return ccode.at(cc_type) - 1;     // throws if out-of-range
#else
        return ccode[cc_type] - 1;
#endif
    }

    // disassemble via search
    static std::string name(uint8_t code, uint8_t cc_type)
    {
#if 0
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
#else
        ++code;     // value stored is +1
#if 0
        for (auto& c : ccodes)
            if (c.value[cc_type] == code)
                return c.name
#endif
        auto& sym = parser();
        std::string result;
        sym.for_each([&](auto name, auto d)
            {
                if (d[cc_type] == code)
                    result = name;
            });
        return result;
#endif
    }

};
}
#endif

