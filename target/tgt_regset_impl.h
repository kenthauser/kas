#ifndef KAS_TARGET_TGT_REGSET_IMPL_H
#define KAS_TARGET_TGT_REGSET_IMPL_H

#include "tgt_regset_type.h"

namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
//
// implementation of register set binop & value methods
//
////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename Reg_t, typename T, typename Loc>
tgt_reg_set<Derived, Reg_t, T, Loc>::tgt_reg_set(Reg_t const& l)
{
    std::cout << "tgt_reg_set::ctor: " << l << std::endl;
    ops.emplace_back('=', l);
}

template <typename Derived, typename Reg_t, typename T, typename Loc>
int16_t tgt_reg_set<Derived, Reg_t, T, Loc>::kind() const
{
    if (_error)
        return -(_error);
    
    auto& front = ops.front();
    return derived().reg_kind(front.second);
}

// register set binop:: left is always left, thus subverting shunting yard.
template <typename Derived, typename Reg_t, typename T, typename Loc>
auto tgt_reg_set<Derived, Reg_t, T, Loc>::binop(const char op, tgt_reg_set const& r)
    -> derived_t&
{
    // use `op` to create new `reg_set_op` at end
    if (kind() != r.kind())
        _error = RS_ERROR_INVALID_CLASS;

    auto iter = std::begin(r.ops);
    ops.emplace_back(op, iter->second);
    ops.insert(ops.end(), ++iter, r.ops.end());
    return derived();
}

// for *only* predecrement (f)movem to memory, bits are reversed
template <typename Derived, typename Reg_t, typename T, typename Loc>
auto tgt_reg_set<Derived, Reg_t, T, Loc>::value(bool reverse) const -> rs_value_t
{
    // short circuit if previously calculated
    // NB: mask can't be zero: we don't create empty regsets
    if (!reverse && _value)
        return _value;
    if (reverse && _value2)
        return _value2;

    auto [dir, mask_bits] = derived().rs_mask_bits(reverse);

    rs_value_t mask = 0;

    // structured bindings can't be captured. Name everything
    auto get_mask = [dir=dir, mask_bits=mask_bits, &mask, this](auto from, auto to) -> rs_value_t
    {
        // validate arguments
        auto to_bitnum   = derived().reg_bitnum(*to);
        auto from_bitnum = derived().reg_bitnum(*from);
        if (from_bitnum < 0 || from_bitnum > to_bitnum) {
            _error = RS_ERROR_INVALID_RANGE;
            return 0;
        }

        // calculate # of bits to set
        auto n = to_bitnum - from_bitnum + 1;
        auto mask = (1 << n) - 1;
        if (dir == RS_DIR_MSB0)
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
                throw std::logic_error("tgt_reg_set::get_mask: unexpected op");
        }
    }

    if (rp)
        mask |= get_mask(rp, rp);

    // cache result
    if (!reverse)
        _value = mask;
    else
        _value2 = mask;

    return mask;
}


template <typename Derived, typename Reg_t, typename T, typename Loc>
template <typename OS>
void tgt_reg_set<Derived, Reg_t, T, Loc>::print(OS& os) const {
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


