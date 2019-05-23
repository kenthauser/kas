#ifndef KAS_ARM_ARM_MCODE_SIZES_H
#define KAS_ARM_ARM_MCODE_SIZES_H

//
// The `tgt_mcode_sizes` is a support class for the `tgt_mcode_adder` class.
//
// Since the `arm` processor doesn't have immediate args, use the `sz` arg to 
// generate variants.
//
// Variants of base code are:
//
// 1. S_FLAG:   add `S` to base name to indicate updating condition registers.
//              set `SZ_SZ_S_FLAG` in `sz` to indicate active state
//
// 2. COND:     opcode lines `0-14` indicate conditional execution. 
//              set 4 LSBs as mask. use zero if COND not set
//
// 3. NW_FLAG:  generate `.w` (wide) and `.n` (narrow) suffix. Set
//              `SZ_SZ_N_FLAG` or `SZ_SZ_W_FLAG` if set
//
// 4. NO_AL:    don't generate condition code with `al` suffix. Don't
//              set bit in `SZ`, just don't generate
//
// In addition, "aliases" (ie pre-unified names) are supplied as arguments to 
// `OP<>` type. If specified, generate these aliases.

#include "arm_mcode.h"

#include <meta/meta.hpp>
#include <type_traits>


namespace kas::tgt::opc
{
//
// declare "types" for use in instruction definintion
//


// these are the bits in `DEFN`
static constexpr auto SZ_ARCH_ARM     = 0x00;
static constexpr auto SZ_ARCH_THB     = 0x01;
static constexpr auto SZ_ARCH_THB_EE  = 0x02;
static constexpr auto SZ_ARCH_ARM64   = 0x04;

static constexpr auto SZ_DEFN_COND    = 0x10;
static constexpr auto SZ_DEFN_S_FLAG  = 0x20;
static constexpr auto SZ_DEFN_NW_FLAG = 0x40;
static constexpr auto SZ_DEFN_NO_AL   = 0x80;

// only single-sizes have non-standard suffix treatment
template <int OP_ARCH, int...OP_FLAGS>
using arm_sz = meta::int_<(OP_ARCH | ... | OP_FLAGS)>;

template <>
struct tgt_mcode_sizes<arm::arm_mcode_t>
{
    template <int N, typename SZ_FN>
    constexpr tgt_mcode_sizes(meta::list<meta::int_<N>, SZ_FN>)
        : arm_sz_defn { N } 
        {}
    
    // create an `iterator` to generate requested 
    struct iter
    {
        // iterate over this array for `FLAGs` args
        static constexpr uint8_t zero_two[] = {0, 1, 2};

        // iterate over this array for Condition Codes
        static constexpr std::pair<uint8_t, const char *> cc_info[] =
        {
              { 0, "eq"}        // must be first for `mcode::code` logic
            , { 1, "ne"}
            , { 2, "cs"}
            , { 2, "hs"}
            , { 3, "cc"} 
            , { 3, "lo"}
            , { 4, "mi"}
            , { 5, "pl"}
            , { 6, "vs"}
            , { 7, "vc"}
            , { 8, "hi"}
            , { 9, "ls"}
            , {10, "ge"}
            , {11, "lt"}
            , {12, "lt"}
            , {13, "le"}
            , {14, ""}
            , {14, "al"}        // must be last (for `NO_AL`)
        };


        iter(tgt_mcode_sizes const& obj, bool is_begin_iter = {})
        {
            auto& defn = obj.arm_sz_defn;
            
            // calculate `iters` for end, using instance value
            if (!(defn & SZ_DEFN_COND))
                ccode_end = std::next(ccode_iter);
            else if (defn & SZ_DEFN_NO_AL)
                ccode_end= std::prev(ccode_end);

            if (!(defn & SZ_DEFN_S_FLAG))
                sflag_end = std::next(sflag_iter);

            if (!(defn & SZ_DEFN_NW_FLAG))
                nwflag_end= std::next(nwflag_iter);
        }

        // std `range-for` methods
        auto& operator++() 
        {
            // XXX ignore variants...
#ifdef XXX 
            // CCODE first
            if (++ccode_iter != ccode_end)
                return *this;
            ccode_iter = std::begin(cc_info);

            // S_FLAG middle
            if (++sflag_iter != sflag_end)
                return *this;
            sflag_iter = std::begin(zero_two);
#endif
            // NW_FLAG last
            ++nwflag_iter;
            return *this;
        }
        bool operator!=(iter const&) const
        {
            return nwflag_iter != nwflag_end;
        }
        
        // instance operators
        auto& operator*() const
        { 
            return *this;
        }
        operator uint8_t() const
        {
            uint8_t sz = ccode_iter->first;
            if (*sflag_iter)
                sz |= arm::SZ_SZ_S_FLAG;
            if (*nwflag_iter & 1)
                sz |= arm::SZ_SZ_N_FLAG;
            else if (*nwflag_iter & 2)
                sz |= arm::SZ_SZ_W_FLAG;
            return sz;
        }
    
        // Default iterators:
        // 1. iterate over {0, 1} for S_FLAG
        uint8_t const *sflag_iter  = std::begin(zero_two);
        uint8_t const *sflag_end   = &sflag_iter[2];

        // 2. iterate of all condition codes
        using cc_info_t = std::remove_extent_t<decltype(cc_info)>;
        cc_info_t const* ccode_iter = std::begin(cc_info);
        cc_info_t const* ccode_end  = std::cend(cc_info);

        // 3. iterator over {0, 1, 2} for NW_FLAG
        uint8_t const *nwflag_iter = std::begin(zero_two);
        uint8_t const *nwflag_end  = std::end  (zero_two);
    };

    auto begin() const { return iter(*this, true); }
    auto end()   const { return iter(*this);       }

    // NB: this is the `range expression` in range-based-for-loop
    auto operator()(const char *base_name, iter const& it) const
    {
        std::string name(base_name);
       
        // XXX this is for `ARM` arch. 
        if (*it.sflag_iter)
            name += "s";
        if (arm_sz_defn & SZ_DEFN_COND)
            name += it.ccode_iter->second;
        if (*it.nwflag_iter & 1)
            name += ".n";
        else if (*it.nwflag_iter & 2)
            name += ".w";

        std::array<std::string, 1> names = { name };
        return names;
    };

    uint8_t  arm_sz_defn;
};
}
#endif
