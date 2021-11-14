#ifndef KAS_ARM_ARM_ADDR_MAPPING_H
#define KAS_ARM_ARM_ADDR_MAPPING_H

#include "kas_core/core_fragment.h"
#include "kas_core/core_symbol.h"
#include "kas/kas_string.h"
#include <deque>
#include <algorithm>

namespace kas::arm
{
using ARM_SEG_DATA  = KAS_STRING("$d");
using ARM_SEG_ARM   = KAS_STRING("$a");
using ARM_SEG_THUMB = KAS_STRING("$t");

using arm_mapping_v = meta::list<
          ARM_SEG_DATA
        , ARM_SEG_ARM
        , ARM_SEG_THUMB
        >;

namespace detail
{
    struct arm_seg_mapping
    {
        using index_t    = typename core::core_segment::index_t;
        using value_t    = std::pair<core::core_addr_t const *, const char *>;
        using addr_seg_t = std::deque<value_t>;

        // retrieve table of all segment mappings
        static auto& map()
        {
            static auto _map = new std::map<index_t, addr_seg_t>;
            return *_map;
        }

        const char *map_get(core::core_fragment const *frag_p) const
        {
            auto& segment = frag_p->segment();
            auto  it = map().find(segment.index());
            if (it == map().end())
                return ARM_SEG_DATA();
            return it->second.back().second;
        }

        void map_set(core::core_fragment const *frag_p, const char *s) const
        {
            // create symbol (STT_NOTYPE, STB_LOCAL)
            auto& sym = core::core_symbol_t::add(s);
            sym.make_label(STB_LOCAL);

            // save address in deque
            auto& segment = frag_p->segment();
            auto& obj     = map()[segment.index()];
            obj.emplace_back(sym.addr_p(), s);
        }

        void operator()(const char *s = ARM_SEG_DATA())
        {
   //         std::cout << "arm_seg_mapping: " << s;
   //         std::cout << ", seg = " << core::core_fragment::cur_frag->segment();
   //         std::cout << std::endl;
   //         std::cout << "arm_seg_mapping: current = ";
   //         std::cout << map_get(core::core_fragment::cur_frag) << std::endl;
            if (s != map_get(core::core_fragment::cur_frag))
                map_set(core::core_fragment::cur_frag, s);
        }

        // XXX should not be in-lined...
        const char *get_arm_map_t(core::core_addr_t   const& addr) const
        {
            // compare function
            auto cmp = [](auto& a, auto& b)
                { //  std::cout << std::endl;
                  //  std::cout << "[cmp: " << a << " < " << *b.first << " -> ";
                    bool result = a < *b.first;
                  //  std::cout << std::boolalpha << result << "] ";
                    return result;
                };

            // get handle to frag
            auto& frag_p = addr.frag_p;
            if (!frag_p)
                return ARM_SEG_DATA();      // undefined -- external

            auto map_it = map().find(frag_p->segment().index());
            if (map_it == map().end())
                return ARM_SEG_DATA();
            
            auto& obj = map_it->second;
            auto it = std::upper_bound(obj.begin(), obj.end(), addr, cmp);
            const char *value = "not found";

            if (it == obj.end())
                value = obj.back().second;
            else if (it == obj.begin())
                value = it->second;
            else
                value = std::prev(it)->second;
#if 0                
            if (it == obj.end())
            {
                return "not found";
            }
#endif
            return value;
        }
        const char *get_arm_map_t(core::core_symbol_t const& sym) const
        {
            if (auto p = sym.addr_p())
                return get_arm_map_t(*p);
            return ARM_SEG_DATA();
        }
    };
}

struct arm_data_seg : detail::arm_seg_mapping
{
    arm_data_seg()
    {
        std::cout << "arm_data_seg::ctor()" << std::endl;
        (*this)();
    }
};

template <typename REF>
const char *arm_addr_map_t(core::core_symbol<REF> const& sym)
{
    return detail::arm_seg_mapping().get_arm_map_t(sym);
}
}
#endif

