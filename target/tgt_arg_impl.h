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

namespace kas::tgt
{

template <typename Derived, typename M, typename I, typename R, typename RS>
tgt_arg_t<Derived, M, I, R, RS>
        ::tgt_arg_t(arg_mode_t mode, kas_token const& tok) : tok(tok) 
{
    //std::cout << "arg_t::ctor mode = " << +mode << " expr = " << expr;
    //std::cout << " *this::loc = " << static_cast<parser::kas_loc>(*this) << std::endl;
    
    // accumulate error
    const char *msg   {};

#if 1
    auto reg_as_tok = reg_tok(tok);
    if (reg_as_tok)
        reg = *reg_as_tok();
    
    // `regset_t` can be void, so code slightly differently
    regset_t *rs_p  {};
    if constexpr (!std::is_void_v<rs_tok>)
    {
        auto rs_as_tok = rs_tok(tok);
        if (rs_as_tok)
            rs_p = rs_as_tok();
    }

    if (rs_p)
        std::cout << "tgt_arg::ctor: regset loc = " << rs_p->loc() << std::endl;

#else
    // see if `reg` or `regset` in expr
    reg_t *reg_p = expr.template get_p<reg_t>();
    if (reg_p)
    {
        reg = *reg_p;
        expr = {};
    }

    // `regset_t` can be void, so code slightly differently
    regset_t *rs_p  {};
    if constexpr (!std::is_void_v<regset_t>)
        rs_p = expr.template get_p<regset_t>();

    if (rs_p)
        std::cout << "tgt_arg::ctor: regset loc = " << rs_p->loc() << std::endl;
#endif

    // big switch to evaluate DIRECT/INDIRECT/IMMEDIATE
    switch (mode)
    {
    case arg_mode_t::MODE_DIRECT:
        if (reg_as_tok)
            mode = arg_mode_t::MODE_REG;
        else if (rs_p)
            mode = arg_mode_t::MODE_REGSET;
        break;
    
    case arg_mode_t::MODE_IMMEDIATE:
        // immediate must be non-register expression
        if (reg_as_tok || rs_p)
            msg = err_msg_t::ERR_argument;
        break;
#if 0
    case arg_mode_t::MODE_REGSET:
        if constexpr (!std::is_void<regset_t>::value)
            if (rs_p && rs_p->kind() != -regset_t::RS_OFFSET)
                break;
        
        msg = err_msg_t::ERR_argument;
        break;

    case arg_mode_t::MODE_REG_OFFSET:
        if constexpr (!std::is_void<regset_t>::value)
        {
            if (rs_p && rs_p->kind() == -regset_t::RS_OFFSET)
            {
                reg = rs_p->reg();
                expr = rs_p->offset();
                break;
            }
        }
        msg = err_msg_t::ERR_argument;

        break;
#endif
    case arg_mode_t::MODE_INDIRECT:
        // check for register indirect
        if (reg_as_tok)
        {
            mode = arg_mode_t::MODE_REG_INDIR;
            break;
        }
        
        // if not reg or regset, just indirect expression
        else if (!rs_p)
            break;
        
        // indirect regset. must be `MODE_REG_OFFSET`
        // Errors out if `rs_p` not RS_OFFSET
        mode = arg_mode_t::MODE_REG_OFFSET;
        
        // FALLSTHRU
    default:
#if 0
        // INDIRECT regset or non-standard mode. Check for RS_OFFSET
        // NB: must wrap regset test in `constexpr if`
        if constexpr (!std::is_void<regset_t>::value)
        {
            // need better interface to `RS_OFFSET`
            if (rs_p && rs_p->kind() == -regset_t::RS_OFFSET)
            {
                reg  = rs_p->reg();
                expr = rs_p->offset();
                break;
            }
        }
        
        // reg-sets must be DIRECT or RS_OFFSET. 
        if (rs_p)
            msg = err_msg_t::ERR_argument;
#endif   
        break;
    }

    // if !error, set mode
    if (!msg)
        msg = derived().set_mode(mode);
    
    // if error, generate `error` argument
    if (msg)
    {
        err = kas::parser::kas_diag_t::error(msg, *this).ref();
        set_mode(arg_mode_t::MODE_ERROR);
    }
}

// error message for invalid `mode`. msg used by ctor only.
template <typename Derived, typename M, typename I, typename R, typename RS>
const char *tgt_arg_t<Derived, M, I, R, RS>
                ::set_mode(unsigned mode)
{ 
    _mode = static_cast<arg_mode_t>(mode);
    if (wb_serial_p)
        (*wb_serial_p)(mode);

    return {};
}

// calculate size (for inserter)
template <typename Derived, typename M, typename I, typename R, typename RS>
int tgt_arg_t<Derived, M, I, R, RS>
            ::size(uint8_t sz, expression::expr_fits const *, bool *is_signed) const
{
    // default to unsigned
    if (is_signed) *is_signed = false;

    switch (mode())
    {
        case arg_mode_t::MODE_NONE:
        case arg_mode_t::MODE_ERROR:
        case arg_mode_t::MODE_IMMED_QUICK:
        case arg_mode_t::MODE_REG:
        case arg_mode_t::MODE_REG_INDIR:
            return 0;
        case arg_mode_t::MODE_DIRECT:
        case arg_mode_t::MODE_INDIRECT:
            return sizeof (typename expression::detail::e_addr<>::type);
        case arg_mode_t::MODE_REGSET:
#if 0
        ////
            if constexpr (!std::is_void<regset_t>::value_t)
                return sizeof(typename regset_t::value_t);
            return 0;
#endif
        // assume non-standard modes are a single word data
        default:
        case arg_mode_t::MODE_REG_OFFSET:
            if (is_signed) *is_signed = true;
            return sizeof(typename expression::detail::e_data<>::type);

        case arg_mode_t::MODE_IMMEDIATE:
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
                            parser::kas_diag_t::warning(err_msg_t::ERR_flt_fixed);
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
template <typename Derived, typename M, typename I, typename R, typename RS>
template <typename Inserter, typename WB>
bool tgt_arg_t<Derived, M, I, R, RS>
            ::serialize(Inserter& inserter, stmt_info_t const& info, WB *wb_p)
{
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero size or zero value
            auto p = tok.get_fixed_p();
            if ((p && !*p) || !size)
            {
                wb_p->has_data = false;     // supress write 
                return false;               // and no expression.
            }
            wb_p->has_data = true;    

#if 0
        // XXX 2020/02/18: `typename e_float_t...` fails because `e_float_t`
        // XXX is `void`, inside constexpr `!is_void` if. Find another solution.
            // if possibly `e_float_t` perform tests 
            if constexpr (!std::is_void_v<e_float_t>)
            {
                std::cout << "tgt_arg_t::serialize: " << tok << std::endl;
                using fmt = typename e_float_t::object_t::fmt;
                if (!p)
                    fmt::ok_for_fixed(tok.expr(), sz * 8);
            }
#endif
            // NB: can't `std::move`: tok.expr() returns const&
            return !inserter(tok.expr(), size);
        };
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // don't save if no register present
    if (!reg.valid())
        wb_p->has_reg = false;
    if (wb_p->has_reg)
        inserter(std::move(reg));
    
    // get size of expression
    if (wb_p->has_data)
        return save_expr(info.sz());

    // no-reg. no-data. no-expr.
    return false;
}

// handle all cases serialized above
// NB: `mode` set both *before* and *after* this method executed
// NB: `mode` first set to `init_mode` (value when serialized)
// NB: `mode` updated to `cur_mode` after deserialization completed
template <typename Derived, typename M, typename I, typename R, typename RS>
template <typename Reader>
void tgt_arg_t<Derived, M, I, R, RS>
            ::extract(Reader& reader, uint8_t sz, opc::detail::arg_serial_t *serial_p)
{
    using reg_tok = meta::_t<expression::token_t<reg_t>>;
    
    if (serial_p->has_reg)
    {
        // register stored as expression
        auto p = reader.get_expr().template get_p<reg_t>();
        if (!p)
            throw std::logic_error{"tgt_arg_t::extract: has_reg"};
        reg = *p;
    } 
    
    // read expression. Check for register
    if (serial_p->has_expr)
    {
        tok = reader.get_expr();

        // if expression holds register, process
        if (auto r_tok = reg_tok(tok))
        {
            reg = *r_tok(); // wierd syntax
            tok = {};
        }
    }

    // here has data, but not expression
    else if (serial_p->has_data)
    {
        bool is_signed {true};
        int  bytes = this->size(sz, {}, &is_signed);
        if (is_signed)
            bytes = -bytes;
        tok = (e_fixed_t)reader.get_fixed(bytes);
    }

    // save write-back pointer to serialized data
    wb_serial_p = serial_p;
}

// default implementation for `emit` argument.
// most likely needs to be overridden for all processor types
template <typename Derived, typename M, typename I, typename R, typename RS>
void tgt_arg_t<Derived, M, I, R, RS>
            ::emit(core::emit_base& base, uint8_t sz) const
{}

// default immediate arg `emit` routine
template <typename Derived, typename M, typename I, typename R, typename RS>
void tgt_arg_t<Derived, M, I, R, RS>
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
        // NB: a previous `fmt::ok_for_fixed` should have converted all inappropriate
        //     values to diagnoistics. Just throw if problem.
#if 0
        using flt_t = typename e_float_t::object_t;
        if (auto p = expr.template get_p<flt_t>())
        {
            // XXX warning float as fixed
            std::cout << "tgt_arg_t::emit_immed: " << expr << " emitted as integral value" << std::endl;
            base << core::set_size(info.sz_bytes) << flt_t::fmt::fixed(*p);
            return;
        }
#endif
    }
    base << core::set_size(info.sz_bytes) << tok.expr();
}
#if 0
// immediate arg floating point `emit` routine
template <typename Derived, typename arg_mode_t, typename reg_t, typename regset_t>
std::enable_if_t<!std::is_void_v<expression::e_float_t>>
void tgt_arg_t<Derived, arg_mode_t, reg_t, regset_t>
            ::emit_flt(core::emit_base& base, uint8_t bytes, uint8_t flt_fmt) const
{
    // get floating point `object` format (from `ref_loc_t`)
    using flt_t       = typename expression::e_float_t::object_t;
    using flt_value_t = typename flt_t::value_type;

    // the contortion below is because a temporary `flt_t` can be constructed
    // but not assigned. static's are no help, because they're only constructed once.
    auto p = expr.get_fixed_p();
    flt_t fixed_as_float { p ? static_cast<flt_value_t>(*p) : 0 };
    flt_t const *flt_p = expr.template get_p<flt_t>();
    
    if (!flt_p && p)
        flt_p = &fixed_as_float;    // fixed point formatted as floating point
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
#endif

// default print implementation
template <typename Derived, typename M, typename I, typename R, typename RS>
template <typename OS>
void tgt_arg_t<Derived, M, I, R, RS>
    ::print(OS& os) const
{
    switch (mode())
    {
        case arg_mode_t::MODE_DIRECT:
            os << tok;
            break;
        case arg_mode_t::MODE_INDIRECT:
            os << "(" << tok << ")";
            break;
        case arg_mode_t::MODE_IMMEDIATE:
        case arg_mode_t::MODE_IMMED_QUICK:
            os << "#" << tok;
            break;
        case arg_mode_t::MODE_REG:
            os << reg;
            break;
        case arg_mode_t::MODE_REG_INDIR:
            os << reg << "@";
            break;
#if 0
        case arg_mode_t::MODE_REG_OFFSET:
            os << reg << "@(" << tok << ")";
            break;
#endif
        case arg_mode_t::MODE_ERROR:
            if (err)
                os << err;
            else
                os << "Err: *UNDEFINED*";
            break;
        case arg_mode_t::MODE_NONE:
            os << "*NONE*";
            break;
        default:
            os << "[MODE:"  << std::dec << +mode();
            os << ",REG="   << reg;
            os << ",TOK="   << tok << "]";
            break;
    }
}
}

#endif

