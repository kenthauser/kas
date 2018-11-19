#ifndef KAS_M68K_M68K_REG_IMPL_H
#define KAS_M68K_M68K_REG_IMPL_H

#include "m68k_reg_types.h"
//#include "m68k_reg_defn.h"
//#include "m68k_reg_ctor.h"
//#include "X_m68k_reg_type.h"

namespace kas::m68k
{

////////////////////////////////////////////////////////////////////////////
// implementation of m68k_register
////////////////////////////////////////////////////////////////////////////

auto m68k_reg::get_defn(reg_data_t n) -> decltype(*insns)
{
    return insns[n-1];
}

auto m68k_reg::select_defn() const -> decltype(*insns)
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
m68k_reg::m68k_reg(reg_data_t reg_class, uint16_t value = 0)
{
    // cache common registers
    static reg_data_t base_data, base_addr, base_fp, base_pc;

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

    if (reg_0_index)
        reg_0_ok = !hw::cpu_defs[get_defn(reg_0_index).reg_tst];
}

auto m68k_reg::find_data(reg_data_t rc, uint16_t rv) -> reg_data_t
{
    // brute force search thru data
    auto p = insns;
    for (auto n = 0; n++ < insns_cnt; ++p)
        if (p->reg_class == rc && p->reg_num == rv)
            return n;
    return 0;
}

template <typename T>
void m68k_reg::add(T const& d, reg_data_t n)
{
    //std::cout << "m68k_reg: adding: " << d;
    if (!reg_0_index) 
    {
        reg_0_index = n + 1;
        reg_0_ok    = !hw::cpu_defs[d.reg_tst]; 
    } else {
        //std::cout << "m68k_reg: second entry" << std::endl;
        reg_1_index = n + 1;
        reg_1_ok    = !hw::cpu_defs[d.reg_tst]; 
    }
}

const char *m68k_reg::name() const
{
    return select_defn().name() + 1;
}

uint16_t const m68k_reg::kind() const
{
    return select_defn().reg_class;
}

uint16_t const m68k_reg::value() const
{
    return select_defn().reg_num;
}

const char *m68k_reg::validate_msg() const
{
    return hw::cpu_defs[get_defn(reg_0_index).reg_tst];
}


////////////////////////////////////////////////////////////////////////////
//
// implementation of register set binop & value methods
//
////////////////////////////////////////////////////////////////////////////

m68k_reg_set::m68k_reg_set(m68k_reg const& l)
                        { ops.emplace_back('=', l); }

int16_t m68k_reg_set::kind() const
{
    auto& front = ops.front();
    if (front.first == 'X')
        return -1;

    switch (front.second.kind())
    {
        case RC_DATA:
        case RC_ADDR:
            return RC_DATA;
        case RC_FLOAT:
            return RC_FLOAT;
        case RC_FCTRL:
            return RC_FCTRL;
        default:
            return -1;
    }
}

// register set binop:: left is always left, thus subverting shunting yard.
m68k_reg_set& m68k_reg_set::binop(const char op, m68k_reg_set const& r)
{
    // use `op` to create new `reg_set_op` at end
    if (kind() != r.kind())
        ops.front().first = 'X';
    auto iter = std::begin(r.ops);
    ops.emplace_back(op, iter->second);
    ops.insert(ops.end(), ++iter, r.ops.end());
    return *this;
}

// for *only* predecrement (f)movem to memory, bits are reversed
auto m68k_reg_set::value(bool is_predec) const -> rs_value_t
{
    // short circuit if recalculate...
    if (_value && is_predec == val_predec)
        return _value;

    // For CPU: sixteen bit mask with
    // Normal bit-order: D0 -> LSB, A7 -> MSB
    //
    // For FPU: 8 bit mask with
    // Normal bit-order: FP7 -> LSB, FP0 -> MSB
    //
    // Easiest solution: toggle reverse for FP

    const int mask_bits = (kind() == RC_FLOAT) ? 8 : 16;
    const bool reverse  = (kind() == RC_FLOAT) ^ is_predec;

    bool is_valid = true;
    rs_value_t mask = 0;

    // convert "register" to bit number in range [0-> (mask_bits - 1)]
    auto get_reg_bitnum = [](m68k_reg const *r)
    {
        switch (r->kind())
        {
            case RC_DATA:  return  0 + r->value();
            case RC_ADDR:  return  8 + r->value();
            case RC_FLOAT: return  0 + r->value();
            case RC_FCTRL:
                // floating point control registers are "special"
                switch (r->value())
                {
                    case REG_FPCTRL_CR:  return 12;
                    case REG_FPCTRL_SR:  return 11;
                    case REG_FPCTRL_IAR: return 10;
                }
                // FALLSTHRU
            default:       return -1;
        }
    };

    auto get_mask = [&](auto from, auto to) -> rs_value_t
    {
        // validate arguments
        auto to_bitnum   = get_reg_bitnum(to);
        auto from_bitnum = get_reg_bitnum(from);
        if (to_bitnum < 0 || from_bitnum < 0 || from > to) {
            is_valid = false;
            return 0;
        }

        // calculate # of bits to set
        auto n = to_bitnum - from_bitnum + 1;
        auto mask = (1 << n) - 1;
        if (reverse)
            return mask << ((mask_bits - 1) - to_bitnum);
        else
            return mask << from_bitnum;
    };

    // walk thru regset ops & calculate mask
    auto it  = ops.begin();
    auto end = ops.end();
    auto rp = &(*it++).second;

    for (; it != end; ++it)
    {
        switch (it->first) {
            case '/':
                if (rp)
                    mask |= get_mask(rp, rp);
                rp = &it->second;
                break;
            case '-':
                mask |= get_mask(rp, &it->second);
                rp = nullptr;
                break;
            default:
                throw std::logic_error("m68k_reg_set::get_mask: unexpected op");
        }
    }

    if (rp)
        mask |= get_mask(rp, rp);

    val_predec = is_predec;
    _value = is_valid ? mask : -1;
    return _value;
}


template <typename OS>
void m68k_reg_set::print(OS& os) const {
    for (auto const& op : ops) {
        if (op.first == '=')
            os << "rs[";
        else if (op.first == 'X')
            os << "rs ERR[";
        else
            os << op.first;
        op.second.print(os);
    }
    os << "]";
}

}

#endif


