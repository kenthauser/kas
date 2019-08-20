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


// NB: the `REG_T` & `REGSET_T` also to allow lookup of type names
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T = void>
struct tgt_arg_t : kas_token
{
    using base_t     = tgt_arg_t;
    using derived_t  = Derived;
    using arg_mode_t = MODE_T;
    using error_msg  = typename expression::err_msg_t<>::type;

    // allow lookup of `reg_t` & `regset_t`
    using reg_t    = REG_T;
    using regset_t = REGSET_T;

    // writeback data for arg update
    // NB: default should be void, but c++17 doesn't allow
    using arg_wb_info = uint8_t;

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

    // ctor(s) for default parser
    tgt_arg_t(std::pair<expr_t, MODE_T> const& p) : tgt_arg_t(p.second, p.first) {}
    tgt_arg_t(std::pair<MODE_T, expr_t> const& p) : tgt_arg_t(p.first, p.second) {}

protected:
    // declare primary constructor
    tgt_arg_t(arg_mode_t mode, expr_t const& e);

    // CRTP casts
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto constexpr& derived()
        { return *static_cast<derived_t*>(this); }

public:
    // arg mode: default getter/setter
    auto mode() const { return _mode; }

    // error message for invalid `mode`. msg used by ctor only.
    const char *set_mode(unsigned mode)
    { 
        _mode = static_cast<arg_mode_t>(mode);
        return {};
    }

    // for validate_min_max: default implmentation
    bool is_missing() const { return _mode == MODE_NONE; }

    // helper method for evaluation of insn: default implementation
    // XXX should also allow `reg_t`
    bool is_const () const
    {
        return expr.get_fixed_p();
    }

    bool is_immed () const
    {
        switch (mode())
        {
            case arg_mode_t::MODE_IMMEDIATE:
            case arg_mode_t::MODE_IMMED_QUICK:
                return true;
            default:
                return false;
        }
    }

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
    
    // calculate size of extension data for argument (based on MODE & reg/expr values)
    int size(uint8_t sz, expression::expr_fits const *fits_p = {}, bool *is_signed = {}) const;

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

    // serialize methods
    template <typename Inserter, typename ARG_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, ARG_INFO *info_p);
    
    template <typename Reader, typename ARG_INFO>
    void extract(Reader& reader, uint8_t sz, ARG_INFO const *, arg_wb_info *);

    // emit immediate value
    void emit_immed(core::emit_base& base, uint8_t sz ) const;
    void emit_flt  (core::emit_base& base, uint8_t fmt) const;

    // get/set state during relax: default is `mode` with no `info`
    struct arg_state
    {
        arg_mode_t  mode;
        arg_wb_info info;
    };
        
    arg_state get_state() const             { return { mode(), 0 }; }
    void set_state(arg_state  const& state) { set_mode(state.mode); }

    // support methods
    template <typename OS>
    void print(OS&) const;
   
    // reset static variables for each insn.
    // sometimes ARGs depend on other ARGs. `static`s can be used to communicate
    static void reset()  {}
    
    // common member variables
    expr_t      expr {};
    reg_t       reg  {}; 
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
