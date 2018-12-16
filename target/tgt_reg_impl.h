#ifndef KAS_TARGET_TGT_REG_IMPL_H
#define KAS_TARGET_TGT_REG_IMPL_H

#include "tgt_reg_defn.h"


namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
// implementation of register methods
////////////////////////////////////////////////////////////////////////////

template <typename Derived>
auto tgt_reg<Derived>::get_defn(reg_defn_idx_t n) -> defn_t const&
{
    return insns[n-1];
}

template <typename Derived>
auto tgt_reg<Derived>::select_defn() const -> defn_t const&
{
    auto index = reg_0_index;
    if (!reg_0_ok && reg_1_ok)
        index = reg_0_index;
    if (!index)
        throw std::runtime_error("invalid register");
    return get_defn(index);
}

//
// The class/value ctor is non-trival. It is designed
// for use in the disassembler only
//
template <typename Derived>
tgt_reg<Derived>::tgt_reg(reg_defn_idx_t reg_class, uint16_t value)
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

template <typename Derived>
auto tgt_reg<Derived>::find_data(reg_defn_idx_t rc, uint16_t rv) -> reg_defn_idx_t
{
    // brute force search thru data
    auto p = insns;
    for (auto n = 0; n++ < insns_cnt; ++p)
        if (p->reg_class == rc && p->reg_num == rv)
            return n;
    return 0;
}

template <typename Derived>
template <typename T>
void tgt_reg<Derived>::add(T const& d, reg_defn_idx_t n)
{
    //std::cout << "m68k_reg: adding: " << d;
    if (!reg_0_index) 
    {
        reg_0_index = n + 1;
        //reg_0_ok    = !hw::cpu_defs[d.reg_tst]; 
    } else {
        //std::cout << "m68k_reg: second entry" << std::endl;
        reg_1_index = n + 1;
        //reg_1_ok    = !hw::cpu_defs[d.reg_tst]; 
    }
}

template <typename Derived>
const char *tgt_reg<Derived>::name() const
{
    return derived_t::format_name(select_defn().name());
}

template <typename Derived>
uint16_t const tgt_reg<Derived>::kind(int reg_class) const
{
    return select_defn().reg_class;
}

template <typename Derived>
uint16_t const tgt_reg<Derived>::value(int reg_class) const
{
    return select_defn().reg_num;
}

template <typename Derived>
const char *tgt_reg<Derived>::validate(int reg_class) const
{
    return {};
    //return hw::cpu_defs[get_defn(reg_0_index).reg_tst];
}
}

#endif


