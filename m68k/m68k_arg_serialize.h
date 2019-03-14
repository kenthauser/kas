#ifndef KAS_M68K_M68K_ARG_SERIALIZE_H
#define KAS_M68K_M68K_ARG_SERIALIZE_H

// serialize a single argument
//
// for each argument, save the "mode", register number, and data
//
// If possible, save register number (and mode) in actual "opcode" data.
// If needed, save extra data.
// Data can be stored as "fixed" constant, or "expression" 

#include "m68k_arg.h"
//#include "m68k_formats_type.h"
//#include "m68k_data_inserter.h"
#include "kas_core/opcode.h"

//#include "expr/expr_fits.h"     // for INDEX_BRIEF


namespace kas::m68k
{
#if 0
namespace detail {
    using m68k_op_size_t = m68k_ext_size_t;
    using fixed_t = typename core::opcode::fixed_t;

    // Special (word aligned) type to hold info about an m68k argument
    // in the insn_data deque(). The info for each "arg" is 8-bits
    // some of which indicate extension words follow.

    // arg_mode holds the `m68k_arg_mode_enum`
    // `has_*_expr` is true if argument is `expr_t`, false for fixed
    struct arg_info_t {
        static constexpr std::size_t MODE_FIELD_SIZE = 6;
        static_assert(NUM_ARG_MODES <= (1<< MODE_FIELD_SIZE)
                    , "too many `m68k_arg_mode` enums");

        uint8_t arg_mode : MODE_FIELD_SIZE;
        uint8_t has_data : 1;       // additional data stored
        uint8_t has_expr : 1;       // data stored as expression
    };

    // store info in pairs
    struct m68k_arg_info : kas::detail::alignas_t<m68k_arg_info, m68k_ext_size_t>
    {
        constexpr static auto ARGS_PER_INFO = sizeof(m68k_ext_size_t)/sizeof(arg_info_t);
        // room for fist two arguments in "extension" word
        arg_info_t  info[ARGS_PER_INFO];
    };
    static_assert (sizeof(m68k_arg_info) <= sizeof (m68k_ext_size_t));


    // what additional data to store for each arg "MODE"
    inline auto m68k_arg_data_size(int mode)
    {
        switch (mode)
        {
            default:
                return M_SIZE_NONE;

            case MODE_INDEX:
            case MODE_PC_INDEX:
                return M_SIZE_INDEX;

            case MODE_INDEX_BRIEF:
            case MODE_BITFIELD:
                return M_SIZE_UWORD;

            case MODE_DIRECT:
            case MODE_DIRECT_ALTER:
            case MODE_DIRECT_LONG:
                return M_SIZE_LONG;

            case MODE_ADDR_DISP:
            case MODE_DIRECT_SHORT:
            case MODE_PC_DISP:
            case MODE_MOVEP:
                return M_SIZE_SWORD;

            // case MODE_REG:
            case MODE_REGSET:
            case MODE_PAIR:
                return M_SIZE_AUTO;

            case MODE_IMMED_LONG:       // 0
                return M_SIZE_LONG;

            case MODE_IMMED_WORD:       // 4
            case MODE_IMMED_BYTE:       // 6
                return M_SIZE_SWORD;

            case MODE_IMMED_SINGLE:     // 1
            case MODE_IMMED_XTND:       // 2
            case MODE_IMMED_PACKED:     // 3
            case MODE_IMMED_DOUBLE:     // 5
               return M_SIZE_AUTO;

            case MODE_IMMED:
                throw std::logic_error("m68k_arg_data_size::mode");
        }
    }
}
#endif

template <typename Inserter>
bool m68k_arg_t::serialize (Inserter& inserter, bool& completely_saved)
{
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero
            auto p = expr.get_fixed_p();
            if (p && !*p)
            {
                completely_saved = true;    // validator saved all data.
                return false;               // and no expression.
            }
            completely_saved = false;
            return !inserter(std::move(expr), size);
        };
    
    //using expression::expr_fits;
    //using expression::fits_result;

#if 0

    p->arg_mode = arg.mode;
    auto ext_mode = detail::m68k_arg_data_size(arg.mode);
    switch (ext_mode)
    {
        default:
            throw std::logic_error("m68k_insert_one: ext_mode");

        case M_SIZE_NONE:
            // here test if data not properly stored...
            if (!p->has_data)
                break;

            // not properly stored -- add an "*_AUTO"
            ext_mode = M_SIZE_AUTO;

            // FALLSTHRU
        case M_SIZE_WORD:
        case M_SIZE_UWORD:
        case M_SIZE_LONG:
        case M_SIZE_SWORD:
        case M_SIZE_AUTO:
            p->has_data = true;
            p->has_expr = !inserter(std::move(arg.disp), ext_mode);
            break;

        case M_SIZE_INDEX:
            // if MODE_INDEX brief with fixed offset, just store extension word
            // PC_INDEX is a unicorn. Don't need to try as hard...
            if (arg.mode == MODE_INDEX) {
                if (auto ip = arg.disp.get_fixed_p()) {
                    if (expr_fits{}.fits<int8_t>(*ip) == fits_result::DOES_FIT) {
                        if (arg.ext.brief_ok()) {
                            inserter(arg.ext.brief_value(*ip), M_SIZE_UWORD);
                            // facilitate easy size calculation...
                            p->arg_mode = MODE_INDEX_BRIEF;
                            p->has_expr = false;
                            break;
                        }   // no suppressed index or outer mode
                    }   // disp is byte
                }   // is integer
            }   // is MODE_INDEX

            // brief can not be calculated -- save base, disp & outer.
            // NB: index always `has_data`: reuse bit as `has_outer_expr`
            inserter(arg.ext.value(), M_SIZE_UWORD);

            {
                // insert displacements as signed
                auto disp_ext = arg.ext.disp_size;
                if (disp_ext == M_SIZE_WORD)
                    disp_ext = M_SIZE_SWORD;

                p->has_expr = !inserter(std::move(arg.disp), disp_ext);
                if (arg.ext.outer()) {
                    disp_ext = arg.ext.outer_size();
                    if (disp_ext == M_SIZE_WORD)
                        disp_ext = M_SIZE_SWORD;
                    p->has_data = !inserter(std::move(arg.outer), disp_ext);
                }
            }
            break;
    }

    // std::cout << "write_one: mode = " << std::setw(2) << (int)p->arg_mode;
    // std::cout << " bits: " << (int)p->has_data << "/" << (int)p->has_expr;
    // std::cout << " -> ";
#endif
    completely_saved = true;
    return false;
}


// deserialize m68k_arguments: for format, see above
template <typename Reader>
void m68k_arg_t::extract(Reader& reader, bool has_data, bool has_expr)
{
#if 0
    // std::cout << "read_one:  mode = " << std::setw(2) << (int)p->arg_mode;
    // std::cout << " bits: " << (int)p->has_data << "/" << (int)p->has_expr;
    // std::cout << " -> ";

    // tell extract what to mode to extract
    arg_p->mode = p->arg_mode;
    fmt.extract(n, opcode_p, arg_p);
    arg_p->mode = p->arg_mode;      // prevent disassembler override...

    auto ext_mode = detail::m68k_arg_data_size(arg_p->mode);
    switch (ext_mode) {
        default:
            throw std::runtime_error{"m68k_extract_one"};

        case M_SIZE_NONE:
            if (!p->has_data)
                break;

            // not stored in opcode. Need extension
            ext_mode = M_SIZE_AUTO;

            // FALLSTHRU
        case M_SIZE_WORD:
        case M_SIZE_LONG:
        case M_SIZE_SWORD:
        case M_SIZE_AUTO:
            arg_p->disp = reader.get_expr(p->has_expr ? M_SIZE_AUTO : ext_mode);
            break;

        case M_SIZE_UWORD:
            {
#if 0
                if (p->has_expr)
                    arg_p->disp = reader.get_expr(M_SIZE_AUTO);
                else
                    p->has_expr = (unsigned)reader.get_fixed(M_SIZE_UWORD);
                    //arg_p->disp = (unsigned)reader.get_fixed(M_SIZE_UWORD);
#else
            arg_p->disp = reader.get_expr(p->has_expr ? M_SIZE_AUTO : M_SIZE_UWORD);
#endif
            }
            break;

        case M_SIZE_INDEX:
            arg_p->ext  = reader.get_fixed(M_SIZE_WORD);
            {
                auto disp_ext = arg_p->ext.disp_size;
                if (disp_ext == M_SIZE_WORD)
                    disp_ext = M_SIZE_SWORD;
                arg_p->disp = reader.get_expr(p->has_expr ? M_SIZE_AUTO : disp_ext);
                if (arg_p->ext.outer()) {
                    disp_ext = arg_p->ext.outer_size();
                    if (disp_ext == M_SIZE_WORD)
                        disp_ext = M_SIZE_SWORD;
                    arg_p->outer = reader.get_expr(p->has_data ? M_SIZE_AUTO : disp_ext);
                }
            }
            break;
    }
#endif
}

}


#endif
