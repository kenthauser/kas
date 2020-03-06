#ifndef KAS_TARGET_TGT_REG_IMPL_H
#define KAS_TARGET_TGT_REG_IMPL_H

#include "tgt_reg_defn.h"


namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
// implementation of register methods
////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    get_defn(reg_defn_idx_t n) -> defn_t const&
{
    return insns[n-1];
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
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

#if 0
//
// The class/value ctor is non-trival. It is designed
// for use in the disassembler only
//
template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
tgt_reg<Derived, RS, B, C, V>::
    tgt_reg(reg_defn_idx_t reg_class, uint16_t value)
{
#if 0
    // cache common registers
    static reg_defn_idx_t base_data, base_addr, base_fp, base_pc;

    switch (reg_class)
    {
        case RC_DATA:
            if (!base_data)
                base_data = find_data(reg_class, 0);
            reg_0_index = base_data + value;
            break;
        case RC_ADDR:
            if (!base_addr)
                base_addr = find_data(reg_class, 0);
            reg_0_index = base_addr + value;
            break;
        case RC_FLOAT:
            if (!base_fp)
                base_fp = find_data(reg_class, 0);
            reg_0_index = base_fp + value;
            break;
        case RC_PC:
            if (!base_pc)
                base_pc = find_data(reg_class, 0);
            reg_0_index = base_pc;
            break;
        default:
            reg_0_index = find_data(reg_class, value);
            break;
    }
#else
    reg_0_index = find_data(reg_class, value);
#endif

#if 0
    if (reg_0_index)
        reg_0_ok = !hw::cpu_defs[get_defn(reg_0_index).reg_tst];
#endif
}
#endif

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    find_data(reg_defn_idx_t rc, uint16_t rv) -> reg_defn_idx_t
{
    // brute force search thru data
    auto p = insns;
    for (auto n = 0; n++ < insns_cnt; ++p)
        if (p->reg_class == rc && p->reg_num == rv)
            return n;       // pre-incremented
    return 0;
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    get(reg_defn_idx_t rc, uint16_t rv) -> derived_t const&
{
    auto defn = find_data(rc, rv);      // 1-based index
    if (!defn)
        throw std::logic_error{"tgt_reg::find_data: register not found"};
    return *lookup(insns[defn].name());
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
template <typename T>
auto tgt_reg<Derived, RS, B, C, V>::
    add(T const& d, reg_defn_idx_t n) -> void
{
    //std::cout << "tgt_reg: adding: " << d;
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

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    name() const -> const char *
{
    return derived_t::format_name(select_defn().name());
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    kind(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_class;
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    value(int reg_class) const -> uint16_t const
{
    return select_defn(reg_class).reg_num;
}

template <typename Derived, typename RS, typename B, unsigned C, unsigned V>
auto tgt_reg<Derived, RS, B, C, V>::
    validate(int reg_class) const -> const char *
{
    return {};
    //return hw::cpu_defs[get_defn(reg_0_index).reg_tst];
}
}

#endif


