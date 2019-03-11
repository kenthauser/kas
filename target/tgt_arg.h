#ifndef KAS_TARGET_TGT_ARG_H
#define KAS_TARGET_TGT_ARG_H

#include "parser/token_parser.h"

namespace kas::tgt
{

using kas::parser::kas_token; 
struct token_missing  : kas_token {};

template <typename Derived, typename Mode_t>
struct tgt_arg_t : kas_token
{
    using base_t     = tgt_arg_t;
    using derived_t  = Derived;
    using arg_mode_t = Mode_t;

    using op_size_t = core::opc::opcode::op_size_t;

    // expose required "MODES"
    static constexpr auto MODE_NONE  = arg_mode_t::MODE_NONE;
    static constexpr auto MODE_ERROR = arg_mode_t::MODE_ERROR;

    // x3 parser requires default constructable
    tgt_arg_t() : _mode(MODE_NONE) {}

    // error
    tgt_arg_t(const char *err, expr_t e = {})
            : _mode(MODE_ERROR), err(err), expr(e)
            {}

protected:
    // simplify derived class ctors
    tgt_arg_t(arg_mode_t mode, expr_t e = {}) : _mode(mode), expr(e) {}

public:
    // arg mode: default getter/setter
    auto mode() const              { return _mode; }
    void set_mode(arg_mode_t mode) { _mode = mode; }

    // for validate_min_max: default implmentation
    bool is_missing() const { return _mode == MODE_NONE; }

    // helper method for evaluation of insn: default implementation
    bool is_const () const  { return false; }
    
    // validate methods: require derived implmentation
    template <typename...Ts> const char *ok_for_target(Ts&&...) const;
    
    // emit methods: require derived implementation
    op_size_t size(expression::expr_fits const& fits = {});
    void emit(core::emit_base& base, unsigned size) const;

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
    const char *err  {};
    
private:
    friend std::ostream& operator<<(std::ostream& os, tgt_arg_t const& arg)
    {
        static_cast<derived_t const&>(arg).print(os);
        return os;
    }
    arg_mode_t _mode {};
};

}

#endif
