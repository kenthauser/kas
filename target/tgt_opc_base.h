#ifndef KAS_TARGET_TGT_OPC_BASE_H
#define KAS_TARGET_TGT_OPC_BASE_H


#include "tgt_insn_serialize.h"
#include "tgt_data_inserter.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_base : core::opc::opcode
{
    using base_t       = tgt_opc_base;
    using mcode_t      = MCODE_T;
    
    using insn_t       = typename mcode_t::insn_t;
    using bitset_t     = typename insn_t::bitset_t;
    using arg_t        = typename mcode_t::arg_t;
    using argv_t       = typename mcode_t::argv_t;
    using arg_mode_t   = typename mcode_t::arg_mode_t;
    using stmt_info_t  = typename mcode_t::stmt_info_t;
    using stmt_args_t  = typename mcode_t::stmt_args_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;
    using val_t        = typename mcode_t::val_t;

    using op_size_t    = typename core::opcode::op_size_t;
    using emit_value_t = typename core::core_emit::emit_value_t;
   
    using reader_t     = tgt_data_reader_t<mcode_size_t, emit_value_t>;
   
    // forward declaration (required for virtual function declarations)
protected:
    struct serial_args_t;

public:
    // gen_insn:
    //
    // A "kitchen sink" interface for `core_insn` which takes all args
    //
    // Generate header & save args as is appropriate for derived opcode.
    // Implemented for all `tgt_opc*` objects to support it's specific prefix.

    virtual core::opcode *gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const& mcode
                 , stmt_args_t&&  args
                 , stmt_info_t    stmt_info

                 // and kas_core boilerplate
                 , opcode::data_t& data
                 ) = 0;

    // default: use `mcode` method
    virtual fits_result do_size(mcode_t const& mcode
                       , argv_t& args
                       , decltype(data_t::size)& size
                       , expr_fits const& fits
                       , stmt_info_t info) const
    {
        // get opcode runtime value
        auto trace = this->trace;

        // hook into validators
        auto& val_c = mcode.vals();
        auto  val_p = val_c.begin();

        // size of base opcode
        size = mcode.base_size();

        // here know val cnt matches
        auto result = fits.yes;
        auto sz     = info.sz(mcode);
        for (auto& arg : args)
        {
            if (trace)
                *trace << " " << val_p.name() << " ";

            auto r = val_p->size(arg, sz, fits, size);
            
            if (trace)
                *trace << +r << " ";
                
            switch (r)
            {
                case expr_fits::maybe:
                    result = fits.maybe;
                    break;
                case expr_fits::yes:
                    break;
                case expr_fits::no:
                    size = -1;
                    result = fits.no;
                    break;
            }
            
            // exit loop if switch() result is NO_FIT
            if (result == expression::NO_FIT)
                break;
            
            ++val_p;
        }

        if (trace)
            *trace << " -> " << size << " result: " << result << std::endl;
        
        return result;
    }

    // default: use `mcode` method
    virtual void do_emit(core::core_emit& base
                       , mcode_t const& mcode
                       , argv_t& args
                       , stmt_info_t info) const
    {
        // 0. generate base machine code data
        auto machine_code = mcode.code(info);
        auto code_p       = machine_code.data();

        // 1. apply args & emit relocs as required
        // NB: matching mcodes have a validator for each arg
        
        // Insert args into machine code "base" value
        // if base code has "relocation", emit it
        auto& fmt = mcode.fmt();
        auto val_iter = mcode.vals().begin();
        unsigned n = 0;
        for (auto& arg : args)
        {
            auto val_p = &*val_iter++;
            if (!fmt.insert(n, code_p, arg, val_p))
                fmt.emit_reloc(n, base, code_p, arg, val_p);
            ++n;
        }

        // 2. emit base code
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        while (words--)
            base << *code_p++;

        // 3. emit arg information
        auto sz = info.sz(mcode);
        for (auto& arg : args)
            arg.emit(base, sz);
    }

protected:
    static auto tgt_data_inserter(data_t& data)
    {
        return tgt_data_inserter_t<mcode_size_t, emit_value_t>(data);
    }

    static auto tgt_data_reader(data_t const& data)
    {
        return tgt_data_reader_t<mcode_size_t, emit_value_t>(data);
    }

    static auto serial_args(reader_t& reader, MCODE_T const& mcode)
    {
        return serial_args_t(reader, mcode);
    }

    // create a "container" for deserialized args
    // in addition to `argv_t` it holds other serialized values:
    //   eg: info, `code_p`, array of write-back pointers
    // Also, hold `insn_p` & `fixed_p` for opc_list support

    struct serial_args_t : argv_t
    {
        // use base ctor for parsed args
        using argv_t::argv_t;

        // ctor for deserialized args
        serial_args_t(reader_t& reader, MCODE_T const& mcode)
        {
            // deserialize serialized data into components
            std::tie(code_p, serial_pp, info) 
                    = tgt::opc::tgt_read_args(reader, mcode, *this);
        }
       
        void update_modes(arg_mode_t *modes) const override
        {
            // if parsed args, use base class update
            if (!code_p)
                return argv_t::update_modes(modes);

            // drive `set` writeback method for each arg
            auto p = std::begin(serial_pp);
            for (auto& arg : *this)
                (*p++)->set(*modes++);
        }

        // deserialized values to supplement `argv_t`
        std::array<detail::arg_serial_t *, mcode_t::MAX_ARGS> serial_pp;
        mcode_size_t  *code_p {};       // nullptr means parsed args
        stmt_info_t    info;
        
        // instance data not generally required
        // stash pointers required for `opc_list`
        insn_t const                  *insn_p;
        decltype(data_t::fixed.fixed) *fixed_p;
    };
};
}
#endif
