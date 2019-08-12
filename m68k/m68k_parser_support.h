#ifndef KAS_M68K_PARSER_SUPPORT_H
#define KAS_M68K_PARSER_SUPPORT_H

//
// Complex instruction set processors are called "complex" for a reason.
//
// The MIT m68k indirect format allows great flexibility in specifying
// the operands. This code normalizes the various ways of specifying
// arguments into the standardized `m68k_arg_t` type.
//
// The MIT direct format uses '#', '@', '@+', '@-' prefixes/postfixes
// to inform interpretation of the single argument. The class
// `m68k_parsed_arg_t` processes the single argument (plus pre/postfix)
// arguments.
//
// The indirect formats are generally used to specify register indirect
// displacement and register indirect index operations. These are quite
// complex in their general form.
//
// 1. The indirect format allows an optionally suppressed base register
// (normally an address register or PC, but also on an 020+, a data register)
// to be named.
//
// 2. In addition to a base register, one or two additional "indirection"
// operations may be specified.
//
// 3. Each of the "indirection" operations may specify an optionally
// sized (word/long) and scaled (x1,2,4,8) "general register" (aka the
// index register) and a displacement. The displacement and register
// may be in either order, but each may only be specified once.
//
// 4. The "index" register may be named in either the first or second
// indirect specification, but not both. If a data register is used for
// the base register, it is implicitly added to the first index specification
// and the base register is suppressed.
//
// 5. Long multiply & divides can involve 64-bit operands/results which
// require two data registers: two registers separated by a `:`
//
// 6. Bit Field operators require an bit-offset and bitfield-width
// Offset & width can be constants (0-31) or data registers.
//
// Other complexities apply. Implementation issues include offset values
// which become zero (or go out of range) during evaluation. Not all formats
// are supported by all processors in the family.
//
// In other words...complex.


#include "m68k_arg.h"

#include "m68k_error_messages.h"
#include "expr/expr_fits.h"

#include <boost/fusion/include/adapt_struct.hpp>

namespace kas::m68k::parser
{
//
// Parser structures & values
//
enum { PARSE_MISSING, PARSE_DIR, PARSE_IMMED, PARSE_INDIR,
       PARSE_INCR, PARSE_DECR, PARSE_PAIR, PARSE_BITFIELD,

       // these are for internal moto arg processing
       PARSE_MOTO_INDEX, PARSE_MOTO_MEM, PARSE_INDIR_W, PARSE_INDIR_L,
       
       // these are are for coldfire MAC upper/lower register support...
       PARSE_SFX_W, PARSE_SFX_L, PARSE_SFX_U, NUM_PARSE_ENUM,
       PARSE_MASK = 0x20
   };

static_assert (NUM_PARSE_ENUM <= PARSE_MASK);

enum { P_SCALE_1 = 0, P_SCALE_2 = 1, P_SCALE_4 = 2, P_SCALE_8 = 3,
        P_SCALE_AUTO, P_SCALE_Z };

//
// structures to hold raw parsed values
//

// hold register/expression + scale + size arg
struct expr_size_scale
{
    expr_t  value;
    short   size;
    short   scale;

    friend std::ostream& operator<<(std::ostream& os, expr_size_scale const& e)
    {
        return os << e.value << ", size=" << e.size << ", scale=" << e.scale << " ";
    }
};

using m68k_disp_t = std::list<expr_size_scale>;

struct m68k_displacement
{
    int mode;
    std::list<m68k_disp_t> args;
};

struct m68k_parsed_arg_t
{
    expr_size_scale base;
    m68k_displacement mode;

    operator m68k_arg_t();
    m68k_arg_t indirect();
};

// type to classify parsed arguments
enum : uint32_t { E_UNKN, E_ERROR, E_INT, E_EXPR, E_DATA, E_ADDR, E_PC, E_ZADDR, E_ZPC, E_FLOAT, E_REG, E_REGSET };

struct m68k_classify_expr
{
    m68k_classify_expr(expr_t const& e) : expr(e) {}

    auto operator()()
    {
        if (!e_type)
            get_etype();
        return e_type;
    }

    auto value()
    {
        if (!e_type)
            get_etype();
        return _value;
    }
private:
    void get_etype()
    {
        if (auto ip = expr.get_fixed_p())
        {
            e_type = E_INT;
            _value = *ip;
        }
        else if (auto *rp = expr.get_p<m68k_reg_t>())
        {
            std::cout << "classify: kind = " << rp->kind() << std::endl;
            switch (rp->kind())
            {
            case RC_DATA:
                e_type = E_DATA;
                _value = rp->value();
                break;
            case RC_ADDR:
                e_type = E_ADDR;
                _value = rp->value();
                break;
            case RC_PC:
                e_type = E_PC;
                _value = -1;
                break;
            case RC_ZADDR:
                e_type = E_ZADDR;
                _value = 0;
                break;
            case RC_ZPC:
                e_type = E_ZPC;
                _value = -1;
                break;
            default:
                e_type = E_REG;
                break;
            }
        }
        else if (auto *fp = expr.get_p<expression::e_float_t>())
        {
            e_type = E_FLOAT;
        } 
        else if (auto *rs = expr.get_p<m68k_reg_set>())
        {
            e_type = E_REGSET;
        } 
        else
        {
            e_type = E_EXPR;
        }
    }

    expr_t const& expr;
    int           _value {};
    uint32_t      e_type {};
};



//
// Method to process the single argument (empty displacement) formats
//

m68k_parsed_arg_t::operator m68k_arg_t ()
{

#define TRACE_M68K_PARSE
#ifdef  TRACE_M68K_PARSE
    static const char * const parse_names[] = 
        { "MISSING", "DIR", "IMMED", "INDIR", "INCR", "DECR", "PAIR", "BITFIELD"
        , "MOTO_INDEX", "MOTO_MEM", "INDIR_W", "INDIR_L"
        , "SFX_W", "SFX_L", "SFX_U"
        };
    
    // print arg
    std::cout << "m68k_parsed_arg_t: mode = ";
    if (mode.mode < std::extent<decltype(parse_names)>::value)
        std::cout << parse_names[mode.mode] << std::endl;
    else
        std::cout << +mode.mode << std::endl;

    std::cout << "base: " << base;
    if (auto rp = base.value.get_p<m68k_reg_t>())
    {
        std::cout << " rc = " << +rp->kind() << " value = " << +rp->value();
        std::cout << ", ";
    }
    std::cout << "disp_lists: ";
    for (auto& arg_list : mode.args)
    {
        for (auto& arg : arg_list)
            std::cout << arg;
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif

    // BEGIN normalize to MIT format: ie: base is a register
    // MOTO may have base as Z_EXPR or constant. Modify as appropriate
    if (base.scale == P_SCALE_Z)
    {
        std::cout << "m68k_parsed_arg_t: P_SCALE_Z" << std::endl;
        auto& inner_list = mode.args.front();
        base = std::move(inner_list.front());
        inner_list.pop_front();
    }

    // END normalize to MIT

#ifdef  TRACE_M68K_PARSE
    std::cout << "norm: " << base;
    std::cout << "disp_lists: ";
    for (auto& arg_list : mode.args)
    {
        for (auto& arg : arg_list)
            std::cout << arg;
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif

    // check for the PARSE_MASK & save in reg_subword field.
    auto sub_reg = REG_SUBWORD_FULL;
    if (mode.mode & PARSE_MASK)
    {
        mode.mode &=~ PARSE_MASK;
        sub_reg    = REG_SUBWORD_MASK;
    }

    // use "indirect" method to process arguments with displacements
    // XXX but not PAIR nor BITFIELD
    if (!mode.args.empty() && !mode.args.front().empty())
        if (mode.mode != PARSE_PAIR &&
            mode.mode != PARSE_BITFIELD)
            return indirect();

    // if (mode.mode == PARSE_INDIR)
    //     return indirect();

    auto& base_value = base.value;
    // auto second;

    // direct parsed operands have a single "arg". See what it is.
    auto classify = m68k_classify_expr{base_value};
    auto kind = classify();

#ifdef TRACE_M68K_PARSE
    std::cout << "m68k_parsed_arg_t::m68k_arg_t: " << base << " = " << kind <<std::endl;
#endif
    // support routine to require addr register
    auto addr_only = [&](auto mode) -> m68k_arg_t
    {
        if (kind != E_ADDR)
            return { error_msg::ERR_no_addr_reg, base_value };
        m68k_arg_t r { mode, base_value };
        r.reg_num     = classify.value();
        r.reg_subword = sub_reg;
        return r;
    };

    switch (mode.mode)
    {
        default:
            // enable this while debugging...
            // throw std::runtime_error{"invalid mit::parse_mode"};
            // FALLSTHRU

        case PARSE_DIR:
        case PARSE_INDIR_W:       // XXX
        case PARSE_INDIR_L:
            switch (kind)
            {
                case E_DATA:
                {
                    m68k_arg_t r{ MODE_DATA_REG, base_value };
                    r.reg_num = classify.value();
                    r.reg_subword = sub_reg;
                    return r;
                }
                case E_ADDR:
                {
                    m68k_arg_t r{ MODE_ADDR_REG, base_value };
                    r.reg_num = classify.value();
                    r.reg_subword = sub_reg;
                    return r;
                }
                case E_EXPR:
                case E_INT:
                    return { MODE_DIRECT, base_value };
                case E_REG:
                {
                    m68k_arg_t r{ MODE_REG, base_value };
                    r.reg_num = classify.value();
                    r.reg_subword = sub_reg;
                    return r;
                }
                case E_REGSET:
                {
                    // is register set well formed? (ie: kind() >= 0)
                    m68k_arg_t r{ MODE_REGSET, std::move(base_value) };
                    if (auto rp = r.expr.get_p<m68k_reg_set>())
                        if (rp->kind() >= 0)
                            return r;
                    return { error_msg::ERR_regset, r.expr };
                }
                default:
                    return { error_msg::ERR_direct, base_value };
            }

        case PARSE_IMMED:
            switch (kind)
            {
                case E_EXPR:
                case E_INT:
                case E_FLOAT:
                    return { MODE_IMMED, base_value };
                default:
                    return { error_msg::ERR_immediate, base_value };
            }

        case PARSE_INDIR:
            switch (kind) 
            {
                case E_DATA:
                    // allow DATA as base if index_full provided
                    if (!hw::cpu_defs[hw::index_full{}])
                    {
                            // initialize index register
                            m68k_arg_t r { MODE_INDEX };
                            r.ext.reg_num       = classify.value();
                            r.ext.has_index_reg = true;
                            r.ext.base_suppr    = true;
                            return r;
                        }
                    // FALLSTHRU
                case E_ADDR:
                    return addr_only(MODE_ADDR_INDIR);
                case E_PC:
                    return { MODE_PC_DISP };
                default:
                    return { error_msg::ERR_indirect , base_value };
            }

        case PARSE_INCR:
            return addr_only(MODE_POST_INCR);
        case PARSE_DECR:
            return addr_only(MODE_PRE_DECR);
        case PARSE_MISSING:
            return { MODE_DIRECT, core::missing_ref{} };

        case PARSE_PAIR:
            {
                auto& inner = mode.args.front();
                if (inner.size() != 1)
                    return { error_msg::ERR_bad_pair, base_value };
                auto& second = inner.front();
                return { MODE_PAIR, base_value, second.value };
            }
        case PARSE_BITFIELD:
            {
                auto& inner = mode.args.front();
                if (inner.size() != 1)
                    return { error_msg::ERR_bad_bitfield, base_value };

                auto offset = classify.value();
                switch (kind)
                {
                    case E_DATA:
                    case E_EXPR:
                        break;
                    case E_INT:
                        if (offset >= 0 && offset <= 31)
                            break;
                        // FALLSTHRU
                    default:
                        return { error_msg::ERR_bad_offset, base_value };
                }

                auto& second = inner.front();

                auto classify_width = m68k_classify_expr{second.value};
                auto width = classify_width.value();

                switch (classify_width())
                {
                    case E_DATA:
                    case E_EXPR:
                        break;
                    case E_INT:
                        if (width >= 0 && width <= 31)
                            break;
                        // FALLSTHRU
                    default:
                        return { error_msg::ERR_bad_width, second.value };
                }
                return { MODE_BITFIELD, base_value, second.value };
            }
       
        // support motorola word/long expression specifier .w/.l
        // support coldfire upper/lower subregister mode. .u/.l
        case PARSE_SFX_W:
            switch (kind)
            {
                case E_EXPR:
                case E_INT:
                    // just absorb ".w"
                    return { MODE_DIRECT, base_value};
                default:
                    return { error_msg::ERR_direct, base_value };
            }
        case PARSE_SFX_L:
        case PARSE_SFX_U:
            switch (kind)
            {
                case E_DATA:
                {
                    m68k_arg_t r{ MODE_DATA_REG };
                    r.reg_num = classify.value();
                    r.reg_subword = (mode.mode == PARSE_SFX_L)
                                        ? REG_SUBWORD_LOWER 
                                        : REG_SUBWORD_UPPER;
                    return r;
                }
                case E_ADDR:
                {
                    m68k_arg_t r{ MODE_ADDR_REG };
                    r.reg_num = classify.value();
                    r.reg_subword = (mode.mode == PARSE_SFX_L)
                                        ? REG_SUBWORD_LOWER 
                                        : REG_SUBWORD_UPPER;
                    return r;
                }
                case E_EXPR:
                case E_INT:
                    // absorb ".l" meaning "long"
                    if (mode.mode == PARSE_SFX_L)
                        return { MODE_DIRECT, base_value };
                    // FALLSTHRU
                default:
                    return { error_msg::ERR_direct, base_value };
            }
    }
}

//
// Methods to process the indirect addressing formats
//
// `classify_indirect_arg`
// support routine to put "@(register,displacement)" into `index_arg`
//
// 1) register & displacement may be in either order
// [rule 2: choose 1 (via #define)]
// 2a) if both are omitted, set displacement size to ZERO
// 2b) either (but not both) may be omitted
// 3) neither may be repeated
// 4) register may have :w or :l size (but not :b)
// 5) register (but not displacement) may have scale
// 6) if displacement specified, calculate M_SIZE_*

template <typename It, typename IndexArg_t>
const char *classify_indirect_arg(It& it, IndexArg_t& idx, expr_t const * &error_p)
{
    expression::expr_fits fits;

    for (auto& arg : it)
    {
        auto classify = m68k_classify_expr{arg.value};
        auto kind = classify();

        error_p = &arg.value;

        switch (kind)
        {
        default:
            return error_msg::ERR_argument;

        case E_ADDR:
        case E_DATA:
            if (idx.reg_p)
                return error_msg::ERR_argument;
            
            if (arg.scale == P_SCALE_AUTO)
                arg.scale = P_SCALE_1;
            // test features for various index scaling;
            if (arg.scale != P_SCALE_1)
                if (auto err = hw::cpu_defs[hw::index_scale{}])
                    return err;

            // coldfire CPUs with FPU block also support scale_8
            if (arg.scale == P_SCALE_8)
                if (auto err = hw::cpu_defs[hw::index_scale_8{}])
                    if (hw::cpu_defs[hw::f_index_scale_8{}])
                        return err;
                        
            if (arg.size  == M_SIZE_WORD)
                if(auto err = hw::cpu_defs[hw::index_word{}])
                    return err;

            idx.reg_num = classify.value();
            if (kind == E_ADDR)
                idx.reg_num += 8;
            idx.reg_p = &arg.value;

            idx.index_scale = arg.scale;
            idx.index_size  = arg.size;
            break;

        case E_INT:
            // two ways to process empty displacement
#if 1
            // case 2a) above
            if (arg.scale == P_SCALE_Z)
                continue;       // completely ignore
#else
            // case 2b) above
            if (arg.scale == P_SCALE_Z)
                return "must specify displacement";
#endif
            // FALLSTHRU
        case E_EXPR:
            if (idx.expr_p)
                return error_msg::ERR_argument;

            // don't allow scale except on registers
            if (arg.scale != P_SCALE_AUTO)
                return hw::index_scale::name();

            idx.expr_p = &arg.value;

            // ignore size specifications on displacements: calculate instead
            if (kind != E_INT)
            {
                idx.expr_size = M_SIZE_LONG;
                break;
            }

            // check fixed value size
            auto value = classify.value();
            if (value == 0)
                idx.expr_size = M_SIZE_ZERO;
            else if (fits.fits<std::int16_t>(value) == fits.yes)
                idx.expr_size = M_SIZE_WORD;
            else
                idx.expr_size = M_SIZE_LONG;
            break;
        }
    }

    // return no error...
    return nullptr;
}

//
// Method to process indirect argument. These formats support
// M68K indirect displacement & indirect index formats
//
// input general format: base_reg@(register, displacement)@(...)
//
// Strategy is to put everything into `m68k_extension_t` instance.
// That type has a `bool()` method which returns "is index insn"
//
// 1) process base register. ADDR/PC types stored in `reg_num`
//    DATA/Z_ADDR/Z_PC stored in `m68k_extension_t`
// 2) process @(...) into `index_arg` instances: `inner` & `outer`
// 3) add `inner` and `outer` to `m68k_extension_t` instance
// 4) if bool() says no index: generate displacement insn
// 5) must be `MODE_INDEX` insn.
//

m68k_arg_t m68k_parsed_arg_t::indirect()
{
#ifdef TRACE_M68K_PARSE
    std::cout << "indirect()" << std::endl;
#endif
    // NORMALIZE TO MIT
    // if !register base, swap for next in `inner` list
    if (!base.value.get_p<m68k_reg_t>() && !mode.args.empty())
    {
        auto& inner_list = mode.args.front();
        inner_list.push_back(std::move(base));
        base = std::move(inner_list.front());
        inner_list.pop_front();
    }
    // END NORMALIZE

    // parsed index arg data
    struct index_arg
    {
        // save pointer to expressions
        expr_t const *reg_p   {};
        expr_t const *expr_p  {};
        int index_size  {};
        int index_scale {};
        int reg_num     {};
        int expr_size = M_SIZE_NONE;
    };

    // accumulate data in result
    m68k_arg_t result { MODE_INDEX };

    index_arg first_arg;
    index_arg second_arg;
    auto& base_value = base.value;
    expr_t const *error_p = &base_value;

    // analyze indirect arguments
    auto ip  = mode.args.begin();
    auto end = mode.args.end();

    // check first_arg
    auto err_msg = classify_indirect_arg(*ip++, first_arg, error_p);
    if (err_msg)
        return { err_msg, *error_p };

    // check second_arg if present
    if (ip != end)
        err_msg = classify_indirect_arg(*ip++, second_arg, error_p);
    if (err_msg)
        return { err_msg, *error_p };

    // limit is 2
    if (ip != end)
        return { error_msg::ERR_indirect, ip->front().value };

    // now analyze base register
    auto classify = m68k_classify_expr{base_value};
    auto kind = classify();
    switch (kind)
    {
        case E_ADDR:
            result.reg_num = classify.value();
            break;
        case E_PC:
            result.set_mode(MODE_PC_INDEX);
            break;

        // data indirect mapped to index w/ base_value_reg suppressed
        case E_DATA:
            first_arg.reg_num = classify.value();
            first_arg.reg_p   = &base_value;
            // FALLSTHRU
        case E_ZADDR:
        case E_ZPC:
            // testing here gives better error message...
            if (auto err = hw::cpu_defs[hw::index_full{}])
                return { err, base_value };

            // `INDEX` mode required because `base_suppr`
            result.ext.base_suppr = true;
            if (kind == E_ZPC)
                result.set_mode(MODE_PC_INDEX);
            break;
        default:
            return { error_msg::ERR_addr_mode, base_value };
    }

    // Here all arguments processed.
    // use `m68k_extension_t` instance to evaluate address mode.
    // first copy "expressions"
    if (first_arg.expr_p)
    {
        result.expr          = *first_arg.expr_p;
        result.ext.disp_size =  first_arg.expr_size;
    }

    // handle second expression (ie outer)
    if (second_arg.expr_p)
    {
        result.outer        = *second_arg.expr_p;
        result.ext.mem_size =  second_arg.expr_size;
    }

    // check if register in "inner"
    if (first_arg.reg_p)
    {
        result.ext.reg_num       = first_arg.reg_num;
        result.ext.reg_scale     = first_arg.index_scale;
        result.ext.reg_is_word   = first_arg.index_size == M_SIZE_WORD;
        result.ext.has_index_reg = true;
        std::cout << "m68k_parser: has_first: " << std::endl;
    }

    // check if register in "outer"
    if (second_arg.reg_p)
    {
        // Error check: only a single register allowed
        if (first_arg.reg_p)
            return { error_msg::ERR_indirect, *second_arg.reg_p };

        result.ext.reg_num       = second_arg.reg_num;
        result.ext.reg_scale     = second_arg.index_scale;
        result.ext.reg_is_word   = second_arg.index_size == M_SIZE_WORD;
        result.ext.has_index_reg = true;
        result.ext.is_post_index = true;
        
        // if no outer "displacement", set to zero
        if (result.ext.mem_size == M_SIZE_NONE)
            result.ext.mem_size  = M_SIZE_ZERO;
    }
#if 0
    // suppress illegal value of M_SIZE_NONE for inner
    // NB: value of `M_SIZE_NONE` reused to indicate `index brief` format
    if (first_arg.expr_size == M_SIZE_NONE)
        result.ext.disp_size = M_SIZE_ZERO;
#endif
    // check if `full index` mode required
    // no base_suppression, no outer expression, displacement is 8 bits
    expression::expr_fits fits;
    if (!result.ext.brief_ok() || fits.fits<std::int8_t>(result.expr) == fits.no)
        if (auto err = hw::cpu_defs[hw::index_full{}])
            return { err, *error_p };

    // it's illegal to suppress everything...
    if (result.ext.base_suppr
        && !result.ext.has_index_reg
        && !result.ext.outer()
        &&  result.ext.disp_size == M_SIZE_ZERO
        )
    {
        return { error_msg::ERR_addr_mode, *error_p };
    }

    // if index extension required, done.
    if (result.ext)
        return result;


    // look for the non-index patterns:
    // 1) ADDR with zero offset --> mode 2
    // 2) ADDR with word offset --> mode 5
    // 3) PC   with word offset --> mode 7-2
    // 4) Long or Auto offset handled by `MODE_ADDR_DISP_LONG`

    if (result.mode() == MODE_PC_INDEX)
    {
        result.set_mode(MODE_PC_DISP);
    } 
    else switch (result.ext.disp_size)
    {
        case M_SIZE_NONE:
        case M_SIZE_ZERO:
            result.set_mode(MODE_ADDR_INDIR);
            break;

        case M_SIZE_WORD:
            result.set_mode(MODE_ADDR_DISP);
            break; 
        default:
            result.set_mode(MODE_ADDR_DISP_LONG);
            break;
    }

    return result;
}

}

BOOST_FUSION_ADAPT_STRUCT(
kas::m68k::parser::m68k_parsed_arg_t,
base,
mode
)

BOOST_FUSION_ADAPT_STRUCT(
kas::m68k::parser::expr_size_scale,
value,
size,
scale
)

BOOST_FUSION_ADAPT_STRUCT(
kas::m68k::parser::m68k_displacement,
mode,
args
)

#endif
