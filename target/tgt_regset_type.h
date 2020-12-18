#ifndef KAS_TARGET_TGT_REGSET_TYPE_H
#define KAS_TARGET_TGT_REGSET_TYPE_H

#include "kas_core/kas_object.h"

namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
//
// definition of register set
//
////////////////////////////////////////////////////////////////////////////


template <typename Derived, typename Reg_t, typename Ref>
struct tgt_reg_set : core::kas_object<Derived, Ref>
{
    using base_t      = tgt_reg_set;
    using derived_t   = Derived;
    using reg_t       = Reg_t;
    using rs_value_t  = int32_t;     // NB: e_fixed_t
    using core_expr_t = typename core::core_expr_t;
    using regset_name = string::str_cat<typename Reg_t::base_name, KAS_STRING("_REGSET")>;
    using token_t     = parser::token_defn_t<regset_name, Derived>;
    
    // declare errors
    enum  { RS_ERROR_INVALID_CLASS = 1, RS_ERROR_INVALID_RANGE, RS_OFFSET };

protected:
    // Default Register-Set implementations. Override if `Derived`

    // default: kind() is "reg.kind()"
    uint16_t reg_kind(Reg_t const& r) const
    {
        return r.kind();
    }

    // default: bit for reg is "reg.value()"
    uint8_t  reg_num(Reg_t const& r) const
    {
        return r.value();
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
    tgt_reg_set(reg_t const& r, char op = '=');

    // need mutable operators only
    // range ops
    auto& operator- (derived_t const& r)     { return binop('-', r); }
    auto& operator/ (derived_t const& r)     { return binop('/', r); }

    // offset ops
    auto& operator+ (core_expr_t const& r)   { return binop('+', r); }
    auto& operator- (core_expr_t const& r)   { return binop('-', r); }
    auto& operator+ (int r)                  { return binop('+', r); }
    auto& operator- (int r)                  { return binop('-', r); }

    // calculate register-set value 
    // Reg-0 is LSB, unless reversed by `reverse` bits
    rs_value_t value(uint8_t reverse = {}) const;

    // return RC_* for regset class.
    // NB: negative value indicates error index
    int16_t kind() const;
    
    bool is_offset() const { return kind() == -RS_OFFSET; }

    const char *is_error() const
    {
        if (ops.front().first != 'X') return {};

        switch (kind())
        {
            case -RS_ERROR_INVALID_CLASS:
                return "X regset class mismatch";
            case -RS_ERROR_INVALID_RANGE:
                return "X regset invalid range";
            default:
                return "X regset unknown error";
        }
    }

    // these methods only valid for `RC_OFFSET`
    // NB: expr_t definition not complete. Work around...
    template <typename OFFSET_T = expr_t>
    OFFSET_T offset() const;
    auto reg_p() const { return &ops.front().second; }

    template <typename OS> void print(OS&) const;
    
    // expose binop to facilitate manual regset manipulation
    derived_t& binop(const char op, derived_t const& r);
    derived_t& binop(const char op, core_expr_t const& r);
    derived_t& binop(const char op, int r);

private:
    // state is list of reg-set ops
    using reg_set_op = std::pair<char, reg_t>;
    std::vector<reg_set_op> ops;

    // cache expensive to calculate values
    mutable rs_value_t _value     {};
    mutable rs_value_t _value_rev {};
    mutable int16_t    _error  {};
    core_expr_t       *_expr   {};

    static inline core::kas_clear _c{base_t::obj_clear};
};

// hook regset into type system.

// Declare operators to catch "reg op reg" to evaluate register sets
// NB: Allocate a "kas-object" instance of regset & proceed with evaluation
template <typename L, typename R, typename RS = typename L::regset_t>
auto operator- (L const& l, R const& r)
    -> decltype(std::declval<RS>().operator-(r))
{
    // if R is reg_t or regset_t, construct regset, else offset
    using reg_t = typename RS::reg_t;
    static constexpr bool is_reg_t = std::is_same_v<R, reg_t> ||
                                     std::is_same_v<R, RS>;
    return RS::add(l, (is_reg_t ? '=' : '+')).operator-(r);
}
template <typename L, typename R, typename RS = typename L::regset_t>
auto operator/ (L const& l, R const& r)
    -> decltype(std::declval<RS>().operator/(r))
{
    return RS::add(l).operator/(r);
}

// Declare operators to handle displacements (Reg +/- expr, expr + Reg)
template <typename L, typename R, typename RS = typename L::regset_t>
auto operator+ (L const& l, R const& r)
      -> decltype(std::declval<RS>().operator+(r))
{
    return RS::add(l, '+').operator+(r);
}

template <typename L, typename R, typename RS = typename R::regset_t>
auto operator+ (L const& l, R const& r)
      -> decltype(std::declval<RS>().operator+(l))
{
    return RS::add(r, '+').operator+(l);
}
}
#endif
