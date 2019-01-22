#ifndef Z80_Z80_FORMATS_IMPL_H
#define Z80_Z80_FORMATS_IMPL_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

#include "z80_formats_type.h"

namespace kas::z80::opc
{
// tinker-toy functions to put args into various places...
// always paired: insert & extract

// Insert/Extract N bits from machine code
template <unsigned SHIFT, unsigned BITS, unsigned WORD = 0>
struct fmt_generic
{
    using mcode_size_t = uint8_t;
    static constexpr auto MASK = (1 << BITS) - 1;
    static bool insert(mcode_size_t* op, z80_arg_t& arg, z80_validate const *val_p)
        {
            expression::expr_fits fits;
            auto result = val_p->ok(arg, fits);

            if (result != fits.yes)
                return false;

            auto value = val_p->get_value(arg);       // NB: logic error if val_p == nullptr
            
            auto old_word = op[WORD];
            op[WORD] &= ~(MASK << SHIFT);
            op[WORD] |= value << SHIFT;         // NB: logic error if (VALUE &~ MASK)
            // std::cout << std::hex;
            // std::cout << "fmt_gen: " << old_word << " -> " << op[WORD];
            // std::cout << " mask = " << MASK  << " shift = " << SHIFT << std::endl;
            return true;
        }

    static void extract(mcode_size_t const* op, z80_arg_t* arg, z80_validate const *val_p)
        {
            auto value = MASK & op[WORD] >> SHIFT;
            val_p->set_arg(*arg, value);
        }
};

}
#endif

:z
