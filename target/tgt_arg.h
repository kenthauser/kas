#ifndef KAS_TARGET_TGT_ARG_H
#define KAS_TARGET_TGT_ARG_H

#include "parser/token_defn.h"

namespace kas::tgt
{

// define "tokens" to hold parsed target CPU args
using parser::token_defn_t;
using parser::kas_token;

// forward declare writeback pointer
namespace opc::detail
{
    struct arg_serial_t;
}

// declare immediate information type
struct tgt_immed_info
{
    uint8_t sz_bytes;
    uint8_t flt_fmt;
    uint8_t mask;       // XXX not useful??. relocs used for sub-bytes
};

// NB: the `REG_T` & `REGSET_T` also to allow lookup of type names
template <typename Derived
        , typename MODE_T               // argument mode definitions
        , typename REG_T                // target register type
        , typename REGSET_T    = void   // target register set (or offset) type
        , typename STMT_INFO_T = void   // stmt immed size info (or void)
        >
struct tgt_arg_t : parser::kas_position_tagged
{
    using base_t        = tgt_arg_t;
    using derived_t     = Derived;
    using arg_mode_t    = MODE_T;

    using arg_serial_t  = typename opc::detail::arg_serial_t;

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
    tgt_arg_t(kas_position_tagged_t const& pos = {}) : kas_position_tagged_t(pos) {}

    // error from const char *msg
    tgt_arg_t(const char *err, kas_position_tagged_t const& pos = {})
            : _mode(MODE_ERROR), kas_position_tagged_t(pos)
    {
        // create a `diag` instance
        auto& diag = parser::kas_diag_t::error(err, *this);
        this->err  = diag.ref();
    }
    
    // error from `kas_error_t`
    tgt_arg_t(parser::kas_error_t err) : _mode(MODE_ERROR), err(err)
                                        , kas_position_tagged_t(err.get_loc()) {}

    // ctor(s) for default parser
    tgt_arg_t(std::pair<kas_token, MODE_T> const& p) :
                tgt_arg_t(p.second, p.first, p.first) {}

    tgt_arg_t(std::tuple<MODE_T, kas_token, kas_position_tagged_t> const& p)
            : tgt_arg_t(std::get<0>(p), std::get<1>(p), std::get<2>(p)) {}

    // declare primary constructor
    tgt_arg_t(arg_mode_t mode, kas_token const& tok, kas_position_tagged_t const& pos = {});

    // Access other instances. Used for `PREV` validator et.al.
    // NB: Define configuration hook if other than `array` used for instances
    static derived_t& prev(derived_t& obj, unsigned n)
    {
        return (&obj)[-n];
    }

protected:
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

    // validate argument
    kas::parser::kas_error_t ok_for_target(void const *stmt_p);
    
    // calculate size of extension data for argument (based on MODE & reg/expr values)
    int size(uint8_t sz, expression::expr_fits const *fits_p = {}, bool *is_signed = {}) const;

    // information about argument sizes
    // default:: single size immed arg, size = data_size, no float
    static constexpr tgt_immed_info sz_info[] = { sizeof(expression::detail::e_data<>) };

    static auto& immed_info(uint8_t sz)
    {
        static constexpr auto sz_max = std::extent<decltype(derived_t::sz_info)>::value;

        if (sz >= sz_max)
            throw std::runtime_error{"immed_info::invalid sz: " + std::to_string(sz)};
        return derived_t::sz_info[sz];
    }

    // serialize methods (use templated args instead of including all required headers)
    template <typename Inserter, typename WB_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, WB_INFO *wb_p, bool has_val);
    
    template <typename Reader>
    void extract(Reader& reader, uint8_t sz, arg_serial_t *, bool has_val);
   
    // number of additional bytes to serialize arg
    int8_t serial_data_size(uint8_t sz) const;

    // emit args as addresses or immediate args
    void emit      (core::core_emit& base, uint8_t sz) const;
    void emit_immed(core::core_emit& base, uint8_t sz) const;
    void emit_float(core::core_emit& base, tgt_immed_info const&) const;

    parser::kas_error_t set_error(const char *msg);

    // support methods
    template <typename OS>
    void print(OS&) const;
   
    // reset static variables for each insn.
    // sometimes ARGs depend on other ARGs. statics can be used to communicate
    static constexpr void reset()  {}
    
    // common member variables
    expr_t          expr     {}; 
    reg_t    const *reg_p    {};
    regset_t const *regset_p {};
    parser::kas_error_t err; 


protected:
    friend std::ostream& operator<<(std::ostream& os, tgt_arg_t const& arg)
    {
        arg.derived().print(os);
        return os;
    }

    arg_serial_t *wb_serial_p {};           // writeback pointer into serialized data
    arg_mode_t    _mode       { MODE_NONE };
};

// wrapper around std::vector (or array) of `args` to facitate looping.
// loop `ends` when encounters arg with `MODE_NONE` 

template <typename ARG_T, unsigned MAX_ARGS = 4>
struct tgt_argv_t
{
    using arg_t = ARG_T;
    static constexpr auto NUM_ARGS = MAX_ARGS;
    
    // use case: args extracted from insn_container
    tgt_argv_t() = default;
    
    // use case: args passed from parser
    tgt_argv_t& operator=(const std::vector<arg_t>& other)
    {
        // overflow processing:
        // if more than MAX_ARGS passed, copy MAX_ARGS+1 at most.
        // this allows assembler to detect too many args.
        // always add MODE_NONE arg sentinal to flag end

        unsigned index{};
        auto p = args.data();
        for (auto& arg : other)
        {
            *p++ = arg;
            if (index++ > MAX_ARGS)
                break;
        }
        *p++ = {};      // clear end
        return *this;
    }
#if 0
    // XXX use case: args passed from c-style array (eg: tgt_insn_serialize)
    // XXX ???
    template <unsigned N>
    tgt_argv_t& operator=(arg_t other[N])
    {
        // XXX deal with overflow. fix in general
        auto p = args.data();
        for (auto& arg : other)
            *p++ = arg;
        return *this;
    }
#endif
    // method to update the `arg.mode()` for all args
    // method is virtual to allow single interface for `parsed` argv
    // (via `arg.set_mode()`) and `extracted` argv (via writeback)
    // default: use `arg.mode()`
    virtual void update_modes(typename arg_t::arg_mode_t *modes) const
    {
        for (auto& arg : *this)
            arg.set_mode(*modes++);
    }
    
    // create an `iterator` to allow range-for to process sizes
    struct iter : std::iterator<std::forward_iterator_tag, arg_t>
    {
        iter(tgt_argv_t const& obj, bool make_begin = false) 
                : obj(obj)
                , index(make_begin ? 0 : -1)
                {}

        // range operations
        auto& operator++() 
        {
            ++index;
            return *this;
        }
        auto& operator*() const
        { 
            return obj[index];
        }
        auto operator!=(iter const& other) const
        {
            auto tst = obj[index].mode() == arg_t::MODE_NONE ? -1 : 0;
            return tst != other.index;
        }
   
    private:
        tgt_argv_t const& obj;
        int               index;
    };


    auto& operator[](unsigned n) const { return (argv)[n]; }
    auto& operator[](unsigned n)       { return (argv)[n]; }

    // methods to make `tgt_argv_t` act like standard container
    auto begin() const { return iter(*this, true); }
    auto end()   const { return iter(*this);       }
    
    auto& front()                const { return (*this)[0]; }
    auto& front()                      { return (*this)[0]; }

    void  clear()                      { front().set_mode(arg_t::MODE_NONE); }

    // types to make `tgt_argv_t` resolve as standard container
    using value_type = arg_t;
    using iterator   = iter;

private:
    // hold data for args
    std::array<arg_t, MAX_ARGS + 2> args;
    arg_t *argv {args.data()};
};

}

#endif
