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

namespace kas::tgt
{

// constructor
template <typename Derived, typename M, typename I, typename R, typename RS>
tgt_arg_t<Derived, M, I, R, RS>
        ::tgt_arg_t(arg_mode_t mode, kas_token const& tok, kas_position_tagged_t const& pos) : kas_position_tagged_t(pos) 
{
    //std::cout << "arg_t::ctor mode = " << +mode << " expr = " << expr;
    //std::cout << " *this::loc = " << static_cast<parser::kas_loc>(*this) << std::endl;
   
    if (!static_cast<parser::kas_loc>(*this))
        static_cast<kas_position_tagged_t>(*this) = tok;
        
    // accumulate error
    const char *msg   {};
    
    // extract regset & register-set values
    reg_p = reg_tok(tok)();
    // `rs_tok` can be void, so code slightly differently
    if constexpr (!std::is_void_v<rs_tok>)
        regset_p = rs_tok(tok)();

#if 0
    if (reg_p)
        std::cout << "tgt_arg_t::ctor: reg = " << *reg_p << std::endl;
    if (regset_p)
        std::cout << "tgt_arg_t::ctor: regset = " << *regset_p << std::endl;
#endif

    // big switch to evaluate DIRECT/INDIRECT/IMMEDIATE
    switch (mode)
    {
    case arg_mode_t::MODE_DIRECT:
        if (reg_p)
            mode = arg_mode_t::MODE_REG;
        else if (regset_p)
        {
            // see if `regset` holds register set or offset
            if (regset_p->kind() >= 0)
            {
                // `regset` convert to expression
                mode = arg_mode_t::MODE_REGSET;
                expr = tok.expr();
                regset_p = {};
                break;
            }
        } 
        else
            expr = tok.expr();
        break;
    
    case arg_mode_t::MODE_IMMEDIATE:
        // immediate must be non-register expression
        if (reg_p || regset_p)
            msg = err_msg_t::ERR_argument;
        expr = tok.expr();
        break;

    case arg_mode_t::MODE_REGSET:
        if constexpr (!std::is_void<regset_t>::value)
            if (regset_p && regset_p->kind() != -regset_t::RS_OFFSET)
                break;
        
        msg = err_msg_t::ERR_argument;
        break;

    case arg_mode_t::MODE_REG_OFFSET:
        if constexpr (!std::is_void<regset_t>::value)
        {
            if (regset_p && regset_p->kind() == -regset_t::RS_OFFSET)
            {
                reg_p = regset_p->reg_p();
                expr = regset_p->offset();
                break;
            }
        }
        msg = err_msg_t::ERR_argument;

        break;

    case arg_mode_t::MODE_INDIRECT:
        // check for register indirect
        if (reg_p)
        {
            mode = arg_mode_t::MODE_REG_INDIR;
            break;
        }
        
        // if not reg or regset, just indirect expression
        else if (!regset_p)
        {
            expr = tok.expr();
            break;
        }
        // indirect regset. must be `MODE_REG_OFFSET`
        // Errors out if `regset_p` not RS_OFFSET
        if constexpr (!std::is_void_v<regset_t>)
        {
            // need better interface to `RS_OFFSET`
            if (regset_p && regset_p->is_offset())
            {
                mode     = arg_mode_t::MODE_REG_OFFSET;
                reg_p    = regset_p->reg_p();
                expr     = regset_p->offset();
                regset_p = {};
                break;
            }
        }
        
        // if not offset, error
        msg = err_msg_t::ERR_argument;
        break;
        
    default:
        // some other parsed "mode", evaluated in derived().set_mode()
        // default: just set mode & expr from passed values
        if (!reg_p)
            expr = tok.expr();
        break;
    }

    // if !error, allow derived type to process `set_mode`
    if (!msg)
        msg = derived().set_mode(mode);
    
    // if error, generate `error` argument
    if (msg)
    {
        err = kas::parser::kas_diag_t::error(msg, *this).ref();
        derived().set_mode(arg_mode_t::MODE_ERROR);
    }

    //std::cout << "tgt_arg_t::ctor: expr = " << expr << std::endl;
}

// error message for invalid `mode`. msg used by ctor only.
template <typename Derived, typename M, typename I, typename R, typename RS>
const char *tgt_arg_t<Derived, M, I, R, RS>
                ::set_mode(unsigned mode)
{ 
    _mode = static_cast<arg_mode_t>(mode);
    // XXX if (wb_serial_p)
    // XXX    (*wb_serial_p)(mode);

    return {};
}

template <typename Derived, typename M, typename I, typename R, typename RS>
auto tgt_arg_t<Derived, M, I, R, RS>
                ::set_error(const char *msg) -> parser::kas_error_t 
{
    set_mode(MODE_ERROR);
    err = kas::parser::kas_diag_t::error(msg, *this).ref();
    return err;
}

// validate argument
template <typename Derived, typename M, typename I, typename R, typename RS>
auto tgt_arg_t<Derived, M, I, R, RS>
                ::ok_for_target(void const *stmt_p) -> kas::parser::kas_error_t
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

    // 1. check if register is supported
    if (reg_p)
        if (auto err = reg_p->validate())
            return error(err);

    // 2. check for improper REGSET (ok syntax, but bad semantics)
    if constexpr (!std::is_void_v<regset_t>)
        if (regset_p)
            if (auto msg = regset_p->is_error())
                return error(msg);

    return {};
}


// calculate size (for inserter)
template <typename Derived, typename M, typename I, typename R, typename RS>
int tgt_arg_t<Derived, M, I, R, RS>
            ::size(uint8_t sz, expression::expr_fits const *, bool *is_signed_p) const
{
    // default to unsigned
    if (is_signed_p) *is_signed_p = false;

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
            return sizeof (expression::e_addr_t);
        case arg_mode_t::MODE_REGSET:
            return sizeof(expression::e_data_t);

        // assume non-standard modes are a single signed word
        case arg_mode_t::MODE_REG_OFFSET:
        default:
            if (is_signed_p) *is_signed_p = true;
            return sizeof(expression::e_data_t);

        case arg_mode_t::MODE_IMMEDIATE:
            if (is_signed_p) *is_signed_p = true;
            return derived().immed_info(sz).sz_bytes;
    }
}

template <typename Derived, typename M, typename I, typename R, typename RS>
auto tgt_arg_t<Derived, M, I, R, RS>
                ::serial_data_size(uint8_t sz) const -> int8_t
{ 
    // modes defined by `tgt` don't have additional info (execpt immed)
    switch (mode())
    {
        default:
            return sizeof(expression::e_data_t);
        case arg_mode_t::MODE_IMMED_QUICK:
            return 0;
        case arg_mode_t::MODE_IMMEDIATE:
            return derived().immed_info(sz).sz_bytes;
    }
}

#if 0
// true if extension data needed for `arg_info` 
template <typename Derived, typename M, typename I, typename R, typename RS>
auto tgt_arg_t<Derived, M, I, R, RS>
                ::has_data() const -> bool
//                ::has_arg_info() const -> bool
{ 
    return derived().size(0) && !expr.empty();
}
#endif

// save argument in serialized format
// NB: `serialize` can trash `arg` instance. 
// XXX ARG_INFO needs a "has_reg" flag
template <typename Derived, typename M, typename I, typename R, typename RS>
template <typename Inserter, typename WB>
bool tgt_arg_t<Derived, M, I, R, RS>
            ::serialize(Inserter& inserter, uint8_t sz, WB *wb_p)
{
    auto save_expr = [&](auto bytes) -> bool
        {
            // suppress writes of zero size or zero value
            auto p = get_fixed_p();
            if ((p && !*p) || !bytes)
            {
                wb_p->has_data = false;     // supress write 
                return false;               // and no expression.
            }
            return !inserter(std::move(expr), bytes);
        };
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // NB: all validators always save registers directly.
    // don't save if no register present
    if (!reg_p)
        wb_p->has_reg = false;
    else if (wb_p->has_reg)
        inserter(reg_p->index());


    // test for floating point immediate
    if (mode() == arg_mode_t::MODE_IMMEDIATE)
    {
        auto& info = derived().immed_info(sz);
        if (info.flt_fmt)
        {
            // if fixed point value, create floating point instance
            if (auto p = get_fixed_p())
                expr = e_float_t::add(*p, *this);
            return !inserter(std::move(expr));
        }
        // FALLSTHRU if not floating point, use integral processing
    }

    // get size of expression
    if (wb_p->has_data)
        return save_expr(derived().size(sz));   // calculate size in bytes

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
            ::extract(Reader& reader, uint8_t sz, arg_serial_t *serial_p)
{
    using reg_tok = meta::_t<expression::token_t<reg_t>>;
    
    if (serial_p->has_reg)
    {
        // register stored as index
        auto reg_idx = reader.get_fixed(sizeof(typename reg_t::reg_name_idx_t));
        reg_p = &reg_t::get(reg_idx);
    } 
    
    // read expression. Check for register
    if (serial_p->has_expr)
    {
        expr = reader.get_expr();
#if 0
        // if expression holds register, process
        if (auto r_tok = reg_tok(tok))
        {
            reg_p = r_tok(); // wierd syntax
            tok = {};
        }
#endif
    }

    // here has data, but not expression
    else if (serial_p->has_data)
    {
        bool is_signed {true};
        int  bytes = this->size(sz, {}, &is_signed);
        if (is_signed)
            bytes = -bytes;
        expr = (e_fixed_t)reader.get_fixed(bytes);
    }

    // save write-back pointer to serialized data
    wb_serial_p = serial_p;
}

// default implementation for `emit` argument.
// sizeof data emitted must match value returned from `tgt_arg_t::size()`
template <typename Derived, typename M, typename I, typename R, typename RS>
auto tgt_arg_t<Derived, M, I, R, RS>
            ::emit(core::core_emit& base, uint8_t sz) const -> void
{
    switch (auto m = mode())
    {
        case arg_mode_t::MODE_NONE:
        case arg_mode_t::MODE_ERROR:
        case arg_mode_t::MODE_IMMED_QUICK:
        case arg_mode_t::MODE_REG:
        case arg_mode_t::MODE_REG_INDIR:
            break;
        case arg_mode_t::MODE_DIRECT:
        case arg_mode_t::MODE_INDIRECT:
            base << core::set_size(sizeof(expression::e_addr_t));
            base << expr;
            break;
        case arg_mode_t::MODE_REGSET:
        case arg_mode_t::MODE_REG_OFFSET:
            base << core::set_size(sizeof(expression::e_data_t));
            base << expr;
            break;
        case arg_mode_t::MODE_IMMEDIATE:
            emit_immed(base, sz);
            break;
        default:
            if (m >= arg_mode_t::MODE_BRANCH &&
                m <  arg_mode_t::MODE_BRANCH + arg_mode_t::NUM_BRANCH)
            {
                // branch: number of "words" is based on delta from MODE_BRANCH
                // default implemetation: just guess & allow override
                // NB: don't know `mcode_t`, so guess based on `expression::e_data_t`
                using mcode_sz = typename expression::e_data_t;
                auto words = (m - arg_mode_t::MODE_BRANCH) << sizeof(mcode_sz);
                auto bytes = std::max<unsigned>(1, words);
                std::cout << "tgt_arg_t::emit: branch: expr = " << expr;
                std::cout << ", words = "  << words << std::endl;
                if (bytes)
                    base << core::emit_disp(bytes, -bytes) << expr;
            }
            break;
    }
}

// default immediate arg `emit` routine
template <typename Derived, typename M, typename I, typename R, typename RS>
void tgt_arg_t<Derived, M, I, R, RS>
            ::emit_immed(core::core_emit& base, uint8_t sz) const
{
    auto& info = immed_info(sz);

    // test if floating point format used
    // first test if floating point support is included
    if constexpr (!std::is_void_v<e_float_t>)
        if (info.flt_fmt)
            return derived().emit_float(base, info);
    
    base << core::set_size(info.sz_bytes) << expr;
}

// immediate arg floating point `emit` routine
template <typename Derived, typename M, typename I, typename R, typename RS>
std::enable_if_t<!std::is_void_v<expression::e_float_t>>
tgt_arg_t<Derived, M, I, R, RS>
            ::emit_float(core::core_emit& base, tgt_immed_info const& info) const
{
    // common routine to format and emit
    auto do_emit = [&](e_float_t const& flt)
        {
            auto [chunk_size, chunks, data_p] = flt.format(info.flt_fmt);
            base << core::emit_data(chunk_size, chunks) << data_p;
        };
  
    // can't copy `e_float_t`, so pass by reference
    // NB: construct float from `e_fixed_t` if required
    if (auto p = expr.template get_p<e_float_t>())
        do_emit(*p);
    else if (auto p = expr.get_fixed_p())
        do_emit(*p);
    else
    {
        // XXX should emit diagnostic
        std::cout << "tgt_arg_t::emit_float: " << expr << " is not rational value" << std::endl;
    }
}

// default print implementation
template <typename Derived, typename M, typename I, typename R, typename RS>
template <typename OS>
void tgt_arg_t<Derived, M, I, R, RS>
    ::print(OS& os) const
{
    switch (mode())
    {
        case arg_mode_t::MODE_DIRECT:
            os << expr;
            break;
        case arg_mode_t::MODE_INDIRECT:
            os << expr << "@";
            break;
        case arg_mode_t::MODE_IMMEDIATE:
        case arg_mode_t::MODE_IMMED_QUICK:
            os << "#" << expr;
            break;
        case arg_mode_t::MODE_REG:
            os << *reg_p;
            break;
        case arg_mode_t::MODE_REG_INDIR:
            os << *reg_p << "@";
            break;
        case arg_mode_t::MODE_REG_OFFSET:
            if constexpr (!std::is_void_v<rs_tok>)
                os << *reg_p << "@(" << expr << ")";
            else
                os << "Internal: tgt_arg_t::print:: Invalid MODE_REG_OFFSET";
            break;
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
            if (reg_p)
                os << ",REG=" << *reg_p;
            if (!expr.empty())
                os << ",EXPR=" << expr;
            os << "]";
            break;
    }
}
}

#endif

