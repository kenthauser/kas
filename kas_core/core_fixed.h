#ifndef KAS_CORE_CORE_FIXED_H
#define KAS_CORE_CORE_FIXED_H

#include "opcode.h"
#include "core_print.h"     // `fmt`
#include "core_insn.h"      // back-inserter
#include "core_fixed_inserter.h"
#include <cstdint>

// Opcode support for fixed data types
//
// Assemblers must support a number of fixed data types. These include
// .byte, .double, and .string. These data types are similar in that they
// typically take an unlimited number of arguments (or none at all) and 
// generate a fixed object code block. With the addition of LEB128 variants,
// the size of the object code block can also vary.
//
// To support the various fixed data types, a CRTP base class is defined: `opc_data`
//
// The CRTP class back-end methods, in turn, are implemented by a non-templated class
// `core_fixed_impl`. This class uses various configuration points in the `opc_data` class
// to properly implement all the variants of fixed data types.

// The configuration points for the backends are gathered in `opc_fixed_config`.
// A static constexpr instance is part of the CRTP `opc_data` base class.

namespace kas::core::opc
{

namespace detail {
    using fixed_t   = typename opc::opcode::data_t::fixed_t;
    using op_size_t = typename opc::opcode::data_t::op_size_t;

    using SIZE_ONE = op_size_t(*)(expr_t const&, core_fits const&);
    using EMIT_ONE = void(*)(emit_base&, expr_t const&, core_expr_dot const *);

    struct opc_fixed_config
    {
        SIZE_ONE size_one;
        EMIT_ONE emit_one;
        uint16_t value_sz;
        uint16_t sizeof_diag;
    };

    // implmement `fixed` opcode backend methods
    struct opc_fixed_impl
    {
        constexpr opc_fixed_impl(opc_fixed_config const& config) : config(config) {}

        template <typename Iter>
        auto calc_size(fixed_reader_t<Iter>& reader, core_fits const& fits) const
        {
            op_size_t size {};

            // `calc_size` only called if `size` is not relaxed
            while(!reader.empty()) {
                if(reader.is_chunk()) {
                    // chunk data size is fixed size
                    auto tpl = reader.get_chunk();
                    auto& sz  = std::get<1>(tpl);
                    auto& cnt = std::get<2>(tpl);
                    size += sz * cnt;
                } else {
                    // evaluate expression 
                    auto& value = reader.get_expr();
                    size += config.size_one(value, fits);
                }
            }
            return size;
        }

        template <typename T>
        static void do_print_tpl(std::ostream&os, void const* v, uint16_t cnt)
        {
            T const *p = reinterpret_cast<T const *>(v);
                    
            // slightly complicated to avoid integer overflow on 64-bit case...
            // NB: calc is constexpr, so only happens at compile time
            uint64_t mask = ~0;
            static unsigned N = sizeof(T);
            if (sizeof (mask) > N)
                mask >>= (sizeof(mask) - N) * 8;

            while (cnt--)
                os << (*p++ & mask) << " ";
        }

        template <typename Iter>
        void fmt(fixed_reader_t<Iter>& reader, std::ostream& os) const
        {
            // reader returns `tuple` for array of chunks
            auto print_tpl = [&os](auto tpl)
                {
                    auto& p   = std::get<0>(tpl);
                    auto& sz  = std::get<1>(tpl);
                    auto& cnt = std::get<2>(tpl);

                    switch (sz) {
                    case 1:
                        do_print_tpl<uint8_t>(os, p, cnt);
                        break;
                    case 2:
                        do_print_tpl<uint16_t>(os, p, cnt);
                        break;
                    case 4:
                        do_print_tpl<uint32_t>(os, p, cnt);
                        break;
                    case 8:
                        do_print_tpl<uint64_t>(os, p, cnt);
                        break;
                    }
                };

            while(!reader.empty()) {
                if(reader.is_chunk()) {
                    // print chunk as tuple
                    print_tpl(reader.get_chunk());
                } else {
                    // print expession 
                    os << reader.get_expr() << " ";

                }
            }
        }

        template <typename Iter>
        void emit(fixed_reader_t<Iter>& reader, emit_base& base, core_expr_dot const *dot_p) const
        {
            using namespace expression;
            auto fits   = core_fits(dot_p);

            while(!reader.empty()) {
                if(reader.is_chunk()) {
                    // emit chunk as raw data (byte-swapped from host->target in backend)
                    auto tpl = reader.get_chunk();
                    auto& p   = std::get<0>(tpl);
                    auto& sz  = std::get<1>(tpl);
                    auto& cnt = std::get<2>(tpl);
                    base << emit_data(sz, cnt) << p; 
                } else {
                    // evaluate expression & emit
                    auto& value = reader.get_expr();        // what
                    auto loc_p = value.get_loc_p();         // where

                    // check for error conditions
                    if (value.template get_p<missing_ref>() && loc_p)
                        value = kas::parser::kas_diag::error("missing argument", *loc_p);
                    else if (!fits.ufits_sz(value, config.value_sz) && loc_p)
                        //value = kas_diag::error("value out of range");
                        ; /* XXX don't error */

                    config.emit_one(base, value, dot_p);
                }
            }
        }

    private:
        opc_fixed_config const& config;
    };
}

//
// `opc_data` is a CRTP trampoline class. 
// use trampoline to prevent code bloat
// all opcode "backend" operations (size/fmt/emit) performed by
// non-templated `core_fixed_impl`
//

template <typename INSN, typename VALUE_T>
struct opc_data : opcode
{
    OPC_INDEX();
    using base_t    = opc_data;
    using derived_t = INSN;
    using value_t   = VALUE_T;

#if 0
    // return "inserter" for instances of type `E`
    template <typename E, typename Arg_Inserter>
    auto inserter(Arg_Inserter& di)
    {
        // create fixed-back-inserter
        auto bi = detail::fixed_inserter_t<value_t>(di, *fixed_p);

        // move fixed-inserter into lambda context
        return [bi=std::move(bi)](expr_t&& e, kas_loc const& loc = {}) mutable
        {
            return derived_t::proc_one(bi, std::move(e), loc);
        };
    }
#else
#endif

    auto gen_proc_one(data_t& data)
    {
        // create chunk back-inserter
        auto bi = detail::fixed_inserter_t<value_t>(data.di(), data.fixed);

        // move fixed-inserter into lambda context
        // NB: if `loc` not provided, don't pass args that could generate errors...
        return [bi=std::move(bi)](expr_t&& e, kas_loc const &loc = {}) mutable -> op_size_t
            {
                return derived_t::proc_one(bi, std::move(e), loc);
            };
    }


    // each type represents a different inserter. Instantiated as `opcode::proc_args`
    template <typename C>
    void proc_args(data_t& data, C&& c)
    {
        // handle the solo "missing_arg" case
        opcode::validate_min_max(c);
        
        auto fn = gen_proc_one(data);
        op_size_t size = 0;

        // loop to move data from `parsed` expressions to `opc` data
        // NB: no loc for errors...so don't pass error values!
        for (auto&& e : c)
            size += fn(std::move(e), kas_loc());

        // accumulated size
        data.size = size;
    }

    // override `opcode` virtual methods
    const char *name() const override
    { 
        return INSN::NAME::value;
    }

    op_size_t calc_size(data_t& data, Iter it, core_fits const& fits) const override
    {
        auto reader = get_reader(data, it);
        return impl.calc_size(reader, fits); 
    }

    void fmt(data_t& data, Iter it, std::ostream& os) const override
    {
        auto reader = get_reader(data, it);
        return impl.fmt(reader, os);
    }

    void emit(data_t& data, Iter it, emit_base& base, core_expr_dot const *dot_p) const override
    {
        auto reader = get_reader(data, it);
        impl.emit(reader, base, dot_p);
    }

private:
    // support for backend methods
    // define configuration points
    static constexpr detail::opc_fixed_config config = 
        {
            derived_t::size_one,
            derived_t::emit_one,
            sizeof(VALUE_T),
            derived_t::sizeof_diag
        };
    
    static constexpr detail::opc_fixed_impl impl{config};

    //template <typename Iter>
    auto get_reader(data_t& data, Iter& it) const
    {
        return detail::fixed_reader_t<Iter>(data.fixed, it, data.cnt, sizeof(value_t));
    }

    // provide defaults for `opc_fixed_config`
    static op_size_t size_one(expr_t const&, core_fits const&)
    {
        throw std::logic_error("core_fixed::size_one undefined");
    }

    static void emit_one(emit_base&, expr_t const&, core_expr_dot const *)
    {
        throw std::logic_error("core_fixed::emit_one undefined");
    }

    // declare size to use for diagnostics 
    static constexpr unsigned sizeof_diag = sizeof(VALUE_T);
};




}
#endif
