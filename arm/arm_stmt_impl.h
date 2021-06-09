#ifndef KAS_ARM_ARM_STMT_IMPL_H
#define KAS_ARM_ARM_STMT_IMPL_H

// implement `arm_stmt_t` methods overriden in CRTP derived class

#include "arm_stmt.h"
#include "arm_mcode.h"
#include "arm_stmt_flags.h"

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
    stmt.insn_tok = std::get<0>(insn);
    stmt.info     = std::get<1>(insn);

    x3::_val(ctx) = &stmt;
}

// Validate single MCODE for parsed STMT
auto arm_stmt_t::validate_stmt(mcode_t const *mcode_p) const
    -> tagged_msg 
{
    // XXX deal with stmt_t.sfx...

    return base_t::validate_stmt(mcode_p);
}

// NB: This method rejects single `MCODE` not `STMT`
// NB: Doesn't process flags associted with LDR/STR statements
const char *arm_stmt_info_t::ok(arm_mcode_t const &mcode) const
{
    //std::cout << "validate_mcode: info = " << *this << std::endl;

    // check condition code, s-flag, and arch match MCODE & mode
    auto m_info = mcode.defn().info;
    if (ccode == ARM_CC_OMIT)
        if (~m_info & SZ_DEFN_COND)
            return "condition code not allowed";

    if (ccode >= ARM_CC_ALL)    // _ALL, or _OMIT
        if (m_info & SZ_DEFN_NO_AL)
            return "AL condition code not allowed";

    if (has_sflag)
        if (~m_info & SZ_DEFN_S_FLAG)
            return "s-flag not allowed";
#if 0
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
#endif
    return {};
}

void arm_stmt_info_t::print(std::ostream& os) const
{
    constexpr const char *SEP_STR = ", ";
    const char *sep = "";
    os << "[";
   
    if (auto p = arm_ccode::name(ccode))
    {
        os << "cc = " << p;
        sep = SEP_STR;
    }

    if (sfx_code != 0)
    {
        os << sep << "sfx = " << arm_sfx_t::get_p(sfx_code)->name;
        sep = SEP_STR;
    }

    auto put_flag = [&](const char *flag)
        {
            if (sep)
                os << sep << "flags = ";
            sep = {};
            os << flag;
        };
    
    if (has_sflag) put_flag("S");
    if (has_nflag) put_flag("N");
    if (has_wflag) put_flag("W");
    
    os << "]";
};

}

#endif
