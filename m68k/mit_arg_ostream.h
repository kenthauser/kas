#ifndef KAS_M68K_MIT_ARG_OSTREAM_H
#define KAS_M68K_MIT_ARG_OSTREAM_H

// instantiate m68k instruction parser.

#include "m68k.h"
#include "m68k_arg.h"

namespace kas::m68k
{
    // instantiate insn_m68k printer for test runners
    // template void insn_m68k::m68k_op_t::print<std::ostream>(std::ostream&, args_t&&);

    // print m68k_arg_t in MIT format
    std::ostream& mit_arg_ostream(std::ostream& os, m68k_arg_t const& arg)
    {
        // declare a couple of utilities
        auto disp_str = [](expr_t e, int mode)
            {
                const char *mode_strs[] = {":a", ":z", ":w", ":l"};

                std::stringstream str;
                str << e << mode_strs[mode & 3];
                return str.str();
            };

        auto reg_name = [](int reg_class, int reg_num)
            {
                switch (reg_class)
                {
                    case RC_PC:
                    case RC_ZPC:
                    case RC_ZADDR:
                        reg_num = 0;
                        break;
                    default:
                        break;
                }
                return m68k::m68k_reg_t(reg_class, reg_num).name();
            };

        auto index_reg_name = [&reg_name](m68k_extension_t const& ext) -> std::string
            {
                if (ext.index_suppr())
                    return reg_name(RC_ZADDR, 0);

                auto reg = ext.reg_num();
                std::string name = reg_name(reg & 8 ? RC_ADDR : RC_DATA, reg & 7);

                if (ext.reg_long()) {
                    name += ":l";
                } else {
                    name += ":w";
                }
                switch (ext.reg_scale) {
                    case 0: break;
                    case 1: name += ":2"; break;
                    case 2: name += ":4"; break;
                    case 3: name += ":8"; break;
                }
                return name;
            };

        // create aliases
        // using parser::disp_str;

        // NB: MODE_INDEX_BRIEF requires twiddling index & base -- copy these.
        auto const& reg    = arg.reg_num;
        auto index         = arg.ext;
        auto base          = arg.expr;
        auto const& outer  = arg.outer;
        
        // coldfire MAC subregisters. And MASK. sigh...
        auto suffix = "";
        switch (arg.reg_subword) 
        {
            case REG_SUBWORD_LOWER:
                suffix = ".l";
                break;
            case REG_SUBWORD_UPPER:
                suffix = ".u";
                break;
            case REG_SUBWORD_MASK:
                suffix = "&";
                break;
            default:
                break;
        }

        bool pc_reg = false;

        switch (arg.mode()) {
        case MODE_ERROR:
            return os << "Err: " << base;
        case MODE_NONE:
            return os << "*NONE*";
        case MODE_DATA_REG:
            return os << reg_name(RC_DATA, reg) << suffix;
        case MODE_ADDR_REG:
            return os << reg_name(RC_ADDR, reg) << suffix;
        case MODE_ADDR_INDIR:
            return os << reg_name(RC_ADDR, reg) << "@" << suffix;
        case MODE_POST_INCR:
            return os << reg_name(RC_ADDR, reg) << "@+" << suffix;
        case MODE_PRE_DECR:
            return os << reg_name(RC_ADDR, reg) << "@-" << suffix;
        case MODE_ADDR_DISP:
        case MODE_MOVEP:
            os << reg_name(RC_ADDR, reg);
            return os << "@(" << base << ")" << suffix;
        case MODE_INDEX:
            break;      // addr + index
        case MODE_DIRECT_SHORT:
            return os << base << ":w";
        case MODE_DIRECT_LONG:
            return os << base << ":l";
        case MODE_PC_DISP:
            return os << "pc@(" << base << ")";
        case MODE_PC_INDEX:
            pc_reg = true;
            break;      // pc + index
        case MODE_IMMED:
        case MODE_IMMED_QUICK:
            return os << "#" << base;
        case MODE_INDEX_BRIEF:
        case MODE_PC_INDEX_BRIEF:
            {
                // expand brief word to full index
                auto brief_word = *base.get_fixed_p();
                bool is_brief;
                index = { brief_word, brief_word, is_brief };
                base = brief_word;
                break;
            }
        case MODE_DIRECT:
        case MODE_DIRECT_ALTER:
        case MODE_REG:
        case MODE_REG_QUICK:
        case MODE_REGSET:
            return os << base;
        case MODE_PAIR:
            return os << base << ":" << outer;
        case MODE_BITFIELD:
            return os << "{" << base << "," << outer << "}";
        default:
            return os << "XXX MODE: " + std::to_string(arg.mode());
            throw std::runtime_error("print_arg: unknown mode: " + std::to_string(arg.mode()));
        }

        // here for index modes...
        if (!pc_reg)
            os << reg_name(index.base_suppr ? RC_ZADDR : RC_ADDR, reg);
        else
            os << reg_name(index.base_suppr ? RC_ZPC : RC_PC, 0);

        // do some bit twiddling with inner/outer modes
        auto const& inner = base;
        auto outer_mode   = index.mem_mode &~ M_SIZE_POST_INDEX;
        auto inner_mode   = index.disp_size;
        bool inner_zero   = inner_mode == M_SIZE_ZERO;
        bool outer_zero   = outer_mode == M_SIZE_ZERO;

        bool index_second = index.mem_mode != outer_mode;
        bool index_first  = !index.index_suppr() && !index_second;

        // print index (2x)
        if (index_first) {
            os << "@(" << index_reg_name(index);
            if (!inner_zero)
                os << "," << disp_str(inner, inner_mode);
            os << ")";
        } else if (!inner_zero) {
            os << "@(" << disp_str(inner, inner_mode) << ")";
        } else {
            os << "@(0)";
        }

        if (index_second) {
            os << "@(" << index_reg_name(index);
            if (!outer_zero)
                os << "," << disp_str(outer, outer_mode);
            os << ")";
        } else {
            if (outer_mode) {
                if (!outer_zero)
                    os << "@(" << disp_str(outer, outer_mode) << ")";
                else
                    os << "@(0)";
            }
        }

        return os;
    }
}

#endif

