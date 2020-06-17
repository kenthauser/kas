#ifndef KAS_M68K_M68K_STMT_IMPL_H
#define KAS_M68K_M68K_STMT_IMPL_H

#include "m68k_stmt.h"
#include "m68k_mcode.h"

namespace kas::m68k
{

template <typename Context>
void m68k_stmt_t::operator()(Context const& ctx)
{
    // X3 method to initialize instance
    auto& x3_args = x3::_attr(ctx);
    auto& stmt    = boost::fusion::at_c<0>(x3_args);
    stmt.args     = boost::fusion::at_c<1>(x3_args); 
    x3::_val(ctx) = stmt;
} 

// Validate ARGS supported by target & floating-point status
template <typename ARGS_T, typename TRACE_T>
auto  m68k_stmt_t::validate_args(insn_t const& insn
                    , ARGS_T& args
                    , bool& ok_for_quick
                    , TRACE_T *trace
                    ) const -> kas_error_t
{
    if (insn.mcodes.empty())
    {
        return parser::kas_diag_t::error(m68k::hw::cpu_defs[insn.tst], args.front()).ref();
    }
    
    // if first is dummy, no args to check
    if (args.front().is_missing())
        return {};

    // get arg size from flags
    // NB: size can be `OP_SIZE_VOID`
    auto sz = flags.arg_size;
    
    for (auto& arg : args)
    {

        // if floating point arg, require `floating point` insn
        if (auto p = arg.template get_p<expression::e_float_t>())
            if (!is_fp())
                arg.set_error(error_msg::ERR_float);

        // if not supported, return error
        // NB: must be last: may `loc_tag` other errors
        if (auto diag = arg.ok_for_target(sz))
            return diag;

        // test if constant & ok for `quick` format
        if (ok_for_quick)
        {
            if (!arg.is_const())
                ok_for_quick = false;
            else if (arg.is_immed() && is_fp())
                ok_for_quick = false;
        }
    }
    
    return {};
}

// Validate single MCODE supported by `TST` & `STMT_FLAGS`
const char *m68k_stmt_t::validate_stmt(mcode_t const *mcode_p) const
{
    if (auto base_err = base_t::validate_stmt(mcode_p))
        return base_err;

    auto sfx_code = mcode_p->defn().info & opc::SFX_MASK;
    
    // if size specified, validate it's supported
    if (flags.arg_size != OP_SIZE_VOID)
    {
        if (!(mcode_p->defn().info & (1 << flags.arg_size)))
            return error_msg::ERR_bad_size;
        
        // if suffix prohibited, error out
        if (sfx_code == opc::SFX_NONE::value)
            return error_msg::ERR_sfx_none;
    }

    // otherwise, validate "no size" is valid
    else
    {
        if (sfx_code == opc::SFX_NORMAL::value)
            return error_msg::ERR_sfx_reqd;
    }

    // validate "conditional" instructions
    if (mcode_p->defn().info & opc::SFX_CCODE_BIT::value)
    {
        // here ccode insn: require ccode suffix
        if (!flags.has_ccode)
            return error_msg::ERR_ccode_reqd;

        // disallow T/F ccode if mcode disallows it 
        if (sfx_code == opc::SFX_NONE::value && !is_fp() && flags.ccode < 2)
            return error_msg::ERR_no_cc_tf;
    }

    // don't allow condition codes on non-ccode insns
    else
    {
        if (flags.has_ccode)
            return error_msg::ERR_no_ccode;
    }

    return {};
}

// Handle instruction with no suffix but implied size.
// Think "link" and "trap".
// Actual mcode size is extracted from `mcode.defn()`
uint8_t m68k_stmt_info_t::sz(m68k_mcode_t const& mc) const
{
    auto sz = arg_size;
    if (sz == OP_SIZE_VOID)
    {
        // if void, check if single size specified
        auto defn_sz = mc.defn().info & 0x7f;

        // don't bother with switch
        if (defn_sz == (1 << OP_SIZE_LONG))
            sz = OP_SIZE_LONG;
        else if (defn_sz == (1 << OP_SIZE_WORD))
            sz = OP_SIZE_WORD;
        else if (defn_sz == (1 << OP_SIZE_BYTE))
            sz = OP_SIZE_BYTE;
    }
    return sz;
}

// Validate instruction supports suffix
const char *m68k_stmt_info_t::ok(m68k_mcode_t const& mc) const
{
    auto defn_sz = mc.defn().info & 0x7f;
    return {};
}

void m68k_stmt_info_t::print(std::ostream& os) const
{
    auto sz = m68k_sfx::suffixes[arg_size];
    if (!sz)
        sz = 'v';
    os << "sz = " << sz;
    
    if (has_ccode)
        os << " ccode = " << m68k_ccode::name(ccode, fp_ccode);
}

}

#endif
