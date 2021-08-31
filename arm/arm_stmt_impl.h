#ifndef KAS_ARM_ARM_STMT_IMPL_H
#define KAS_ARM_ARM_STMT_IMPL_H

// implement `arm_stmt_t` methods overriden in CRTP derived class

#include "arm_stmt.h"
#include "arm_mcode.h"
#include "arm_stmt_flags.h"

namespace kas::arm::parser
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


// NB: This method rejects single `MCODE` not `STMT`
// NB: Doesn't process flags associted with LDR/STR statements
const char *arm_stmt_info_t::ok(arm_mcode_t const &mcode) const
{
    //std::cout << "validate_mcode: info = " << *this << std::endl;

    // check condition code, s-flag, and arch match MCODE & mode
    auto m_info = mcode.defn().info;
    if (ccode == ARM_CC_OMIT)
        if (~m_info.flags & SZ_DEFN_COND)
            return "condition code not allowed";

    if (ccode >= ARM_CC_ALL)    // _ALL, or _OMIT
        if (m_info.flags & SZ_DEFN_NO_AL)
            return "AL condition code not allowed";

    if (has_sflag)
        if (~m_info.flags & SZ_DEFN_S_FLAG)
            return "S flag not allowed";

    // suffix tests should mirror defns in `arm_mcode.h`
    auto msg   = "suffix required";
    auto sfx_p = arm_sfx_t::get_p(sfx_index);
    auto flags = m_info.flags & SZ_DEFN_SFX_MASK;

    // if suffix present, try to "consume" it
    if (sfx_p)
    {
        switch (flags)
        {
            // consume if allowed
            case SZ_DEFN_B_FLAG: if (sfx_p->type == SFX_B) sfx_p = {}; break;
            case SZ_DEFN_T_FLAG: if (sfx_p->type == SFX_T) sfx_p = {}; break;

            // consume if required
            case SZ_DEFN_REQ_T:  if (sfx_p->type == SFX_T) msg = {}; break;
            case SZ_DEFN_REQ_H:  if (sfx_p->type == SFX_H) msg = {}; break;
            case SZ_DEFN_REQ_M:  if (sfx_p->type == SFX_M) msg = {}; break;
           
            // not consumed
            case 0: break;
            
            // keeping it honest
            // unknown flags -- raise configuration error
            default:
                return "INTERNAL ERROR: a7_info_flags: unknown value";
        }
    }

    // error if suffix and not consumed by above switch
    if (!msg)  return {};
    if (sfx_p) return "invalid suffix";

    // check if required suffix not present
    switch (flags)
    {
        case SZ_DEFN_REQ_T:
        case SZ_DEFN_REQ_H:
        case SZ_DEFN_REQ_M:
            return msg;
        default:
            break;
    }

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

    auto sfx_p = arm_sfx_t::get_p(sfx_index);
    if (sfx_p)
    {
        os << sep << "sfx = " << sfx_p->name;
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

    // suffix size...
    if (sfx_p)
    {
        if (auto sz = sfx_p->size)
            os << ", sz = " << arm_mcode_t::size_names[sz]; 
    }
    
    os << "]";
};

}



#endif
