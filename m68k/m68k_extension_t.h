#ifndef KAS_M68K_M68K_EXTENSION_T_H
#define KAS_M68K_M68K_EXTENSION_T_H

#include "expr/expr.h"
#include "target/tgt_arg.h"         // get forward declarations
//#include "kas_core/opcode.h"        // get op_size_t
#include "utility/align_as_t.h"

namespace kas::m68k
{
// declare type to hold memory indirect address mode info
// use the format of the 16-bit extension word
// M_SIZE_* enums chosen to match extension word format
#if 0

Bits:
    reg num         : 4 -> 4
    is_long         : 1 -> 5
    shift           : 2 -> 7
    is_brief        : 1 -> 8

    has_index       : 1 -> 8
    base_suppress   : 1 -> 9
    outer postindex : 1 -> 10   : is_postindex

    inner size  (zero/byte/word/long/auto) : 2 -> 12
    outer width (zero/word/long/auto)      : 2 -> 14

NB: don't need `inner::none` as there is no `inner suppress`
NB: don't need `outer::byte` as only `word` is emitted
NB: don't need `auto` as there is `p->has_expr`

NB: `byte` could be refactored as `brief` leaving same inner & outer modes
#endif

enum {
      M_SIZE_NONE  = 0              // reused in `disp_size` as "is_brief"
    , M_SIZE_ZERO  = 1
    , M_SIZE_WORD  = 2
    , M_SIZE_LONG  = 3
    , M_SIZE_AUTO  = M_SIZE_NONE    // special for serializer
};


// save & restore index reg as this type
// extension word is 16-bits
using m68k_ext_size_t = std::uint16_t;
struct m68k_arg_t;      // forward declaration

struct m68k_extension_t : kas::detail::alignas_t<m68k_extension_t, m68k_ext_size_t>
{
    using base_t::base_t;
    
    // expose for `size`
    using op_size_t = core::opc::opcode::op_size_t;

    // 14-bits to implement `m68k_extension`
    value_t reg_num         : 4;        // general register number (addr/data reg)
    value_t reg_scale       : 2;        // index scale for post '000 processors
    value_t reg_is_word     : 1;        // true = word, false = long

    value_t base_suppr      : 1;        // suppress base register
    value_t has_index_reg   : 1;        // index register not suppressed
    value_t is_post_index   : 1;        // using post-index memory mode
    value_t disp_size       : 2;        // M_SIZE_* for inner displacement
    value_t mem_size        : 2;        // M_SIZE_* for outer displacment
#if 0
    // convert from "serialization" format (as well as default ctor)
    m68k_extension_t(value_t ext = 0) : base_t(ext)
    {
        // static_assert in alignas_t makes this safe
        void *v = this;
        *static_cast<value_t*>(v) = ext;

        if (!*this)
        {
            reg_long(M_SIZE_AUTO);
            disp_size = M_SIZE_ZERO;
        }
    }

    // ctor to convert from "hardware" format
    template <typename U>
    m68k_extension_t(U hw, U& inner, bool& is_brief);

    // convert to "hardware" format
    value_t hw_value() const;
    value_t brief_value(int) const;

    bool brief_ok() const
    {
        return !base_suppr && !mem_mode;
    }

    bool is_brief() const
    {
        return  brief_ok() &&
                (disp_size == M_SIZE_BYTE || disp_size == M_SIZE_ZERO);
    }

    auto outer_size() const
    {
        return mem_mode & 3;
    }
#endif
    uint16_t hw_value() const   { return 0; }
    uint16_t outer_size() const { return M_SIZE_WORD; }
    uint16_t brief_value() const { return 0; }

    op_size_t size(m68k_arg_t&, expression::expr_fits const *);

    // return if `brief` extension word indicated
    uint16_t is_brief() const
    {
        return has_index_reg && (disp_size == M_SIZE_NONE);
    }
    
    bool brief_ok() const
    {
        return has_index_reg && !base_suppr && !outer();
    }

    void set_brief()
    {
        if (!brief_ok())
            throw std::logic_error{"m68k_extension::set_brief()"};
        disp_size = M_SIZE_NONE;
    }
    
    // return if `extension` required (not just displacement)
    operator bool() const
    {
        return base_suppr || has_index_reg || outer();
    }
   
    // return iff `outer` dereference required
    bool outer() const
    {
        return mem_size != M_SIZE_NONE && !is_post_index;
    }

    void emit(core::emit_base&, m68k_arg_t const&, uint8_t sz) const;
};


#if 0
// "hardware format" ctor: initialize from binary hardware extension word
template <typename U>
m68k_extension_t::m68k_extension_t(U hw, U& inner, bool& is_brief) : m68k_extension_t(0)
{
    // init "reg byte" values
    reg_num(hw >> 12);
    _reg_long = !!(hw & (1<<11));
    reg_scale = (hw >> 9) & 3;

    // "displacement byte" varies based on "brief mode"
    is_brief = !(hw & 0x100);
    if (is_brief)
    {
        auto disp = static_cast<int8_t>(hw);
        disp_size = inner ? M_SIZE_BYTE: M_SIZE_ZERO;
        inner = disp;
    } 
    else
    {
        base_suppr  = !!(hw & (1 << 7));
        _reg_inited = !(hw & (1<< 6));
        disp_size   = (hw >> 4) & 3;
        mem_mode    = hw & 7;
    }
}

// "hardware format ready to be emitted"
inline m68k_extension_t::value_t m68k_extension_t::hw_value() const
{
    // calculate reg "byte";
    auto reg_byte = reg_num() << 4;
    reg_byte |= (reg_scale << 1) | 1;
    if (_reg_long)
        reg_byte |= 1 << 3;

    // calculate displacements
    auto disp_byte = (disp_size << 4) | (mem_mode & 7);
    if (index_suppr())
        disp_byte |= 1 << 6;
    if (base_suppr)
        disp_byte |= 1 << 7;

    return (reg_byte << 8) | disp_byte;
}

// "brief format ready to be emitted"
inline m68k_extension_t::value_t m68k_extension_t::brief_value(int disp) const
{
    auto base = hw_value() & ~0x1ff;
    return base | (disp & 0xff);
}
#endif
}

#endif
