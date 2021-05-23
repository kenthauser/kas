#ifndef KAS_TARGET_TGT_STMT_IMPL_H
#define KAS_TARGET_TGT_STMT_IMPL_H

// TGT_STMT_IMPL
//
// The principle `tgt_stmt` method is `gen_insn`. This method is invoked by `kas_core` to 
// generate an "instruction" given a "statement".
//
// To select the proper instruction, three "validators" are invoked. These are:
//
// 1) stmt::validate_args   This validator ensures that argument `registers` and addressing
//                          modes are valid for `target`.
// 
// 2) stmt::validate_mcode  This validator ensures `mcode` is valid for `target`. Also, all
//                          non-standard `stmt` information must be consumed.
//
// 3) mcode::validate_args  This validator is applied to all `mcodes` in `tgt_insn` to match
//                          arguments and mcodes. First pass is `OK/FAIL`. Second pass returns
//                          size of resulting object code. First, shortest, is selected.

#include "tgt_stmt.h"
#include "tgt_opc_quick.h"

// args is a container of "MCODE_T::arg_t" from comma-separated arguments
//
// Two special caes:
//  1)  if opcode has no arguments (eg "nop"), a single "MODE_NONE"
//      is created for op-code level error messages
//
// XXX target dependent and expected by validators
//  2)  bitfield arguments (the offset, width portion) are not separated
//      by commas, but are a separate "tgt_arg_t" instance.
namespace kas::tgt
{

using namespace kas::core::opc;

template <typename DERIVED_T, typename INSN_T, typename ARG_T, typename INFO_T>
auto tgt_stmt<DERIVED_T, INSN_T, ARG_T, INFO_T>::
        gen_insn(core::opcode::data_t& data) -> core::opcode *
{
    // get support types from `mcode`
    using mcode_t   = typename insn_t::mcode_t;

    using err_msg_t    = typename mcode_t::err_msg_t;
    using bitset_t     = typename mcode_t::bitset_t;
    using op_size_t    = typename mcode_t::op_size_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    // get kas types from opcode
    using core::opcode;
    auto trace  = opcode::trace;
    //trace = nullptr;
    trace = &std::cout;
    
    // convenience references
    auto& insn  = *insn_tok_t(insn_tok)();
    auto& fixed = data.fixed;
    
    // print name/args
    if (trace)
    {
        *trace << "tgt_stmt::eval: " << insn.name << " [" << insn.mcodes.size() << " opcodes]";
        for (auto& arg : args)
            *trace << ", " << arg;
        *trace << "  stmt_info: " << info << std::endl;
    }

    // validate args as appropriate for target
    // NB: also note if all args are "const" (ie: just registers & literals)
    // NB: "all const args" are candidates for `quick` format
    // NB: some target may have other requirements for `quick` opcode
    bool ok_for_quick = true;
    if (auto diag = derived().validate_args(insn, args, ok_for_quick, trace))
    {
        std::cout << "tgt_stmt::gen_insn: err = " << diag << std::endl;
        data.fixed.diag = diag;
        return {};              // nullptr xlated into opc_diag{}
    }

    // select mcode(s) matching args (normally only a single mcode))
    bitset_t ok;
    mcode_t const* matching_mcode_p {};
    bool multiple_matches = false;
    tagged_msg  err_msg  {};
    int         err_index{};

    // loop thru mcodes, recording first error & recording all matches
    int i = 0; 
    for (auto mcode_p : insn.mcodes)
    {
        if (trace)
            *trace << "validating: " << +i << ": ";

        tagged_msg  diag;
        int         cur_index{};

        // validate supported by arch & by info flags
        diag = derived().validate_stmt(mcode_p);
        
        // test if arguments match mcode
        if (!diag)
            std::tie(diag, cur_index) = mcode_p->validate_mcode(args, info, trace);
        
        if (trace)
        {
            if (diag)
                *trace << " -> " << diag.msg << std::endl;
            else
                *trace << " = OK" << std::endl;
        }

        if (!diag)
        {
            // match found. record in OK
            // also record mcode_p iff first matching
            ok.set(i);
            if (!matching_mcode_p)
                matching_mcode_p = mcode_p;
            else
                multiple_matches = true;
        }
        
        // diag: record best error message
        // best match matches most arguments
        else if (!err_msg || cur_index > err_index)
        {
            err_msg   = diag;
            err_index = cur_index;
        }
        
        ++i;        // next candidate mcode
    }
    
    if (trace)
        *trace << "result: " << ok.count() << " OK" << std::endl;

    // no match: generate message
    if (!matching_mcode_p)
    {
        if (err_index > args.size())
            err_index = args.size();

        // location required: initialize with default
        parser::kas_position_tagged const *loc_p = &insn_tok;

        // if `arg` errored, pick up location from arg
        if (err_index)
            loc_p = &args[err_index-1];

        // if `err_msg` tagged, use tagged location
        if (err_msg.pos_p)
            loc_p = err_msg.pos_p;

        // if `error` without msg (?!?) generate default
        if (!err_msg)
            err_msg = "X invalid instruction";

        data.fixed.diag = parser::kas_diag_t::error(err_msg.msg, *loc_p).ref();
        return {};
    }

    // multiple matches means no match
    if (multiple_matches)
        matching_mcode_p = {};

    // no longer need dummy "arg" for error messages.
    if (args.front().is_missing())
        args.clear();

    // set initial size from single insn or list
    expression::expr_fits fits;     // unitialized `fits` for intial eval
    if (matching_mcode_p)
        matching_mcode_p->size(args, info, data.size, fits, trace);
    else
    {
        // see if can resolve list to single instruction
        matching_mcode_p = insn.eval(ok, args, info, data.size, fits, trace);
        if (ok.count() != 1)
            matching_mcode_p = {};
    }

    // logic: if here, at least one match. 
    // 1) if constant `args`, emit binary code
    // 2) if single match, use format for selected opcode
    // 3) otherwise, use opcode for "list"

    // XXX force list or general during debug
    //matching_mcode_p = {};      // XXX force list for all
    //ok_for_quick   = {};      // XXX don't use quick

    std::cout << "quick format: " << std::boolalpha << ok_for_quick;
    std::cout << ", single match: " << !!matching_mcode_p << std::endl;
    
    // all const args: emit selected opcode
    if (ok_for_quick)
    {
        // must allocate static instance. 
        // NB: normally done in `tgt_fmt_opc_gen<>, but not appropriate for quick`
        static opc::tgt_opc_quick<mcode_t> opc_quick;
        opc_quick.proc_args(data, *matching_mcode_p, args, info);
        return &opc_quick;
    }

    // if `insn` not resolved to single `mcode`, use list
    if (!matching_mcode_p)
        matching_mcode_p = insn_t::list_mcode_p;

    return matching_mcode_p->fmt().get_opc().gen_insn(
                  insn
                , ok
                , *matching_mcode_p
                , std::move(args)
                , info

                // and core_opcode data area reference
                , data
                );
}

// test fixure routine to display statement name
template <typename DERIVED_T, typename INSN_T, typename ARG_T, typename INFO_T>
auto tgt_stmt<DERIVED_T, INSN_T, ARG_T, INFO_T>::
        name() const -> std::string
{
    using BASE_NAME = typename INSN_T::mcode_t::BASE_NAME;
    
    static constexpr auto name_prefix = string::str_cat<BASE_NAME, KAS_STRING(":")>::value;
    auto& insn  = *insn_tok_t(insn_tok)();
    return name_prefix + insn.name;
}

template <typename DERIVED_T, typename INSN_T, typename ARG_T, typename INFO_T>
template <typename ARGS_T, typename TRACE_T>
auto tgt_stmt<DERIVED_T, INSN_T, ARG_T, INFO_T>::
        validate_args(insn_t const& insn
                    , ARGS_T& args
                    , bool& ok_for_quick
                    , TRACE_T *trace
                    ) -> kas_error_t
{
    // if no mcodes, error was saved instead of mcode
    if (insn.mcodes.empty())
        return parser::kas_diag_t::error(insn.err(), insn_tok).ref();
    
    // if first is dummy, no args to check
    if (args.front().is_missing())
        return {};

    for (auto& arg : args)
    {
        // if floating point constant, require `floating point` insn
        if constexpr (!std::is_void_v<e_float_t>)
            if (auto p = arg.expr.template get_p<expression::e_float_t>())
                if (!derived().is_fp())
                    return arg.set_error(error_msg::ERR_float);

        // if not supported, return error
        if (auto diag = arg.ok_for_target(this))
            return diag;

        // test if constant & ok for `quick` format
        if (ok_for_quick)
            if (!arg.is_const())
                ok_for_quick = false;
    }
    
    return {};
}

// allow first arg to be `token` or `std::pair<token, INFO_T>`
template <typename DERIVED_T, typename INSN_T, typename ARG_T, typename INFO_T>
template <typename Context>
auto tgt_stmt<DERIVED_T, INSN_T, ARG_T, INFO_T>::
    operator()(Context const& ctx) -> void
{
    static derived_t stmt;
    auto& x3_args = x3::_attr(ctx);
    auto& insn    = boost::fusion::at_c<0>(x3_args);
    if constexpr (std::is_same_v<std::remove_reference_t<decltype(insn)>
                               , decltype(insn_tok)>)
    {
        // insn is `tok`
        stmt.insn_tok = insn;
    }
    else
    {
        // insn is `std::pair<tok, info>`
        stmt.insn_tok = insn.first;
        stmt.info     = insn.second;
    }
    
    stmt.args     = boost::fusion::at_c<1>(x3_args);
    x3::_val(ctx) = &stmt;
}

}
#endif
