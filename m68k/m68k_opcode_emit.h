#ifndef KAS_M68K_M68K_OPCODE_EMIT_H
#define KAS_M68K_M68K_OPCODE_EMIT_H

#include "m68k_insn_types.h"
#include "m68k_size_defn.h"
#include "m68k_format_float.h"

#include "kas_core/core_emit.h"
#include "expr/expr_fits.h"

namespace kas::m68k::opc
{
/////////////////////////////////////////////////////////////////////////
//
//  Emit m68k instruction: opcode & arguments
//
/////////////////////////////////////////////////////////////////////////

using m68k_word_size_t = m68k_ext_size_t;
using expression::e_fixed_t;
using expression::e_float_t;


template <typename ARGS_T>
void m68k_opcode_t::emit(
                   core::emit_base& base
                 , m68k_ext_size_t *op_p
                 , ARGS_T&&    args
                 , core::core_expr_dot const& dot) const
{
    using expression::expr_fits;

    //
    // emit m68k "extension" word. aka index
    //
    auto emit_index = [&base](auto& ext, auto& inner, auto& outer)
        {
            // utility to emit index displacement
            auto emit_index_disp = [&base](auto sz, auto& disp)
                {
                    if (sz == M_SIZE_ZERO)
                        return;             // don't emit zero displacements
                    base << core::set_size(sz == M_SIZE_WORD ? 2 : 4) << disp;
                };

            // see if brief mode indicated
            if (ext.brief_ok()) {
                if (auto p = inner.get_fixed_p()) {
                    if (expr_fits{}.fits<int8_t>(*p) == expression::DOES_FIT) {
                        base << ext.brief_value(*p);
                        return;
                    }
                // else emit full format
                }
            }

            // emit full index mode
            base << ext.hw_value();
            emit_index_disp(ext.disp_size, inner);
            if (ext.outer())
                emit_index_disp(ext.outer_size(), outer);
        };

    //
    // emit immediate arg: fixed or floating point
    //
    auto emit_immed = [&base](auto sz, auto& disp)
        {
            // set output size
            base << core::set_size(m68k_size_immed[sz]);
            
            // test if fixed or float format
            bool is_fixed = false;
            switch (sz)
            {
                case OP_SIZE_LONG:
                case OP_SIZE_WORD:
                case OP_SIZE_BYTE:
                    is_fixed = true;
                    break;

                // not fixed format
                default:
                    break;
            }

            // normal case: fixed immed with fixed type. Short-circuit
            auto fixed_p = disp.get_fixed_p();

            if (fixed_p && is_fixed)
            {
                base << *fixed_p;
                return;
            }

            // "mixed" mode require floating point type
            // floating point arg (actually a `ref_loc` reference)
            expression::kas_float ref{};

            if (fixed_p)
                ref = decltype(ref)::add(*fixed_p);
            else if (auto p = disp.template get_p<expression::kas_float>())
                ref = *p;

            if (!ref)
            {
                base << 0;//"Invalid floating point object";
                return;
            }

            // get float from reference
            auto& flt = ref.get();

            // format floating point number according to SZ
            m68k_format_float fmt;
            decltype(fmt)::result_type result;
            
            switch (sz)
            {
                case OP_SIZE_XTND:
                    result = fmt.flt2<80>(flt);
                    break;
                case OP_SIZE_DOUBLE:
                    result = fmt.flt2<64>(flt);
                    break;
                case OP_SIZE_SINGLE:
                    result = fmt.flt2<32>(flt);
                    break;
                case OP_SIZE_PACKED:
                    result = fmt.m68k_packed(flt);
                    break;
                case OP_SIZE_LONG:
                    result = fmt.fixed<32>(flt);
                    is_fixed = true;
                    break;
                case OP_SIZE_WORD:
                    result = fmt.fixed<16>(flt);
                    is_fixed = true;
                    break;
                case OP_SIZE_BYTE:
                    result = fmt.fixed<8>(flt);
                    is_fixed = true;
                    break;

                default:
                case OP_SIZE_VOID:
                    throw std::runtime_error{"m68k_emit_immed: sz"};
            }

            // unpack result
            auto [ n, result_p ] = result;
            auto buf_p = static_cast<decltype(fmt)::IEEE_DATA_T const *>(result_p);

            // if fixed, emit as int
            if (is_fixed)
                base << *buf_p;
            else if (n > 0)
                base << core::emit_data(sizeof(*buf_p), n) << buf_p;
            else 
                base << 0;//"FMT not supported";

        };

    //
    // emit base opcode, then args
    //
    base << *op_p;
    if (opc_long)
        base << *++op_p;

    // emit additional arg data
    for (auto& arg : args)
    {
        switch (arg.mode) {
            default:
                 break;

            // M_SIZE_WORD & M_SIZE_SWORD emit identically
            case MODE_ADDR_DISP:
            case MODE_DIRECT_ALTER:     // XXX should not happen?
            case MODE_DIRECT_SHORT:
            case MODE_INDEX_BRIEF:
            case MODE_MOVEP:
            case MODE_PC_DISP:
                base << core::set_size(2) << arg.disp;
                break;
            
            case MODE_DIRECT_LONG:
                base << core::set_size(4) << arg.disp;
                break;
            
            case MODE_DIRECT:
                // always first extenion word.
                // PC is address of first extension word.
                // XXX may need to adjust for 32-bit opcode
                base << core::emit_disp(dot, 2, 2) << arg.disp;
                break;

            case MODE_IMMED:
            case MODE_IMMED_LONG:       // 0
            case MODE_IMMED_SINGLE:     // 1
            case MODE_IMMED_XTND:       // 2
            case MODE_IMMED_PACKED:     // 3
            case MODE_IMMED_WORD:       // 4
            case MODE_IMMED_DOUBLE:     // 5
            case MODE_IMMED_BYTE:       // 6
                // calculate `insn_sz` from mode
                emit_immed(sz(), arg.disp);
                break;

            case MODE_INDEX:
            case MODE_PC_INDEX:
                emit_index(arg.ext, arg.disp, arg.outer);
                break;
        } // switch
    } // for
}

}

#endif
