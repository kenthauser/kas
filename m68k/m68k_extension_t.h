#ifndef KAS_M68K_M68K_INDEX_T_H
#define KAS_M68K_M68K_INDEX_T_H

#include "expr/expr.h"
#include "utility/align_as_t.h"

namespace kas::m68k
{
// declare type to hold memory indirect address mode info
// use the format of the 16-bit extension word
// M_SIZE_* enums chosen to match extension word format

enum {
      M_SIZE_NONE  = 0
    , M_SIZE_ZERO  = 1
    , M_SIZE_WORD  = 2
    , M_SIZE_LONG  = 3
    , M_SIZE_AUTO  = 8
    , M_SIZE_POST_INDEX = 4
    , M_SIZE_BYTE  = 9
    , M_SIZE_UWORD = 10     // special for serializer
    , M_SIZE_INDEX = 11     // special for serializer
    , M_SIZE_SWORD = 12     // special for serializer
};


// save & restore index reg as this type
// extension word is 16-bits
using m68k_ext_size_t = std::uint16_t;

struct m68k_extension_t : kas::detail::alignas_t<m68k_extension_t, m68k_ext_size_t>
{
    using base_t  = kas::detail::alignas_t<m68k_extension_t, m68k_ext_size_t>;
    using value_t = typename base_t::value_t;

    // illegal to suppress both index & base reg. Use as init'd flag
    // ie: if both suppressed, extension is not inited. XXX
    value_t mem_mode     : 4;
    value_t disp_size    : 2;
    value_t _index_suppr : 1;   // ->!_reg_inited
    value_t base_suppr   : 1;
    value_t _reg_inited  : 1;
    value_t reg_scale    : 2;
    value_t _reg_long    : 1;
    value_t _reg_num     : 4;

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

    // index_t utility functions
    operator bool() const
    {
        return _reg_inited || base_suppr
               || mem_mode || (disp_size == M_SIZE_LONG);
    }

    auto reg_num() const
    {
        return _reg_num;
    }

    void reg_num(int _num)
    {
        _reg_num    = _num;
        _reg_inited = true;
    }

    auto reg_long() const
    {
        return _reg_long;
    }

    void reg_long(int mode)
    {
        // compare against value which is not default...
        _reg_long = mode != M_SIZE_WORD;
    }

    bool index_suppr() const
    {
        return !_reg_inited;
    }

    bool brief_ok() const
    {
        return !base_suppr && !mem_mode;
    }

    bool is_brief() const
    {
        return  brief_ok() &&
                (disp_size == M_SIZE_BYTE || disp_size == M_SIZE_ZERO);
    }

    bool outer() const
    {
        return mem_mode &~ M_SIZE_POST_INDEX;
    }

    auto outer_size() const
    {
        return mem_mode & 3;
    }
};

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

}

#endif
