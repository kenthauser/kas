#ifndef KAS_TARGET_TGT_DEFN_SIZES_H
#define KAS_TARGET_TGT_DEFN_SIZES_H

#include "m68k/m68k_size_lwb.h"

#include <meta/meta.hpp>
#include <type_traits>

namespace kas::tgt::opc
{
// instructions for instruction suffix handling:
// eg: instructions such as `moveq.l` can also be spelled `moveq`
// SFX_* types say how to handle "blank" suffix
struct SFX_NORMAL
{
    static constexpr auto optional  = false;
    static constexpr auto canonical = false;
    static constexpr auto only_none = false;
};

struct SFX_OPTIONAL : SFX_NORMAL
{
    static constexpr auto optional  = true;
};
struct SFX_NONE : SFX_NORMAL
{
    static constexpr auto only_none  = true;
};
struct SFX_CANONICAL_NONE : SFX_NORMAL
{
    static constexpr auto canonical  = true;
};

//
// declare "types" for use in instruction definintion
//

// only single-sizes have non-standard suffix treatment
template <int OP_SIZE, typename SFX = SFX_NORMAL>
using define_sz = meta::list<meta::int_<1 << OP_SIZE>, SFX>;

// `void` size is always single size, no suffix
using sz_void = define_sz<0, SFX_NONE>;

template <typename MCODE_T>
struct tgt_defn_sizes
{
    using size_fn_t =  m68k::opc::m68k_insn_lwb;

    template <typename MASK, typename SFX, typename SZ_FN>
    constexpr tgt_defn_sizes(meta::list<meta::list<MASK, SFX>, SZ_FN>)
        : size_mask           { MASK::value    }
        , single_size         { !(MASK::value & (MASK::value - 1)) }
        , opt_no_suffix       { SFX::optional  }
        , no_suffix_canonical { SFX::canonical }
        , only_no_suffix      { SFX::only_none }
        , size_fn             { SZ_FN()         }
        {}
    
    // if no SZ_FN specified, get from `size_fn_t`
    template <typename MASK, typename SFX>
    constexpr tgt_defn_sizes(meta::list<meta::list<MASK, SFX>, void>)
            : tgt_defn_sizes(meta::list<meta::list<MASK, SFX>
                                     , typename size_fn_t::default_t>()) {}
    
    // if no SFX rule specified, default to SFX_NORMAL
    template <typename MASK, typename SZ_FN>
    constexpr tgt_defn_sizes(meta::list<meta::list<MASK>, SZ_FN>)
            : tgt_defn_sizes(meta::list<meta::list<MASK, SFX_NORMAL>, SZ_FN>()) {}

    // create an `iterator` to allow range-for to process sizes
    struct iter
    {
        static constexpr auto ITER_END = -1;
        iter(tgt_defn_sizes const& obj, bool is_begin_iter = {}) : obj(obj)
        {
            if (is_begin_iter)
            {
                if (obj.size_mask & 1)
                    sz = 0;
                else
                    sz = next(0);
            }
        }

        // find first size after `prev`
        int8_t next(uint8_t prev) const
        {
            // if any left, find next
            for (int n = obj.size_mask >> ++prev; n ; ++prev)
                if (n & 1)
                    return prev;
                else
                    n >>= 1;
            
            return ITER_END;      // end of sizes
        }

        // range operations
        auto& operator++() 
        {
            sz = next(sz);
            return *this;
        }
        auto operator*() const
        { 
            return sz;
        }
        auto operator!=(iter const& other) const
        {
            return sz != other.sz;
        }
    
    private:
        tgt_defn_sizes const& obj;
        int8_t                sz{ITER_END};    // -1 is end flag
    };

    auto begin() const { return iter(*this, true); }
    auto end()   const { return iter(*this);       }

    // default: return single instance of base name
    // NB: this is the `range expression` in range-based-for-loop
    auto operator()(const char *base_name, uint8_t sz) const
    {
        std::array<const char *, 1> names = { base_name };
        return names;
    };

    // return pair of suffixes for insn, in canonical order
    std::pair<const char *, const char *> suffixes(const char *sfx) const
    {
        // NB: all 4 combinations are represented
        static constexpr auto suffix_void = "";
#if 0
        std::cout << "tgt_defn_size:" << std::boolalpha;
        std::cout << " only_no_suffix:"  << bool(only_no_suffix);
        std::cout << ", no_suffix_canon:" << bool(no_suffix_canonical);
        std::cout << ", opt_no_suffix:"   << bool(opt_no_suffix);
        std::cout << std::endl;
#endif
        if (only_no_suffix)
            return { suffix_void, {} };
        else if (no_suffix_canonical)
            return { suffix_void, sfx };
        else if (opt_no_suffix)
            return { sfx, suffix_void };
        else
            return { sfx, {} };
    }
    
    uint16_t  size_mask           : 8;
    uint16_t  single_size         : 1;
    uint16_t  opt_no_suffix       : 1;
    uint16_t  no_suffix_canonical : 1;
    uint16_t  only_no_suffix      : 1;
    size_fn_t size_fn;
};
}
#endif
