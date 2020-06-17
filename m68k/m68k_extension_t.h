#ifndef KAS_M68K_M68K_EXTENSION_T_H
#define KAS_M68K_M68K_EXTENSION_T_H

// Type to support M68K `extension` word.
//
// The `extension` word is used to support the `index` addressing mode
// on all 68k processors. The basic (68000 & '010) extension format allows
// a base register (address only), an index register (data or address) and
// an 8-bit signed offset. This is the "brief" format and occupies 16-bits.
//
// On later processors the index register may be scaled (x1, x2, x4 and x8),
// the base & index register can be suppressed, and the offset can be up to 32-bits.
// In addition, a second memory indirect operation can use on the calculated 
// effective address to perform a second index operation with an up to 
// 32-bit offset and possibly the index register. This is "full" mode and it 
// can occupy up to 5 16-bit words.
//
// To fully support the m68k extension word, a bit-field type is defined
// to mimic the fields in the "full" mode. This is a 16-bit type and is
// stored in the `m68k_arg_t`.  In addition, the field `m68k_arg_t::expr` is
// used to hold the "inner" displacement and a second expression field "outer"
// is also added to `m68k_arg_t`. A special serializer in `m68k_arg_serialize.h`
// is required to properly store & restore the fields.
//
// Since C++ doesn't specify bitfield layout, special `emit` and `init`
// routines perform translations from interal to machine formats.
//
// The `m68k_extension_t` type is stored in the `kas::aliagnas_t` CRTP
// type to allow the full bitfield to be initialized, stored, and restored easily.
//
// Since the `emit` method doesn't have `size` nor `fits`, the size-defining
// bits in the extension word must be updated and written back to the serialized
// instance. Since the size fields will have be properly initialized for constant
// values, only the "expression" values will see the size modified. And while
// modifying a "size" field can interfer with de-serialization of the argument value,
// there is no problem in this case because the serializer methods hold special bits
// (inspected first) which show an expression was serialized. This the size-bits
// (and the brief bit) can be used to pass size information to the emit method.

#include "expr/expr.h"
#include "target/tgt_arg.h"         // get forward declarations
#include "utility/align_as_t.h"

namespace kas::m68k
{
// declare type to hold memory indirect address mode info
// use the format of the 16-bit extension word as a guide
//
// Bits:
//    reg num         : 4 bits
//    is_long         : 1 bit
//    shift           : 2 bits
//    is_brief        : 1 bit
//
//    index_suppress  : 1 bit
//    base_suppress   : 1 bit
//    outer postindex : 1 bit   (as is_postindex)
//    has_outer       : 1 bit
//
//    inner size  (zero/byte/word/long/auto) : 2 bits
//    outer width (zero/word/long/auto) : 2 bits
//
// Total bits: 16 
//
// NB: don't need `inner::none` as there is no `inner suppress`
// NB: don't need `outer::byte` as only `word` is emitted
// NB: don't need `auto` as there is `p->has_expr` in serializer
//
// NB: `byte` could be refactored as `brief` leaving same inner & outer modes

// M_SIZE_* enums chosen to match extension word format
enum {
      M_SIZE_AUTO  = 0          // size is unknown
    , M_SIZE_ZERO  = 1
    , M_SIZE_WORD  = 2
    , M_SIZE_LONG  = 3
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

    // 16-bits to implement `m68k_extension`
    value_t reg_num         : 4;        // general register number (addr/data reg)
    value_t reg_scale       : 2;        // index scale for post '000 processors
    value_t reg_is_word     : 1;        // true = word, false = long
    value_t is_brief        : 1;        // true if brief format used

    value_t base_suppress   : 1;        // suppress base register
    value_t has_index_reg   : 1;        // index register "set"
    value_t has_outer       : 1;        // outer displacement "set"
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
    op_size_t size(m68k_arg_t&, expression::expr_fits const&, value_t *wb_ptr);

    // return if `extension` required (effective address not only address + disp)
    operator bool() const
    {
        return base_suppress || has_index_reg || has_outer;
    }
    
    // brief format is OK (if displacement is in 8-bit range)
    // also allow `displacement` mode (ie no index reg)
    bool brief_ok() const
    {
        return !base_suppress && !has_outer && !is_post_index;
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
