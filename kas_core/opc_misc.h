#ifndef KAS_CORE_OPC_MISC_H
#define KAS_CORE_OPC_MISC_H

#include "opcode.h"
#include "core_symbol.h"
#include "core_section.h"

namespace kas::core::opc
{
    using expression::e_fixed_t;

    struct opc_align : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "ALIGN"; }

        opc_align() = default;

        void operator()(data_t& data, uint16_t align = 0) const
        {
            data.fixed.fixed = align;
        }

        void proc_args(data_t& data, parser::kas_position_tagged const& loc, uint16_t alignment)
        {
            // no idea what max_alignment should be...
            static constexpr auto max_alignment = 6;

            // XXX need kas_loc with alignment
            if (alignment > max_alignment)
                make_error(data, "alignment exceeds maximum", loc);
            else if (!alignment)
                make_error(data, "non-zero alignment required", loc);
            else
                data.fixed.fixed = alignment;
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            os << data.fixed.fixed;
        }
        
        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            // XXX need alignment support & addr width support
            // XXX ignore alignment & just do words
            constexpr auto bytes_per_word = 2;

            auto size = dot_p->frag_p->delta();
            auto fill = 0;//fixed_p->fixed;

            // XXX move algorithm to core_emit. Share with align/segment/skip
            for (; size >= bytes_per_word; size -= bytes_per_word)
                base << set_size(bytes_per_word) << fill;

            while (size--)
                base << static_cast<unsigned char>(fill);
        }
    };

    struct opc_org   : opcode
    {
        static constexpr auto max_skip = 1 << 12;
        OPC_INDEX();

        const char *name() const override { return "ORG"; }

        void proc_args(data_t& data, kas_loc loc, expr_t&& value
                        , uint8_t fill_size = {}, uint32_t fill_data = {})
        {
            // Be sure it's location tagged for deferred errors.
            if (!loc)
                throw std::runtime_error("opc_org: value not location tagged");
            
            // fixed: error location for deferred errors
            data.fixed.loc = loc;
            data.size = fill_size;
            auto& di  = data.di();
            *di++     = std::move(value);
            *di++     = fill_data;
        }

        op_size_t calc_size(data_t& data, core_fits const& fits) const override
        {
            auto iter = data.iter();
            auto& org = *iter;
            auto& dot = fits.get_dot();
            
            // XXX need to test "different segment"
            // XXX need to test "org backwards"
            
            std::cout << "opc_org::calc_size:";
            std::cout << " org = " << *iter;
            std::cout << " dot frag = " << *dot.frag_p;
            std::cout << " offset = " << dot.dot_offset;
            std::cout << " base = " << dot.base_delta;
            std::cout << " delta = " << dot.cur_delta;
            std::cout << std::endl;


            // if constant, create expression to beginning of current subsection
            if (auto p = org.get_fixed_p()) {
                // this is a lot of work...
                static constexpr addr_offset_t zero;

                auto& segment = dot.frag_p->segment();
                auto& base = core_addr::add().init_addr(segment.initial(), &zero);
                auto& sum = base + *p;
                std::cout << "opc_org: dest = " << expr_t(sum) << std::endl;
                *iter = sum.ref(data.fixed.loc);
                return { 0, static_cast<short>(*p) };
            }
            
#if 0
            //return dot.set_org(*org.get_fixed_p());
            if (auto p = org.get_fixed_p()) {
                auto delta = *p - dot;
                auto ok = fits.disp(delta, 0, 1 << 31);
                std::cout << "opc_org_calc_size: " << delta;
                std::cout << " ok = " << std::endl;
                auto delta_fixed = 
                return [ 
            } else
#endif
                return {}; //dot.set_org(*org.get_fixed_p());
        }
        

        void fmt(data_t const& data, std::ostream& os) const override
        {
            auto iter = data.iter();
            os << *iter;
        }
    };

    // XXX need to split BSD & generic
    struct opc_skip  : opcode
    {
        OPC_INDEX();
        using opcode::opcode;
        // using opcode<opc_skip>::opcode;
        const char *name() const override { return "SKIP"; }

        void proc_args(data_t& data, kas_loc const& loc, expr_t&& skip, expr_t&& fill = {})
        {
            static constexpr auto fill_default = 0;

            auto skip_p = skip.get_fixed_p();
            if (!skip_p)
                return make_error(data, "fixed skip value required", loc);

            auto fill_p = fill.get_fixed_p();
            
            if (!fill_p)
                return make_error(data, "fill value must be constant", loc);

            data.size = *skip_p;
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            os << data.fixed.fixed;
        }

        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            // XXX need alignment support & addr width support
            // XXX ignore alignment & just do words
            constexpr auto bytes_per_word = 2;

            auto size = data.size.min;
            auto fill = data.fixed.fixed;

            for (; size >= bytes_per_word; size -= bytes_per_word)
                base << set_size(bytes_per_word) << fill;

            while (size--)
                base << static_cast<unsigned char>(fill);
        }
    };
}

#endif
