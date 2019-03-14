#ifndef KAS_M68K_MOTO_ARG_OSTREAM_H
#define KAS_M68K_MOTO_ARG_OSTREAM_H

// instantiate m68k instruction parser.

#include "m68k.h"
#include "m68k_arg.h"

namespace kas::m68k
{
    // instantiate insn_m68k printer for test runners
    // template void insn_m68k::m68k_op_t::print<std::ostream>(std::ostream&, args_t&&);

    // print m68k_arg_t in Motorola format
    std::ostream& moto_arg_ostream(std::ostream& os, m68k_arg_t const& arg)
    {
        // declare a couple of utilities
        auto disp_str = [](expr_t e, int mode)
            {
                const char *mode_strs[] = {".a", "", ".w", ".l"};

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
                    name += ".l";
                } else {
                    name += ".w";
                }
                switch (ext.reg_scale) {
                    case 0: break;
                    case 1: name += "*2"; break;
                    case 2: name += "*4"; break;
                    case 3: name += "*8"; break;
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

        bool pc_reg = false;

        switch (arg.mode()) {
        case MODE_ERROR:
            return os << "Err: " << base;
        case MODE_DATA_REG:
            return os << reg_name(RC_DATA, reg);
        case MODE_ADDR_REG:
            return os << reg_name(RC_ADDR, reg);
        case MODE_ADDR_INDIR:
            return os << "(" << reg_name(RC_ADDR, reg) << ")";
        case MODE_POST_INCR:
            return os << "(" << reg_name(RC_ADDR, reg) << ")+";
        case MODE_PRE_DECR:
            return os << "-(" << reg_name(RC_ADDR, reg) << ")";
        case MODE_ADDR_DISP:
        case MODE_MOVEP:
            // return os << base << "(" << reg_name(RC_ADDR, reg) << ")";
            // display in decimal
            return os << disp_str(base, 1) << "(" << reg_name(RC_ADDR, reg) << ")";
        case MODE_INDEX:
            break;      // addr + index
        case MODE_DIRECT_SHORT:
            return os << "(" << base << ").w";
        case MODE_DIRECT_LONG:
            return os << "(" << base << ").l";
        case MODE_PC_DISP:
            return os << disp_str(base, 1) << "(" << reg_name(RC_PC, reg) << ")";
        case MODE_PC_INDEX:
            pc_reg = true;
            break;      // pc + index
        case MODE_IMMED:
        case MODE_IMMED_QUICK:
        case MODE_IMMED_LONG:       // 0
        case MODE_IMMED_SINGLE:     // 1
        case MODE_IMMED_XTND:       // 2
        case MODE_IMMED_PACKED:     // 3
        case MODE_IMMED_WORD:       // 4
        case MODE_IMMED_DOUBLE:     // 5
        case MODE_IMMED_BYTE:       // 6
            return os << "#" << base;
        case MODE_INDEX_BRIEF:
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
        // do some bit twiddling with inner/outer modes
        auto const& inner = base;
        auto outer_mode   = index.mem_mode &~ M_SIZE_POST_INDEX;
        auto inner_mode   = index.disp_size;
        bool inner_zero   = inner_mode == M_SIZE_ZERO;
        bool outer_zero   = outer_mode == M_SIZE_ZERO;

        bool index_second = index.mem_mode != outer_mode;
        bool index_first  = !index.index_suppr() && !index_second;

        // if not memory indirect
        if (!index_second && !outer_mode) {
            os <<  "(";
            if (!inner_zero) {
                // display brief offset (8-bit) w/o size indicator
                auto mode = index.is_brief() ? 1 : inner_mode;
                os << disp_str(inner, mode) << ",";
            }
            if (!pc_reg)
                os << reg_name(index.base_suppr ? RC_ZADDR : RC_ADDR, reg);
            else
                os << reg_name(index.base_suppr ? RC_ZPC : RC_PC, 0);
            if (index_first)
                os << "," << index_reg_name(index);
            os << ")";
            return os;
        }

        // here memory indirect
        os << "([";
        if (!inner_zero)
            os << disp_str(inner, inner_mode) << ",";
        if (!pc_reg)
            os << reg_name(index.base_suppr ? RC_ZADDR : RC_ADDR, reg);
        else
            os << reg_name(index.base_suppr ? RC_ZPC : RC_PC, 0);
        if (index_first)
            os << "," << index_reg_name(index);
        os << "],";
        if (index_second) {
            os << index_reg_name(index);
            if (!outer_zero)
                os << ",";
        }
        if (!index_second || !outer_zero)
            os << disp_str(outer, outer_mode);
            os << ")";
        return os;
    }
}

#endif

