#ifndef KAS_TARGET_TGT_REG_DEFN_H
#define KAS_TARGET_TGT_REG_DEFN_H

#include "kas_core/kas_object.h"

//
// m68k register patterns:
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple < reg_class(ex RX_DATA), reg_num(eg 0), tst(eg: hw::index_full)
// 1) reg_class    ranges from 0..9. `reg_arg_validate` has a hardwired limit of 12.
// 2) The reg_num  ranges from 0..7 for eg data register; has 12-bit value for move.c
// 3) tst          16-bit hw_tst constexpr value
//
// Each register also has a name. Based on command line options, a leading `%` may
// be permitted or requied
//
// A few registers have aliases. Currently the only aliases are fp->a6 & sp->a7
//
// A few registers can have multiple definitions:
// Example: USP can be a `RC_CPU` register when determining if (eg.) move.l a0,usp is allowed
//          USP can also be used as a `RC_CTRL` register in (eg.)   move.c a0, usp
// The two USPs have different `tst` conditions.
//
// Register `PC` is also overloaded.

// Observations:
// - Only a single alias is permitted.
// - No more hand two register definitions can have the same name
// - The "second" register definition index cannot be zero (because defition
//   zero is processed first, it will always be a "first" definition). Thus
//   a index of zero can be used to tell second definition is not present.
// - KAS_STRING register name definitions should always prepend '%'. A simple +1
//   can be used to remove it.
// currently, for M68K there are ~100-120 register definitions

// RC_PC & RC_ZPC both have a single member. Can be merged into RC_CPU

// Also note: `m68k_reg`    is an expression type
//            `m68k_regset` is an expression type
//            `m68k_reg`    needs to export parser to `expr`

//#include "m68k_types.h"

namespace kas::m68k 
{

////////////////////////////////////////////////////////////////////////////
//
// definition of target register
//
////////////////////////////////////////////////////////////////////////////

// declare constexpr definition of register generated at compile-time
template <typename HW_TST, typename NAME_INDEX_T>
struct tgt_reg_defn
{
    // type definitions for `parser::sym_parser_t`
    // NB: ctor NAMES index list will include aliases 
    using NAME_LIST = meta::list<meta::int_<0>>;

    template <typename...NAMES, typename N, typename REG_C, typename REG_V, typename REG_TST>
    constexpr m68k_reg_defn(meta::list<meta::list<NAMES...>
                                     , meta::list<N, REG_C, REG_V, REG_TST>>)
        : names     { (NAMES::value + 1)... }
        , reg_class { REG_C::value          }
        , reg_num   { REG_V::value          }
        , reg_tst   { REG_TST()             }
    {}

    static constexpr auto MAX_REG_NAMES = 2;        // allow single alias
    static inline const char *const *names_base;

    // first arg is which alias (0 = canonical, non-zero = which alias)
    const char *name(unsigned n = 0, unsigned skip_initial = 0) const noexcept
    {
        if (n < MAX_REG_NAMES)
            if (auto idx = names[n])
                return names_base[idx-1] + skip_initial;
        return {};
    }

    // each defn is 4 16-bit words (ie 64-bits) iff IDX_T is 8-bits
    NAME_IDX_T  names[MAX_REG_NAMES];   // array 
    uint16_t    reg_class;              
    uint16_t    reg_num;
    hw::hw_tst  reg_tst;
  
    template <typename OS>
    friend OS& operator<<(OS& os, tgt_reg_defn const& d)
    {
        os << "reg_defn: " << std::hex;
        const char * prefix = "names= ";
        for (auto& name : d.names)
        {
            if (name)
                os << prefix << name;
            prefix = ",";
        }
        os << " class="  << +d.reg_class;
        os << " num="    << +d.reg_num;
        os << " tst="    <<  d.reg_tst;
        return os << std::endl;
    }
};


}


#endif
