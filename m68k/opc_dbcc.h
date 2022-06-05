#ifndef KAS_M68K_OPC_DBCC_H
#define KAS_M68K_OPC_DBCC_H

// Support loop instructions: allow out-of-range branches
//
// DBcc Dn, <label>
//
// <label> displacement limited to 16-bit displacemnt. If out of range,
// emit as if the following was coded:
//
//      DBcc dn, 1f     | size = 4 (or 6+ for cpDBcc)
//      bra  2f         | size = 2
// 1:   bra.l <label>   | size = 6
// 2:                   | total = 12 (or 14+ for cpDBcc)
//
// NB: if `bra.l` not supported, "jmp.l" is used

// get derived `tgt_opc*` definitions
#include "m68k_formats_opc.h"

namespace kas::m68k::opc
{

// define `opc_dbcc` "opcode" as variant of general (ie: multiple args)
struct m68k_opc_dbcc : m68k_opc_general
{
    using base_t = m68k_opc_general;

    OPC_INDEX();
    using NAME = KAS_STRING("M68K_DBCC");
    const char *name() const override { return NAME::value; }

    static constexpr auto DBCC_SHIM_LENGTH  = 8;    // `bra 3f!bra.l <xxx>`
    
    // default: use `mcode` method
    fits_result do_size(mcode_t const& mcode
               , argv_t& args
               , decltype(data_t::size)& size
               , expression::expr_fits const& fits
              , stmt_info_t info) const override
    {
        // start with DBcc instruction size
        size = mcode.base_size();

        // just consult "destination" validator -- always last
        auto& vals = mcode.vals();
        auto dest_val_p = vals.last();
        auto result = dest_val_p->size(args[vals.size() - 1]    // last arg 
                                     , 0                        // info.sz
                                     , fits
                                     , size);

        std::cout << "dbcc::do_size: size = " << size << ", result = " << +result << std::endl;

        switch (result)
        {
            case expression::DOES_FIT:
                // branch in range -- just return
                break;
            case expression::MIGHT_FIT:
                // branch might not be in range -- update `size.max` to
                // indicate SHIM could be required
                size.max += DBCC_SHIM_LENGTH;
                break;
            case expression::NO_FIT:
            default:
                // SHIM required. update `size` and result
                size = mcode.base_size() + DBCC_SHIM_LENGTH;
                result = expression::DOES_FIT;
                break;
        }
        return result;
    }
   #if 0 
    // default: use `mcode` method
    void do_emit(core::core_emit& base
               , mcode_t const& mcode
               , typename base_t::serial_args_t& args) const override
    {
        mcode.emit(base, args, args.info);
    }
#endif
#if 0
    void do_emit     (data_t const&          data
                    , core::core_emit&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , unsigned               arg_mode) const override
    {
        // if "short" version, use base_t emitter
        auto base_size = mcode.base_size() + 2; // base plus word offset
        
        if (data.size() == base_size)
            return base_t::do_emit(data, base, mcode, code_p, dest, arg_mode);

        if (data.size() != base_size + 8)
            throw std::runtime_error{std::string(NAME()) + "invalid size: "
                                     + std::to_string(data.size())};

        // 0. generate base machine code data
        auto machine_code = mcode.code(info);
        auto code_p       = machine_code.data();

        // 1. apply args & emit relocs as required
        // Insert args into machine code "base" value
        // NB: *DO NOT* emit relocations, or data associated with displacement
        // NB: matching mcodes have a validator for each arg
        auto val_iter = vals().begin();
        unsigned n = 0;
        for (auto& arg : args)
        {
            auto val_p = &*val_iter++;
            fmt().insert(n, code_p, arg, val_p))
            ++n;
        }

        // 2. emit cpDBcc opcode
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        while (words--)
            base << *code_p++;

        // 3. emit boilerplate to convert to long cpDBcc
        base << (uint16_t) 2;       // branch offset
        base << (uint16_t) 0x6006;  // bra 2f
        base << (uint16_t) 0x4ef9;  // jmp (xxx).L opcode
        base << core::set_size(4) << dest; // address
    }
#endif
};

// trampoline for `m68k_format` code
struct fmt_dbcc : virtual m68k_mcode_t::fmt_t
{
    opcode_t& get_opc() const override
    {
        static m68k_opc_dbcc opc;
        return opc;
    }
};

}

#endif

