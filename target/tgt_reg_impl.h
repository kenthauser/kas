#ifndef KAS_TARGET_TGT_REG_IMPL_H
#define KAS_TARGET_TGT_REG_IMPL_H

#include "tgt_reg_defn.h"


namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
// implementation of register methods
////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    get_defn(reg_defn_idx_t n) -> defn_t const&
{
    return insns[n-1];
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    select_defn(int reg_class) const -> defn_t const&
{
    auto& defn = get_defn(defns[0]);

    // if default or class matches, return reg_0
    if (reg_class < 0 || defn.reg_class == reg_class)
        return defn;

    // if reg_1 defined, return it
    if (defns[1])
        return get_defn(defns[1]);
    
    // kind doesn't match
    return defn;
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    find_data(reg_defn_idx_t rc, uint16_t rv) -> reg_defn_idx_t
{
    // brute force search thru data
    auto p = insns;
    for (auto n = 0; n++ < insns_cnt; ++p)
        if (p->reg_class == rc && p->reg_num == rv)
            return n;       // pre-incremented
    return 0;
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    find(reg_defn_idx_t rc, uint16_t rv) -> derived_t const&
{
    //std::cout << "tgt_reg::find: rc = " << std::hex << +rc << ", rv = " << +rv << std::endl;
    //return *defn_map().at(map_key{rc, rv});
    return *defn_map().at({rc, rv});
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
template <typename T>
auto tgt_reg<Derived, N, RS, B, C, V>::
    add_defn(T const& d, reg_defn_idx_t n) -> void
{
    //std::cout << "tgt_reg: adding: " << d;
    // add to lookup "map"
    map_key key{d.reg_class, d.reg_num};
    defn_map().emplace(key, &derived());

    if (!defns[0]) 
    {
        defns[0] = n + 1;
        //reg_0_ok    = !hw::cpu_defs[d.reg_tst]; 
    } else {
        //std::cout << "tgt_reg: second entry" << std::endl;
        defns[1] = n + 1;
        //reg_1_ok    = !hw::cpu_defs[d.reg_tst]; 
    }
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    name() const -> const char *
{
    return derived_t::format_name(select_defn().name());
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    kind(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_class;
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    value(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_num;
}

template <typename Derived, typename N, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, RS, B, C, V>::
    validate(int reg_class) const -> const char *
{
    return {};
    //return hw::cpu_defs[get_defn(reg_0_index).reg_tst];
}
}

#endif


