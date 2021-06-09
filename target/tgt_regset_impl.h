#ifndef KAS_TARGET_TGT_REGSET_IMPL_H
#define KAS_TARGET_TGT_REGSET_IMPL_H

#include "tgt_regset_type.h"

#include <iostream>

namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
//
// implementation of register set binop & value methods
//
////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename Reg_t, typename Ref>
tgt_reg_set<Derived, Reg_t, Ref>::tgt_reg_set(Reg_t const& l, char op)
{
    //std::cout << "tgt_reg_set::ctor: " << l << std::endl;
    ops.emplace_back(op, l);
}

template <typename Derived, typename Reg_t, typename Ref>
int16_t tgt_reg_set<Derived, Reg_t, Ref>::kind() const
{
    if (_error)
        return _error;

    auto& front = ops.front();
    if (front.first == '+')
        return -RS_OFFSET;
   
    //std::cout << "tgt_regset::kind: " << front.second;
    //std::cout << " = " << +derived().reg_kind(front.second) << std::endl;
    return derived().reg_kind(front.second);
}

// register set binop:: left is always left, thus subverting shunting yard.
template <typename Derived, typename Reg_t, typename Ref>
auto tgt_reg_set<Derived, Reg_t, Ref>::binop(const char op, derived_t const& r)
    -> derived_t&
{
    // NB: two cases mimic each other: "expr - regset" & "regset-regset"
    // NB: first case xlated to "regset + -expr"

    // use `op` to create new `reg_set_op` at end
    if (kind() != r.kind())
        _error = RS_ERROR_INVALID_CLASS;

    auto iter = std::begin(r.ops);
    ops.emplace_back(op, iter->second);
    ops.insert(ops.end(), ++iter, r.ops.end());
    if (_error)
        ops.front().first = 'X';
        
    return derived();
}

// expression binop:: only +/- supported, so no precidence issue
template <typename Derived, typename Reg_t, typename Ref>
auto tgt_reg_set<Derived, Reg_t, Ref>::binop(const char op, core_expr_t const& r)
    -> derived_t&
{
    if (kind() != -RS_OFFSET)
        _error = RS_ERROR_INVALID_CLASS;

    // if expression is fixed, treat as an int arg.
    if (auto p = r.get_fixed_p())
        return binop(op, *p);
    if (!_expr)
    {
        auto& expr = _value + r;
        _expr = &expr;
    }
    else if (op == '+')
        _expr->operator+(std::move(r));
    else if (op == '-')
        _expr->operator-(std::move(r));
    else
        _error = RS_ERROR_INVALID_CLASS;
    
    if (_error)
        ops.front().first = 'X';
    
    return derived();
}

template <typename Derived, typename Reg_t, typename Ref>
auto tgt_reg_set<Derived, Reg_t, Ref>::binop(const char op, int value)
    -> derived_t&
{
    if (kind() != -RS_OFFSET)
        _error = RS_ERROR_INVALID_CLASS;

    if (op == '-')
        value = -value;
    
    if (_expr)
        _expr->operator+(value);
    else
        _value += value;
    
    if (_error)
        ops.front().first = 'X';
    
    return derived();
}

// Value calculated with `reg_num(reg)` == 0 as LSB...
// ...unless reverse specifies new bit-position for LSB
// example: use reverse = 16 if reg-0 is 1<<15 & reg-15 == 1<<0
template <typename Derived, typename Reg_t, typename Ref>
auto tgt_reg_set<Derived, Reg_t, Ref>::value(uint8_t reverse) const -> rs_value_t
{
    // short circuit if previously calculated
    // NB: mask can't be zero: we don't create empty regsets
    if (!reverse && _value)
        return _value;
    if (reverse && _value_rev)
        return _value_rev;

    rs_value_t mask = 0;

    // calculate register bits for one term
    auto get_mask = [&reverse, &mask, this](auto from, auto to)
                        -> rs_value_t
    {
        // validate arguments
        auto to_bitnum   = derived().reg_num(*to);
        auto from_bitnum = derived().reg_num(*from);
        if (from_bitnum < 0 || from_bitnum > to_bitnum) {
            _error = RS_ERROR_INVALID_RANGE;
            return 0;
        }

        // calculate # of bits to set
        auto n = to_bitnum - from_bitnum + 1;
        auto mask = (1 << n) - 1;
        if (reverse)
            return mask << ((reverse - 1) - to_bitnum);
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
        _value_rev = mask;

    return mask;
}

template <typename Derived, typename Reg_t, typename Ref>
template <typename OFFSET_T>
OFFSET_T tgt_reg_set<Derived, Reg_t, Ref>::offset() const
{
    if (_expr) return *_expr;
    return _value;
}



template <typename Derived, typename Reg_t, typename Ref>
void tgt_reg_set<Derived, Reg_t, Ref>::print(std::ostream& os) const
{
    // print register-set
    auto print_rs = [&]
        {
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
        };

    // print offset
    auto print_offset = [&]
        {
            // print register
            ops.front().second.print(os);
            os << "@(";
            if (_expr)
                os << expr_t(*_expr);   // XXX _expr->print() gives link error
            else
                os << _value;
            os << ")";
        };

    // select register-set or offset
    if (ops.front().first == '+')
        print_offset();
    else
        print_rs();
}

}

#endif


