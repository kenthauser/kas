#ifndef KAS_ARM_ARM_PARSER_SUPPORT_H
#define KAS_ARM_ARM_PARSER_SUPPORT_H

#include "arm_stmt.h"
#include "arm_arg.h"

namespace kas::arm::parser
{
// support types: help parsing `indirect` and `shift`

// parsed SHIFT formats
enum arm_shift_parsed
{
      ARM_SHIFT_LSL
    , ARM_SHIFT_LSR
    , ARM_SHIFT_ASR
    , ARM_SHIFT_ROR
    , ARM_SHIFT_RRX
    , NUM_ARM_SHIFT
};

enum arm_indir_op
{
      ARM_INDIR_REG         //  0
    , ARM_INDIR_REG_WB      //  1
    , ARM_PRE_INDEX         //  2
    , ARM_POST_INDEX        //  3: NB: all POST_INDEX has writeback
    , NUM_ARM_INDIRECT 
};

// support types to facilitate parsing
struct arm_shift_arg
{
    arm_shift_arg(int shift_op = {}) : shift_op(shift_op) {}

    // constructor
    template <typename Context>
    void operator()(Context const& ctx);
    
    operator arm_arg_t() const
    {
        arm_arg_t arg({expr, mode});
        arg.shift = shift;
        return arg;
    }

    uint8_t      shift_op;
    arm_arg_mode mode{MODE_SHIFT};
    expr_t       expr;      // for error
    arm_shift    shift;
};


struct arm_indirect_arg
{
    arm_indirect_arg(int op = {}) : op(op) {}

    // constructor
    template <typename Context>
    void operator()(Context const& ctx);

    operator arm_arg_t();
    
    arm_reg_t     base_reg;
    arm_reg_t     offset_reg;
    expr_t        offset;
    arm_shift     shift;
    bool          sign {};
    bool          write_back {};
    bool          is_reg {};
    arm_arg_mode  mode {MODE_REG_INDIR};
    uint8_t       op;
};

// support routine to encode parsed shift arg
template <typename Context>
void arm_shift_arg::operator()(Context const& ctx)
{
    auto make_error = [this](const char *, kas::parser::kas_loc loc = {})
        {
            mode = MODE_ERROR;
        };

    // arg list: expr or reg
    auto& arg = x3::_attr(ctx);

    // test if arg is `reg_t`
    constexpr auto is_reg_arg = std::is_assignable_v<arm_reg_t, decltype(arg)>;

    // RRX is special
    if (shift_op == ARM_SHIFT_RRX)
    {
        // type == 3, otherwise zeros
        shift.type = 3;
    }
    
    else if constexpr (is_reg_arg) 
    {
        // here for "register" shift
        shift.type   = shift_op;
        shift.is_reg = true;
            
        // validate index register
        arm_reg_t const& reg = arg;
        if (reg.kind(RC_GEN) == RC_GEN)
            shift.reg = reg.value(RC_GEN);
        else
            make_error("general register required", arg);
    }

    else
    {
        // here for "constant" shift
        shift.type   = shift_op;
        
        // validate constant shift
        expr_t const& e = arg;
        auto p          = e.get_fixed_p();
        if (!p)
            make_error("shift must be constant value", arg);
        else
        {
            // allow ASR & LSR shifts of 32 (map to zero)
            unsigned value = *p;
            if (shift_op == ARM_SHIFT_LSR || shift_op == ARM_SHIFT_ASR)
                if (value == 32)
                    value = 0;

            // translate "no shift" as LSR #0 (ie NONE)
            if (value == 0)
                shift.type = 0;

            if (value < 32)
                shift.ext = value;
            else
                make_error("shift out of range", arg);
        }
    }
    x3::_val(ctx) = *this;
}

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

// constuct `arm_arg` from parsed indirectd args
arm_indirect_arg::operator arm_arg_t() 
{
    arm_indirect  indir;    // FLAGS, REG, M, W, P, R 
    
    switch (op)
    {
        default:
            // should throw...
        case ARM_INDIR_REG:
            indir.flags = arm_indirect::P_FLAG;
            break;
        case ARM_INDIR_REG_WB:
            indir.flags = arm_indirect::P_FLAG;
            indir.flags = arm_indirect::W_FLAG;
            break;
        case ARM_PRE_INDEX:
            indir.flags = arm_indirect::P_FLAG;
            break;
        case ARM_POST_INDEX:
            indir.flags = arm_indirect::W_FLAG;
            break;
    }

    // handle `[r2, +--+-+6]`
    auto p = offset.get_fixed_p();
    if (p && *p < 0)
    {
        offset = -*p;
        sign = !sign;
    }

    if (write_back)
        indir.flags |= arm_indirect::W_FLAG;
    if (!sign)
        indir.flags |= arm_indirect::U_FLAG;
    if (is_reg)
    {
        indir.flags |= arm_indirect::R_FLAG;
        indir.reg    = offset_reg.value(RC_GEN);
    }
    if (shift)
        indir.flags |= arm_indirect::S_FLAG;

    // values sorted, so construct arg
    arm_arg_t arg({offset, mode});
    arg.reg   = base_reg;
    arg.shift = shift;
    arg.indir = indir;
    return arg;
}



}

#endif

