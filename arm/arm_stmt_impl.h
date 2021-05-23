#ifndef KAS_ARM_ARM_STMT_IMPL_H
#define KAS_ARM_ARM_STMT_IMPL_H

// implement `arm_stmt_t` methods overriden in CRTP derived class

#include "arm_stmt.h"
#include "arm_mcode.h"

namespace kas::arm
{

// validate parse of arm_stmt
template <typename Context>
void arm_stmt_t::operator()(Context const& ctx)
{
    static derived_t stmt;
    auto& x3_args = x3::_attr(ctx);

    // extract "x3" args
    auto& insn    = boost::fusion::at_c<0>(x3_args);
    stmt.args     = boost::fusion::at_c<1>(x3_args);
    
    // extract "insn" values from tuple
    stmt.insn_tok  = std::get<0>(insn);
    stmt.info      = std::get<1>(insn);
    auto& suffix   = std::get<2>(insn);

    sfx = suffix.value();
    
    // merge optional ldr/str suffix values into info
    switch (suffix.type)
    {
        case SFX_B: stmt.info.has_bflag = 1; break;
        case SFX_T: stmt.info.has_tflag = 1; break;
        case SFX_H: stmt.info.has_hflag = 1; break;
        case SFX_M: stmt.info.has_mflag = 1; break;
        default:
            // error...
            break;
    }
    x3::_val(ctx) = &stmt;
}

#if 1
uint8_t arm_stmt_info_t::sz(arm_mcode_t const&) const
{
    return {};
}
#endif
// Validate single MCODE supported by `TST` & `STMT_FLAGS`
auto arm_stmt_t::validate_stmt(mcode_t const *mcode_p) const
    -> tagged_msg 
{
    if (auto base_err = base_t::validate_stmt(mcode_p))
        return base_err;
#if 0
    // XXX m68k code
    auto sfx_code = mcode_p->defn().info & opc::SFX_MASK;
    
    // if size specified, validate it's supported
    if (info.arg_size != OP_SIZE_VOID)
    {
        if (!(mcode_p->defn().info & (1 << info.arg_size)))
            return { error_msg::ERR_bad_size, width_tok };
        
        // if suffix prohibited, error out
        if (sfx_code == opc::SFX_NONE::value)
            return { error_msg::ERR_sfx_none, width_tok };
    }

    // otherwise, validate "no size" is valid
    else
    {
        if (sfx_code == opc::SFX_NORMAL::value)
            return { error_msg::ERR_sfx_reqd, width_tok };
    }

    // validate "conditional" instructions
    if (mcode_p->defn().info & opc::SFX_CCODE_BIT::value)
    {
        // here ccode insn: require ccode suffix
        if (!info.has_ccode)
            return error_msg::ERR_ccode_reqd;

        // disallow T/F ccode if mcode disallows it 
        if (sfx_code == opc::SFX_NONE::value && !is_fp() && info.ccode < 2)
            return error_msg::ERR_no_cc_tf;
    }

    // don't allow condition codes on non-ccode insns
    else
    {
        if (info.has_ccode)
            return error_msg::ERR_no_ccode;
    }
    // XXX end m68k code
#endif
    return {};
}
// NB: This method rejects single `MCODE` not `STMT`
const char *arm_stmt_info_t::ok(arm_mcode_t const &mcode) const
{
    //std::cout << "validate_mcode: info = " << *this << std::endl;

    // check condition code, s-flag, and arch match MCODE & mode
    auto m_info = mcode.defn().info;
    if (has_ccode)
        if (~m_info & SZ_DEFN_COND)
            return "condition code not allowed";

    if (ccode == 0xe)
        if (m_info & SZ_DEFN_NO_AL)
            return "AL condition code not allowed";

    if (has_sflag)
        if (~m_info & SZ_DEFN_S_FLAG)
            return "s-flag not allowed";

    // T-flag allowed for `movT` & required for `ldrT`
    if (m_info & SZ_DEFN_REQ_T)
    {
        if (!has_tflag)
            return "t-flag required";
    }
    else if (has_tflag)
        if (~m_info & SZ_DEFN_T_FLAG)
            return "t-flag not allowed";


    if (has_bflag)
        if (~m_info & SZ_DEFN_B_FLAG)
            return "b-flag not allowed";

    if (m_info & SZ_DEFN_REQ_H)
        if (!has_tflag)
            return "h-flag required";

    if (m_info & SZ_DEFN_REQ_M)
        if (!has_tflag)
            return "m-flag required";

    return {};
}

void arm_stmt_info_t::print(std::ostream& os) const
{
    os << "[";
    if (has_ccode)
        os << "cc = " << std::dec << ccode;
    if (value() &~ INFO_CCODE_MASK)
    {
        if (has_ccode) os << ", ";
        os << "flags = ";
        if (has_sflag) os << "S";
        if (has_nflag) os << "N";
        if (has_wflag) os << "W";
        if (has_bflag) os << "B";
        if (has_tflag) os << "T";
        if (has_hflag) os << "H";
        if (has_mflag) os << "M";
    }
    
    os << "]";
};

}

#endif
