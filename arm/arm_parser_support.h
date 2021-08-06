#ifndef KAS_ARM_ARM_PARSER_SUPPORT_H
#define KAS_ARM_ARM_PARSER_SUPPORT_H

// already included by `arm_parser_def.h`, but utilized in file
#include "arm_stmt_flags.h"

namespace kas::arm::parser
{
using namespace x3;
using namespace kas::parser;

// ****** Support methods for parsing ADDRESSING MODES  ******

// parsed SHIFT formats
// NB: enums match ARM hardware definitions and can't be changed
enum arm_shift_parsed
{
      ARM_SHIFT_LSL
    , ARM_SHIFT_LSR
    , ARM_SHIFT_ASR
    , ARM_SHIFT_ROR
    , ARM_SHIFT_RRX
    , NUM_ARM_SHIFT
};

// NB: enumerated values mapped to W_FLAG & P_FLAG by method
enum arm_indir_op
{
      ARM_INDIR_REG         //  0
    , ARM_INDIR_REG_WB      //  1
    , ARM_PRE_INDEX         //  2
    , ARM_POST_INDEX        //  3: NB: POST_INDEX implies writeback
    , NUM_ARM_INDIRECT 
};

// support types to facilitate parsing
struct arm_shift_arg
{
    // NB: X3 requires objects in parser to default-init
    arm_shift_arg(int shift_op = {}) : shift_op(shift_op) {}

    // initializer routine, called as parser side-effect
    template <typename Context>
    void operator()(Context const& ctx);
    
    operator arm_arg_t() const
    {
        arm_arg_t arg({tok, mode});
        arg.shift = shift;
        return arg;
    }

    bool is_reg() const { return shift.is_reg; }

    void gen_shift_arg(kas_loc const&, int, arm_reg_t const * = {});

    void make_error(const char *msg, kas::parser::kas_loc loc)
    {
        mode = MODE_ERROR;
        //offset = parser::kas_diag_t::error(msg, loc);
    };

    uint8_t      shift_op;
    arm_arg_mode mode{MODE_SHIFT};
    kas_token    tok;      // for error
    arm_shift    shift;
};

struct arm_shift_arg_rrx : arm_shift_arg
{
    arm_shift_arg_rrx() {}

    template <typename Context>
    void operator()(Context const& ctx)
    {
        shift.type = 3;  // RRX looks like ROR #0
        x3::_val(ctx) = *this;
    }
};

// support routine to process parsed shift arg arguments
template <typename Context>
void arm_shift_arg::operator()(Context const& ctx)
{
    // extract argument list
    auto& parts    = x3::_attr(ctx);
    auto& has_hash = boost::fusion::at_c<0>(parts);
          tok      = boost::fusion::at_c<1>(parts);

    // test if arg is fixed value
    auto fixed_p = tok.get_fixed_p();

    // if fixed, process
    if (fixed_p)
        gen_shift_arg(tok, *fixed_p);
    else
    {
        // require fixed if `#` present
        if (has_hash && !fixed_p)
            make_error("Fixed value argument required", tok);

        else
        {
            // test if arg is register
            using tok_reg = typename arm_arg_t::reg_t::token_t;
            if (auto reg_p = tok_reg(tok)())
                gen_shift_arg(tok, 0, reg_p);

            // Error: neither register nor fixed
            else
                make_error("Invalid argument", tok);
        }
    }
    
    x3::_val(ctx)  = *this;
}

void arm_shift_arg::gen_shift_arg(kas_loc const& loc
                                , int count 
                                , arm_reg_t const *reg_p)
{
    //std::cout << "gen_shift: count = " << count;
    //if (reg_p)
    //    std::cout << reg_p->name();
    //std::cout << std::endl;

    // RRX is special
    if (shift_op == ARM_SHIFT_RRX)
    {
        // type == 3, shift == 0 
        shift.type = 3;
    }
    
    else if (reg_p) 
    {
        // here for "register" shift
        shift.type   = shift_op;
        shift.is_reg = true;
            
        // validate index register
        if (reg_p->kind(RC_GEN) == RC_GEN)
            shift.ext = reg_p->value(RC_GEN);
        else
            make_error("general register required", loc);
    }

    else
    {
        // here for "constant" shift
        shift.type   = shift_op;
        
        // translate "no shift" as LSR #0 (ie NONE)
        if (count == 0)
            shift.type = 0;

        // allow ASR & LSR shifts of 32 (map to zero)
        if (shift_op == ARM_SHIFT_LSR || shift_op == ARM_SHIFT_ASR)
            if (count == 32)
                count = 0;

        if (count < 32)
            shift.ext = count;
        else
            make_error("shift out of range", loc);
    }
    //std::cout << "gen_shift: type = " << +shift.type;
    //std::cout << ", is_reg = " << std::boolalpha << !!shift.is_reg;
    //std::cout << ", ext  = " << +shift.ext;
    //std::cout << " -> ";
    //shift.print(std::cout);
    //std::cout << std::endl;
}

struct arm_indirect_arg
{
    // validate parsed data
    template <typename Context>
    void operator()(Context const& ctx);

    // validate indirect base register
    void set_base(kas_token const& base);
    
    // convert parsed data to `arm_arg` format
    operator arm_arg_t();

    void make_error(const char *msg, kas::parser::kas_loc loc)
    {
        mode = MODE_ERROR;
        offset = parser::kas_diag_t::error(msg, loc);
    };

    arm_reg_t const *base_reg_p  {};
    kas_token        offset;
    arm_shift        shift;
    arm_indirect     indir;
    arm_arg_mode     mode {MODE_REG_INDIR};
};

template <typename Context>
void arm_indirect_arg::operator()(Context const& ctx)
{
    // indirect with offset zero. Get `write-back` flag
    auto& wb_flag  = x3::_attr(ctx);

    // for zero offset, set U_FLAG & P_FLAG
    indir.u_flag = true;
    indir.p_flag = true;

    // set W_FLAG according to write_back flag
    if (wb_flag)
        indir.w_flag = true;
    x3::_val(ctx)  = *this;
}

struct arm_indirect_post_index : arm_indirect_arg
{
    // valid parsed data
    template <typename Context>
    void operator()(Context const& ctx) 
    {
        // extract argument list
        auto& parts     = x3::_attr(ctx);
        auto& has_hash  = boost::fusion::at_c<0>(parts);
        auto& is_minus  = boost::fusion::at_c<1>(parts);
        auto& tok       = boost::fusion::at_c<2>(parts);
        auto& has_shift = boost::fusion::at_c<3>(parts);

        // for `post_index`, both P_FLAG & W_FLAG are cleared
        // leave flags as initialized
        // (for when method is driven from `arm_indirect_pre_index`)

        // set u_flag if not minus
        if (!is_minus) indir.u_flag = true;

        // test if arg is general register
        using tok_reg = typename arm_arg_t::reg_t::token_t;
        auto offset_reg_p = tok_reg(tok)();
        if (offset_reg_p)
        {
            if (offset_reg_p->kind() != RC_GEN)
                return make_error("general register required", tok);
            if (has_hash)
                return make_error("expression required", tok);
            indir.reg    = offset_reg_p->value();
            indir.r_flag = true;
        }
        else
            offset = tok;

        // if (shift) present, test if `immed` or `register` shift
        if (has_shift)
        {
            auto shift_arg = *has_shift;
            shift = has_shift->shift;
            if (shift.is_reg)
                return make_error("register shift not allowed", has_shift->tok);

            //std::cout << "arm_indirect: shift: ext = " << +shift.ext;
            //std::cout << ", type = " << +shift.type;
            //std::cout << ", is_reg = " << +shift.is_reg << std::endl;
        }

        x3::_val(ctx)  = *this;
    }

};

// process "offset" and "pre-indexed" formats
struct arm_indirect_pre_index : arm_indirect_post_index
{
    // valid parsed data
    template <typename Context>
    void operator()(Context const& ctx) 
    {
        // extract only wb_flag
        auto& parts    = x3::_attr(ctx);
        auto& wb_flag  = boost::fusion::at_c<4>(parts);

        // set P_FLAG & W_FLAG according to args
        // offset & pre-index set p_flag
        indir.p_flag = true;        // untouched by `...post_index`

        // pre-index also set w_flag
        if (wb_flag)
            indir.w_flag = true;
        
        // use `post_index` to process rest of args
        static_cast<arm_indirect_post_index>(*this)(ctx);
    }
};

void arm_indirect_arg::set_base(kas_token const& tok)
{
    // validate `base register` is general register
    using tok_reg = typename arm_arg_t::reg_t::token_t;
    base_reg_p = tok_reg(tok)();

    if (!base_reg_p || base_reg_p->kind() != RC_GEN)
        make_error("Base must be general register", tok);
}

struct gen_regset
{
    // extract types
    using ARG_T = arm_arg_t;
    using regset_t    = typename ARG_T::regset_t;
    using tok_reg     = typename ARG_T::reg_t::token_t;
    using tok_regset  = typename regset_t::token_t;

    template <typename Context>
    void operator()(Context const& ctx) const
    {
        // grab args & types from context (b_nodes is a std::vector<T>)
        auto& args    = x3::_attr(ctx);
        auto& initial = boost::fusion::at_c<0>(args);
        auto& b_nodes = boost::fusion::at_c<1>(args);

        //bool is_reg = initial.is_token_type(tok_reg());
        //std::cout << "gen_regset: is_reg = " << std::boolalpha << is_reg << std::endl;

        auto reg_p = tok_reg()(&initial);
        //std::cout << "gen_regset: initial = " << *reg_p << std::endl;
        auto& rs = regset_t::add(*reg_p);
        
        // parser ensures `initial` is a tok_reg
        //print_type_name("gen_regset::initial")(initial);
        //print_type_name("gen_regset::reg_p")(reg_p);

        //std::cout << "gen_regset: ";
        //rs.print(std::cout);
        //std::cout << std::endl;
        

        // use type system to combine registers -> register set
        // NB: type system ops are "-" for range, "/" to add
        // NB: precedence is range over add. Handled by `tgt_regset_t`

        for (auto&& node : b_nodes)
        {
            //print_type_name{"gen_regset::node"}(node.second);
            auto reg_p = tok_reg()(&node.second);
            //print_type_name{"gen_regset::node"}(reg_p);
            switch (node.first)
            {
#if 1
                case '-':   // range
                    rs.operator-(*reg_p);
                    //initial = initial - node.second;
                    break;
                case ',':   // add
                    rs.operator/(*reg_p);
                    //initial = initial / node.second;
#endif
                default:
                    break;  // XXX gen error message
            }
        }
   
        kas_token tok(rs);
        //tok.tag(initial.loc());
        x3::_val(ctx) = std::move(tok);
    }
};


#if 0
// support routine to encode indirect arg terms
// just init structure values. let `ctor` routine sort out values
template <typename Context>
void arm_indirect_arg::operator()(Context const& ctx)
{
    auto make_error = [this](const char *, kas::parser::kas_loc loc = {})
        {
            mode = MODE_ERROR;
        };

    // arg list: (2 formats)
    // 1. sign, expr_tok, write_back
    // 2. sign, reg_tok, optional(shift_arg), write_back
    auto& args = x3::_attr(ctx);

         sign = boost::fusion::at_c<0>(args);
    auto& arg = boost::fusion::at_c<1>(args);

    // test if base arg is `reg_t`
    constexpr auto is_reg_arg = std::is_assignable_v<arm_reg_t, decltype(arg)>;

    if constexpr (is_reg_arg) 
    {
        // here for "register" indirect
        is_reg = true;
            
        // validate index register
        arm_reg_t const& reg = arg;
        if (reg.kind(RC_GEN) == RC_GEN)
            offset_reg = reg;
        else
            make_error("general register required", arg);

        auto& opt_shift = boost::fusion::at_c<2>(args);
        if (opt_shift)
        {
            // propogate shift error & exclude `register` shift count
            auto& shift_arg = *opt_shift;
            if (shift_arg.mode == MODE_ERROR)
            {
                mode   = MODE_ERROR;
                offset = shift_arg.expr;
            }
            else
            {
                shift = shift_arg.shift;
                if (shift.is_reg)
                    make_error("register shift not allowed for memory access", arg);
            }
        }
        write_back = boost::fusion::at_c<3>(args);
    }

    else
    {
        // here for "offset" indirect
        offset = arg.value;
        write_back = boost::fusion::at_c<2>(args);
    }
    x3::_val(ctx) = *this;
}
#endif

// constuct `arm_arg` from parsed indirectd args
arm_indirect_arg::operator arm_arg_t() 
{
    // values sorted, so construct arg (NB: untagged)
    arm_arg_t arg({offset, mode});
    arg.reg_p = base_reg_p;
    arg.shift = shift;
    arg.indir = indir;
    return arg;
}


// ****** Support methods for parsing INFO  ******
//
// set "arm_stmt_info_t" flags based on condition codes & other insn-name flags
//
auto gen_stmt = [](auto& ctx)
    {
        // result is `stmt_t`
        arm_stmt_info_t info;
        
        auto& parts    = _attr(ctx);
        auto& insn_tok = boost::fusion::at_c<0>(parts);
        auto& ccode    = boost::fusion::at_c<1>(parts);
        auto& sfx      = boost::fusion::at_c<2>(parts);
        auto& s_flag   = boost::fusion::at_c<3>(parts);
        auto& nw_flag  = boost::fusion::at_c<4>(parts);

        // save parsed condition code
        if (ccode)
            info.ccode = *ccode;
        else
            info.ccode = arm_stmt_info_t::ARM_CC_OMIT;

        // save parsed suffix code in info
        if (sfx)
            info.sfx_index = (*sfx)->index();
        
        // save s-flag
        if (s_flag)
            info.has_sflag = true;

        // save `.N` and `.W`
        if (nw_flag)
            switch (*nw_flag)
            {
                case 'n': case 'N': info.has_nflag = true; break;
                case 'w': case 'W': info.has_wflag = true; break;
                default:    break; // parser should not generate;
            }

        _val(ctx) = {insn_tok, info};
    };

    
}

#endif
