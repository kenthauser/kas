#ifndef KAS_TARGET_TGT_ARG_H
#define KAS_TARGET_TGT_ARG_H

#include "parser/token_parser.h"

namespace kas::tgt
{

using kas::parser::kas_token; 
struct token_missing  : kas_token {};
    
// declare immediate information type
struct tgt_immed_info
{
    uint8_t sz_bytes{};
    uint8_t flt_fmt {};
    uint8_t mask    {};
};


// NB: the `REG_T` & `REGSET_T` are just to allow lookup of type names
template <typename Derived, typename Mode_t, typename REG_T, typename REGSET_T = void>
struct tgt_arg_t : kas_token
{
    using base_t     = tgt_arg_t;
    using derived_t  = Derived;
    using arg_mode_t = Mode_t;

    // allow lookup of `reg_t` & `regset_t`
    using reg_t    = REG_T;
    using regset_t = REGSET_T;

    using op_size_t = core::opc::opcode::op_size_t;

    // expose required "MODES"
    static constexpr auto MODE_NONE  = arg_mode_t::MODE_NONE;
    static constexpr auto MODE_ERROR = arg_mode_t::MODE_ERROR;

    // x3 parser requires default constructable
    tgt_arg_t() : _mode(MODE_NONE) {}

    // error from const char *msg
    tgt_arg_t(const char *err, expr_t e = {})
            : _mode(MODE_ERROR), expr(e)
    {
        // create a `diag` instance
        auto& diag = parser::kas_diag::error(err, *this);
        this->err  = diag.ref();
    }
    
    // error from `kas_error_t`
    tgt_arg_t(parser::kas_error_t err) : _mode(MODE_ERROR), err(err) {}

protected:
    // simplify derived class ctors
    tgt_arg_t(arg_mode_t mode, expr_t e = {}) : _mode(mode), expr(e) {}

    // CRTP casts
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto constexpr& derived()
        { return *static_cast<derived_t*>(this); }

public:
    // arg mode: default getter/setter
    auto mode() const              { return _mode; }

    // error message for invalid `mode`. msg used by ctor only.
    const char *set_mode(unsigned mode)
    { 
        _mode = static_cast<arg_mode_t>(mode);
        return {};
    }

    // for validate_min_max: default implmentation
    bool is_missing() const { return _mode == MODE_NONE; }

    // helper method for evaluation of insn: default implementation
    bool is_const () const  { return false; }
    
    // validate methods
    template <typename...Ts>
    kas::parser::kas_error_t ok_for_target(Ts&&...) 
    {
        auto error = [this](const char *msg)
            {
                set_mode(MODE_ERROR);
                return err = kas::parser::kas_diag::error(msg, *this).ref();
            };

        // 0. if parsed as error, propogate
        if (mode() == MODE_ERROR)
        {
            // if not location-tagged, use arg location
            // ie. create new "reference" from diag using `this` as loc
            if (!err.get_loc())
                err = err.get().ref(*this);
            
            return err;
        }

        return {};
    }
    
    // emit methods: require derived implementation
    op_size_t size(int8_t sz, expression::expr_fits const& fits = {});

    // information about argument sizes
    // default:: single size immed arg, size = data_size, no float
    static constexpr tgt_immed_info sz_info[] = { sizeof(expression::e_data_t<>) };

    auto& immed_info(uint8_t sz) const
    {
        static constexpr auto sz_max = std::extent<decltype(derived_t::sz_info)>::value;

        if (sz >= sz_max)
            throw std::runtime_error{"immed_info::invalid sz: " + std::to_string(sz)};
        return derived_t::sz_info[sz];
    }

    template <typename MCODE_T>
    void emit(MCODE_T const& mcode, core::emit_base& base, unsigned size) const
    {
        if (size)
            base << core::set_size(size) << expr;
    }

    // serialize methods
    template <typename Inserter>
    bool serialize(Inserter& inserter, bool& completely_saved);
    
    template <typename Reader>
    void extract(Reader& reader, bool has_data, bool has_expr);

    // support methods
    void print(std::ostream&) const;
    static void reset()  {}
    
    // common member variables
    expr_t      expr {};
    parser::kas_error_t err; 
    
private:
    friend std::ostream& operator<<(std::ostream& os, tgt_arg_t const& arg)
    {
        arg.derived().print(os);
        return os;
    }

    arg_mode_t _mode {};
};

}

#endif
