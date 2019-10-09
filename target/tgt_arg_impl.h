#ifndef KAS_TARGET_TGT_ARG_IMPL_H
#define KAS_TARGET_TGT_ARG_IMPL_H

// default implementations for `tgt_arg_t` methods.
//
// The default constructor generates all default `arg_mode` types of
// arguments (except IMMED_QUICK). When combined with a smart `set_mode`
// it may be useful as is.
//
// The default serialization functions will probably prove suitable if the
// default ctor is suitable. 

// The default `print` uses a modified `m68k mit` sytax for `indirect register`
// and `register offset`. More of a placeholder until arch-specific replacement
// developed.

#include "tgt_arg.h"
#include "expr/format_ieee754.h"
#include "m68k/m68k_format_float.h"

namespace kas::tgt
{

template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
        ::tgt_arg_t(MODE_T mode, expr_t const& arg_expr) : expr(arg_expr)
{
    //std::cout << "arg_t::ctor mode = " << +mode << " expr = " << expr;
    //std::cout << " *this::loc = " << static_cast<parser::kas_loc>(*this) << std::endl;
    
    // accumulate error
    const char *msg   {};

    // see if `reg` or `regset` in expr
    REG_T *reg_p = expr.template get_p<REG_T>();
    if (reg_p)
    {
        reg = *reg_p;
        expr = {};
    }

    // `REGSET_T` can be void, so code slightly differently
    REGSET_T *rs_p  {};
    if constexpr (!std::is_void<REGSET_T>::value)
        rs_p = expr.template get_p<REGSET_T>();

    if (rs_p)
        std::cout << "tgt_arg::ctor: regset loc = " << rs_p->loc() << std::endl;

    // big switch to evaluate DIRECT/INDIRECT/IMMEDIATE
    switch (mode)
    {
    case MODE_T::MODE_DIRECT:
        if (reg_p)
            mode = MODE_T::MODE_REG;
        else if (rs_p)
            mode = MODE_T::MODE_REGSET;
        break;
    
    case MODE_T::MODE_IMMEDIATE:
        // immediate must be non-register expression
        if (reg_p || rs_p)
            msg = err_msg_t::ERR_argument;
        break;

    case MODE_T::MODE_REGSET:
        if constexpr (!std::is_void<REGSET_T>::value)
        {
            if (rs_p && rs_p->kind() != -REGSET_T::RS_OFFSET)
                break;
        }
        msg = err_msg_t::ERR_argument;
        break;

    case MODE_T::MODE_REG_OFFSET:
        if constexpr (!std::is_void<REGSET_T>::value)
        {
            if (rs_p && rs_p->kind() == -REGSET_T::RS_OFFSET)
            {
                reg = rs_p->reg();
                expr = rs_p->offset();
                break;
            }
        }
        msg = err_msg_t::ERR_argument;
        break;

    case MODE_T::MODE_INDIRECT:
        // check for register indirect
        if (reg_p)
        {
            mode = MODE_T::MODE_REG_INDIR;
            break;
        }
        
        // if not reg or regset, just indirect expression
        else if (!rs_p)
            break;
        
        // indirect regset. must be `MODE_REG_OFFSET`
        // Errors out if `rs_p` not RS_OFFSET
        mode = MODE_T::MODE_REG_OFFSET;
        
        // FALLSTHRU
    default:
        // INDIRECT regset or non-standard mode. Check for RS_OFFSET
        // NB: must wrap regset test in `constexpr if`
        if constexpr (!std::is_void<REGSET_T>::value)
        {
            // need better interface to `RS_OFFSET`
            if (rs_p && rs_p->kind() == -REGSET_T::RS_OFFSET)
            {
                reg  = rs_p->reg();
                expr = rs_p->offset();
                break;
            }
        }
        
        // reg-sets must be DIRECT or RS_OFFSET. 
        if (rs_p)
            msg = err_msg_t::ERR_argument;
        
        break;
    }

    // if !error, set mode
    if (!msg)
        msg = derived().set_mode(mode);
    
    // if error, generate `error` argument
    if (msg)
    {
        err = kas::parser::kas_diag_t::error(msg, *this).ref();
        set_mode(MODE_T::MODE_ERROR);
    }
}

// calculate size (for inserter)
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
int tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
            ::size(uint8_t sz, expression::expr_fits const *, bool *is_signed) const
{
    // default to unsigned
    if (is_signed) *is_signed = false;

    switch (mode())
    {
        case MODE_T::MODE_NONE:
        case MODE_T::MODE_ERROR:
        case MODE_T::MODE_IMMED_QUICK:
        case MODE_T::MODE_REG:
        case MODE_T::MODE_REG_INDIR:
            return 0;
        case MODE_T::MODE_DIRECT:
        case MODE_T::MODE_INDIRECT:
            return sizeof (typename expression::e_addr_t<>::type);
        case MODE_T::MODE_REGSET:
#if 0
        ////
            if constexpr (!std::is_void<REGSET_T>::value_t)
                return sizeof(typename REGSET_T::value_t);
            return 0;
#endif
        // assume non-standard modes are a single word data
        default:
        case MODE_T::MODE_REG_OFFSET:
            if (is_signed) *is_signed = true;
            return sizeof(typename expression::e_data_t<>::type);

        case MODE_T::MODE_IMMEDIATE:
    #if 0
            std::cout << "tgt_arg_t::size: immediate: " << std::showpoint << expr << std::endl;

            // special processing if `floating-point` support
            if constexpr (!std::is_void<expression::e_float_t>())
            {
                // if float as immed, check if floating or fixed format
                if (!derived().immed_info(sz).flt_fmt)
                    // if fixed format immed, check if floating value
                    if (auto p = expr.template get_p<expression::e_float_t>())
                    {
                        std::cout << "tgt_arg_t::size: immediate float" << std::endl;
                        auto n = derived().immed_info(sz).sz_bytes;

                        auto msg = expression::ieee754<expression::e_float_t>().ok_for_fixed(*p, n * 8);
                        if (msg)
                            set_error(msg);
                        else
                            parser::kas_diag_t::warning(m68k::err_msg_t::ERR_flt_fixed);
                    }
            }
#endif
            if (is_signed) *is_signed = true;
            return derived().immed_info(sz).sz_bytes;
    }
}

// save argument in serialized format
// NB: `serialize` can trash `arg` instance. 
// XXX ARG_INFO needs a "has_reg" flag
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
template <typename Inserter, typename ARG_INFO>
bool tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
            ::serialize(Inserter& inserter, uint8_t sz, ARG_INFO *info_p)
{
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero size or zero value
            auto p = expr.get_fixed_p();
            if ((p && !*p) || !size)
            {
                info_p->has_data = false;   // supress write 
                return false;               // and no expression.
            }
            info_p->has_data = true;    
            return !inserter(std::move(expr), size);
        };
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // don't save if no register present
    if (!reg.valid())
        info_p->has_reg = false;
    if (info_p->has_reg)
        inserter(std::move(reg));
    
    // get size of expression
    if (info_p->has_data)
        return save_expr(size(sz));

    // no-reg. no-data. no-expr.
    return false;
}

// handle all cases serialized above
// NB: `mode` set both *before* and *after* this method executed
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
template <typename Reader, typename ARG_INFO>
void tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
            ::extract(Reader& reader, uint8_t sz, ARG_INFO const *info_p, arg_wb_info *)
{
    if (info_p->has_reg)
    {
        // register stored as expression
        auto p = reader.get_expr().template get_p<REG_T>();
        if (!p)
            throw std::logic_error{"tgt_arg_t::extract: has_reg"};
        reg = *p;
    } 
    
    // read expression. Check for register
    if (info_p->has_expr)
    {
        expr = reader.get_expr();

        // if expression holds register, process
        if (auto p = expr.template get_p<REG_T>())
        {
            reg = *p;
            expr = {};
        }
    }

    // here has data, but not expression
    else if (info_p->has_data)
    {
        bool is_signed {true};
        int  bytes = this->size(sz, {}, &is_signed);
        if (is_signed)
            bytes = -bytes;
        expr = reader.get_fixed(bytes);
    }
}

// default immediate arg `emit` routine
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
void tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
            ::emit_immed(core::emit_base& base, uint8_t sz) const
{
    auto& info = immed_info(sz);
    
    // process floating point types if supported
    if constexpr (!std::is_void_v<e_float_t>)
    {
        // CASE: emit formatted as floating point
        if (info.flt_fmt)
            return derived().emit_flt(base, info.sz_bytes, info.flt_fmt);

        // CASE: emit floating point as fixed
        using flt_t = typename e_float_t::object_t;
        if (auto p = expr.template get_p<flt_t>())
        {
            // XXX warning float as fixed
            std::cout << "tgt_arg_t::emit_immed: " << expr << " emitted as integral value" << std::endl;
            base << core::set_size(info.sz_bytes) << typename flt_t::fmt().fixed(*p);
            return;
        }
    }
    base << core::set_size(info.sz_bytes) << expr;
}

// immediate arg floating point `emit` routine
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
void tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>
            ::emit_flt(core::emit_base& base, uint8_t bytes, uint8_t flt_fmt) const
{
    // get floating point `object` format (from `ref_loc_t`)
    using flt_t       = typename expression::e_float_t::object_t;
    using flt_value_t = typename flt_t::value_type;

    // the contortion below is because a temporary `flt_t` can be constructed
    // but not assigned. static's are no help, because they're only constructed once.
    auto p = expr.get_fixed_p();
    flt_t fixed{ p ? static_cast<flt_value_t>(*p) : 0 };
    flt_t const *flt_p = expr.template get_p<flt_t>();
    
    if (!flt_p && p)
        flt_p = &fixed;     // fixed point formatted as floating point
    else if (!flt_p)
    {
        // XXX should throw
        std::cout << "tgt_arg_t::emit_flt: " << expr << " is not rational constant" << std::endl;
        return;
    }
    
    auto [chunk_size, chunks, data_p] = typename flt_t::fmt().flt(*flt_p, flt_fmt);
    if (chunk_size == 0)
    {
        // XXX should throw
        if (data_p)
            std::cout << "tgt_arg_t::emit_flt: error: " << static_cast<const char *>(data_p) << std::endl;
        return;
    }
    base << core::emit_data(chunk_size, chunks) << data_p;
}


// default print implementation
template <typename Derived, typename MODE_T, typename REG_T, typename REGSET_T>
template <typename OS>
void tgt_arg_t<Derived, MODE_T, REG_T, REGSET_T>::print(OS& os) const
{
    switch (mode())
    {
        case MODE_T::MODE_DIRECT:
            os << expr;
            break;
        case MODE_T::MODE_INDIRECT:
            os << "(" << expr << ")";
            break;
        case MODE_T::MODE_IMMEDIATE:
        case MODE_T::MODE_IMMED_QUICK:
            os << "#" << expr;
            break;
        case MODE_T::MODE_REG:
            os << reg;
            break;
        case MODE_T::MODE_REG_INDIR:
            os << reg << "@";
            break;
        case MODE_T::MODE_REG_OFFSET:
            os << reg << "@(" << expr << ")";
            break;
        case MODE_T::MODE_ERROR:
            if (err)
                os << err;
            else
                os << "Err: *UNDEFINED*";
            break;
        case MODE_T::MODE_NONE:
            os << "*NONE*";
            break;
        default:
            os << "[MODE:"  << std::dec << +mode();
            os << ",REG="  << reg;
            os << ",expr=" << expr << "]";
            break;
    }
}
}

#endif

