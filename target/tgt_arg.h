#ifndef KAS_TARGET_TGT_ARG_H
#define KAS_TARGET_TGT_ARG_H

#include "parser/token_defn.h"

namespace kas::tgt
{

// define "tokens" to hold parsed target CPU args
using parser::token_defn_t;
using parser::kas_token;

// forward declare writeback pointer
namespace opc { namespace detail
{
    struct arg_serial_t;
}}

// declare immediate information type
struct tgt_immed_info
{
    uint8_t sz_bytes{};
    uint8_t flt_fmt {};
    uint8_t mask    {};
};

// NB: the `REG_T` & `REGSET_T` also to allow lookup of type names
template <typename Derived
        , typename MODE_T           // register mode definitions
        , typename STMT_INFO_T      // stmt immed size info (or void)
        , typename REG_T            // target register type
        , typename REGSET_T = void  // target register set (or offset) type
        >
struct tgt_arg_t : parser::kas_position_tagged
{
    using base_t        = tgt_arg_t;
    using derived_t     = Derived;
    using arg_mode_t    = MODE_T;

    // if `STMT_INFO_T` is void, use default type
    using stmt_info_t   = meta::if_<std::is_void<STMT_INFO_T>
                                  , struct tgt_stmt_info_t
                                  , STMT_INFO_T
                                  >;

    // allow lookup of `reg_t` & `regset_t`
    using reg_t    = REG_T;
    using regset_t = REGSET_T;
    
    // define reg & reg_set as tokens
    using reg_tok = meta::_t<expression::token_t<REG_T>>;
    using rs_tok  = meta::_t<expression::token_t<REGSET_T>>;

    using op_size_t = core::opc::opcode::op_size_t;

    // expose required "MODES"
    static constexpr auto MODE_NONE  = arg_mode_t::MODE_NONE;
    static constexpr auto MODE_ERROR = arg_mode_t::MODE_ERROR;

    // x3 parser requires default constructable
    tgt_arg_t(kas_token const& tok = {}) : kas_position_tagged_t(tok) {}

    // error from const char *msg
    tgt_arg_t(const char *err, kas_token const& token = {})
            : _mode(MODE_ERROR), kas_position_tagged_t(token)
    {
        // create a `diag` instance
        auto& diag = parser::kas_diag_t::error(err, *this);
        this->err  = diag.ref();
    }
    
    // error from `kas_error_t`
    tgt_arg_t(parser::kas_error_t err) : _mode(MODE_ERROR), err(err) {}

    // ctor(s) for default parser
    tgt_arg_t(std::pair<kas_token, MODE_T> const& p) : tgt_arg_t(p.second, p.first) {}
    tgt_arg_t(std::pair<MODE_T, kas_token> const& p) : tgt_arg_t(p.first, p.second) {}

protected:
    // declare primary constructor
    tgt_arg_t(arg_mode_t mode, kas_token const& tok);

    // CRTP casts
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto constexpr& derived()
        { return *static_cast<derived_t*>(this); }

public:
    // arg mode: default getter/setter
    auto        mode() const { return _mode; }
    const char *set_mode(unsigned mode);

    // save & restore arg "state" when evaluating mcodes. Default to just "mode".
    using arg_state = arg_mode_t;
    arg_state get_state() const                 { return derived().mode();   };
    void      set_state(arg_state const& state) { derived().set_mode(state); }; 

    // for validate_min_max: default implmentation
    bool is_missing() const { return _mode == MODE_NONE; }
    
    // for `inserter`: true if arg info not stored in opcode
    bool has_data() const;

    // helper method for evaluation of insn: default implementation
    // NB: `register` case is allowed because `expr` is zero
    bool is_const () const
    {
        return expr.get_fixed_p();
    }

    auto get_fixed_p() const
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
                return err = kas::parser::kas_diag_t::error(msg, *this).ref();
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
    static constexpr tgt_immed_info sz_info[] = { sizeof(expression::detail::e_data<>) };

    auto& immed_info(uint8_t sz) const
    {
        static constexpr auto sz_max = std::extent<decltype(derived_t::sz_info)>::value;

        if (sz >= sz_max)
            throw std::runtime_error{"immed_info::invalid sz: " + std::to_string(sz)};
        return derived_t::sz_info[sz];
    }

    // serialize methods (use templated args instead of including all required headers)
    template <typename Inserter, typename WB_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, WB_INFO *wb_p);
    
    template <typename Reader>
    void extract(Reader& reader, uint8_t sz, opc::detail::arg_serial_t *);

    // emit args as addresses or immediate args
    void emit      (core::emit_base& base, uint8_t sz) const;
    void emit_immed(core::emit_base& base, uint8_t sz) const;
    void emit_flt  (core::emit_base& base, uint8_t sz, uint8_t fmt) const;

    parser::kas_error_t set_error(const char *msg)
    {
        set_mode(MODE_ERROR);
        err = kas::parser::kas_diag_t::error(msg).ref();
        return err;
    }

    // support methods
    template <typename OS>
    void print(OS&) const;
   
    // reset static variables for each insn.
    // sometimes ARGs depend on other ARGs. `static`s can be used to communicate
    static void reset()  {}
    
    // common member variables
    expr_t          expr     {}; 
    reg_t    const *reg_p    {};
    regset_t const *regset_p {};
    parser::kas_error_t err; 

private:
    friend std::ostream& operator<<(std::ostream& os, tgt_arg_t const& arg)
    {
        arg.derived().print(os);
        return os;
    }

    opc::detail::arg_serial_t *wb_serial_p {};    // writeback pointer into serialized data
    arg_mode_t _mode          { MODE_NONE };
};

}

#endif
