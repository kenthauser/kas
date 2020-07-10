#ifndef KAS_TARGET_TGT_REG_IMPL_H
#define KAS_TARGET_TGT_REG_IMPL_H

#include "tgt_reg_defn.h"


namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
// implementation of register methods
////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    get_defn(reg_defn_idx_t n) -> defn_t const&
{
    return insns[n-1];
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
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

// get `reg_t` instance from class/value pair
template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    find(reg_defn_idx_t rc, uint16_t rv) -> derived_t const&
{
    return *defn_map().at({rc, rv});
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
template <typename T>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    add_defn(T const& d, reg_defn_idx_t n) -> void
{
    // add to lookup "map"
    map_key key{d.reg_class, d.reg_num};
    defn_map().emplace(key, &derived());

    // assign to `defns[0]` if first; else to `defns[1]`
    if (!defns[0]) 
        defns[0] = n + 1;
#ifdef XXX
    else if (auto tst = get_defn(defns[0]).reg_tst)
    {
        // test if previously defined defn is supported
        if ((*hw_cpu_p)[tst])
        {
            if (!defns[1])
                defns[1] = defns[0];
            defns[0] = n + 1;
        }
    }
#endif
    else if (!defns[1])
        defns[1] = n + 1;
    else
        throw std::logic_error(
            std::string("tgt_reg::add_defn: too many defns: ") + name()
            );
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    name() const -> const char *
{
    return derived_t::format_name(select_defn().name());
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    kind(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_class;
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    value(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_num;
}

template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    validate(int reg_class) const -> const char *
{
    auto&& tst = select_defn(reg_class).reg_tst;
    if (tst)
        return (*hw_cpu_p)[tst];
    return {};
}


template <typename Derived, typename N, typename HW, typename RS, typename IDX
            , typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, N, HW, RS, IDX, B, C, V>::
    is_unparseable() const -> bool
{
    return false;
    auto& defn = get_defn(defns[0]);
    return defn.name()[0] == '*';
}



}

#endif


