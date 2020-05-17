#ifndef KAS_TARGET_TGT_STMT_H
#define KAS_TARGET_TGT_STMT_H

// Boilerplate to allow `statement parser` to accept machine code insns
//
// format is regular:  opcode_name + [args]

#include "parser/parser_stmt.h"

#include <boost/fusion/include/at.hpp>
#include <vector>

namespace kas::tgt
{

// default a default "info" for arch's which don't derive any info
// from parsed stmt except for `name` and `args`
struct tgt_stmt_info_t
{
    static constexpr uint8_t value() { return 0; }  // no value to store

    // calculate size for insn
    template <typename MCODE_T>
    static constexpr uint8_t sz(MCODE_T const& mc) 
    {
        // default: retrieve size from definition
        return mc.defn().sz();
    }

    template <typename MCODE_T>
    static constexpr const char *ok(MCODE_T const& mc)
    {   
        return {};          // all mcodes match
    }

    void print(std::ostream& os) const
    {
        os << "*None*";
    }
    
    friend std::ostream& operator<<(std::ostream& os, tgt_stmt_info_t const& i)
    { i.print(os); return os; }
};


template <typename DERIVED_T, typename INSN_T, typename ARG_T>
struct tgt_stmt : kas::parser::parser_stmt<tgt_stmt<DERIVED_T, INSN_T, ARG_T>>
{
    using derived_t = DERIVED_T;
    using base_t    = tgt_stmt;
    using insn_t    = INSN_T;
    using mcode_t   = typename INSN_T::mcode_t;
    using arg_t     = ARG_T;


    using kas_error_t = parser::kas_error_t;

    // use default `info` as default `flags`
    tgt_stmt_info_t flags;

protected:
    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

public:
    auto& get_flags()       { return derived().flags; }

    auto& get_info()        { return derived().flags; }
    
    // method used to assemble instruction
    core::opcode *gen_insn(core::opcode::data_t&);

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {});

    // method to validate mcode suitable for stmt.
    // Principally for `hw_tst` & `stmt_info_t` validation
    constexpr const char *validate_stmt(mcode_t const *mcode_p) const { return {}; }
   
    // statement flags: variable data stored in opcode `name`: 
    // eg `ble` (branch if less-than-or-equal)
    // NB: not all architectures use `stmt_flags` to handle cases such as `ble`

    // methods used by test fixtures
    std::string name() const;

    void print_args(parser::print_obj const& p_obj) const
    {
        p_obj(args);
    }
    
    // X3 method to initialize instance
    template <typename Context>
    void operator()(Context const& ctx)
    {
        auto& x3_args = x3::_attr(ctx);
        insn_p        = boost::fusion::at_c<0>(x3_args);
        args          = boost::fusion::at_c<1>(x3_args);
        x3::_val(ctx) = derived();
    }

    insn_t const      *insn_p;
    std::vector<arg_t> args;
};


}

#endif
