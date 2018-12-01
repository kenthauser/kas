#ifndef KAS_TARGET_TGT_REGSET_TYPE_H
#define KAS_TARGET_TGT_REGSET_TYPE_H

//
// m68k register patterns:
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple < reg_class(ex RX_DATA), reg_num(eg 0), tst(eg: hw::index_full)
// 1) reg_class    ranges from 0..9. `reg_arg_validate` has a hardwired limit of 12.
// 2) The reg_num  ranges from 0..7 for eg data register; has 12-bit value for move.c
// 3) tst          16-bit hw_tst constexpr value
//
// Each register also has a name. Based on command line options, a leading `%` may
// be permitted or requied
//
// A few registers have aliases. Currently the only aliases are fp->a6 & sp->a7
//
// A few registers can have multiple definitions:
// Example: USP can be a `RC_CPU` register when determining if (eg.) move.l a0,usp is allowed
//          USP can also be used as a `RC_CTRL` register in (eg.)   move.c a0, usp
// The two USPs have different `tst` conditions.
//
// Register `PC` is also overloaded.

// Observations:
// - Only a single alias is permitted.
// - No more hand two register definitions can have the same name
// - The "second" register definition index cannot be zero (because defition
//   zero is processed first, it will always be a "first" definition). Thus
//   a index of zero can be used to tell second definition is not present.
// - KAS_STRING register name definitions should always prepend '%'. A simple +1
//   can be used to remove it.
// currently, for M68K there are ~100-120 register definitions

// RC_PC & RC_ZPC both have a single member. Can be merged into RC_CPU

// Also note: `reg_t`    is an expression type
//            `reg_tset` is an expression type
//            `reg_t`    needs to export parser to `expr`

#include "kas_core/kas_object.h"

namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
//
// definition of register set
//
////////////////////////////////////////////////////////////////////////////


template <typename Derived, typename Reg_t, typename T = uint32_t, typename Loc = core::ref_loc_t<Derived>>
struct tgt_reg_set : core::kas_object<Derived, Loc>
{
    using base_t     = tgt_reg_set;
    using derived_t  = Derived;
    using reg_t      = Reg_t;
    using rs_value_t = T;

    // declare errors
    enum  { RS_ERROR_INVALID_CLASS = 1, RS_ERROR_INVALID_RANGE };

protected:
    // Default Register-Set implementations. Override if `Derived`

    // declare register-set bit order
    enum { RS_DIR_LSB0, RS_DIR_MSB0 };

    // default: LSB is register zero
    static constexpr bool rs_bit_dir = RS_DIR_LSB0;

    // default: kind() is "reg.kind()"
    uint16_t reg_kind(Reg_t const& r) const
    {
        return r.kind();
    }

    // default: bit for reg is "reg.value()"
    uint8_t  reg_bitnum(Reg_t const& r) const
    {
        return r.value();
    }

    // default: number of bits in result
    // NB: some registers sets on same processor can be in different direction
    std::pair<bool, uint8_t> rs_mask_bits(bool reverse) const
    {
        return {  derived().rs_bit_dir ^ reverse
               ,  std::numeric_limits<rs_value_t>::digits
               };
    }
    
    // record "range" and "extend" operators for disassembler
    static constexpr auto ch_range  = '-';
    static constexpr auto ch_extend = '/';

    
public:
    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

    // ctor (no default needed as not in x3 grammar)
    tgt_reg_set(reg_t const& r);

    // need mutable operators only
    auto& operator- (derived_t const& r)
    { 
        return binop('-', r);
    }
    auto& operator/ (derived_t const& r)
                        { return binop('/', r); }

    // never used, always defined
    auto& operator+ (derived_t const& r)
                        { return binop('/', r); }

    // calculate register-set value 
    rs_value_t value(bool reverse = false) const;

    // return RC_* for regset class.
    // NB: negative value indicates error index
    int16_t kind() const;

    template <typename OS> void print(OS&) const;
#if 1
    template <typename U>
    std::enable_if_t<std::is_same_v<U, Reg_t>, derived_t&>
    friend operator-(Reg_t const& l, U const& r)
    {
        return l - derived_t(r);
    }
#endif
    
private:
    derived_t& binop(const char op, tgt_reg_set const& r);

    // state is list of reg-set ops
    using reg_set_op = std::pair<char, reg_t>;
    std::vector<reg_set_op> ops;

    // cache expensive to calculate values
    mutable rs_value_t _value  {};
    mutable rs_value_t _value2 {};
    mutable int16_t    _error  {};

    static inline core::kas_clear _c{base_t::obj_clear};
};

}

// hook regset into type system.
// NB: must be done in "kas" namespace so template is found
//namespace kas
//{

#if 1
// Add a bunch of TMP tests so only matches "reg, regset" pair
// NB: Allocate a "kas-object" instance of regset & proceed with evaluation
template <typename Derived, typename Reg_t
        , typename = std::enable_if_t<std::is_same_v<Reg_t, typename Derived::reg_t>>
        >
inline auto operator- (Reg_t const& l, Derived const& r)
    -> decltype(std::declval<Derived>().operator-(r))
{
    return Derived::add(l).operator-(r);
}
#endif

template <typename Derived, typename Reg_t
        , typename = std::enable_if_t<std::is_same_v<Reg_t, typename Derived::reg_t>>>
inline auto operator/ (Reg_t const& l, Derived const& r)
    -> decltype(std::declval<Derived>().operator/(r))
{
    return Derived::add(l).operator/(r);
}

//}

#endif
