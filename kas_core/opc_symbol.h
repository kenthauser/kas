#ifndef KAS_CORE_OPC_SYMBOL_H
#define KAS_CORE_OPC_SYMBOL_H

#include "opcode.h"
#include "core_symbol.h"
#include "core_section.h"

namespace kas::core::opc
{
#ifdef XXX
    using e_fixed_t  = typename expression::e_fixed_t;
    using e_string_t = typename expression::e_string_t;
#else
    using expression::e_fixed_t;
    using expression::e_string_t;
#endif

    // label & equ opcodes defined here to solve include problem...
    struct opc_label : opcode
    {
        // inherit default case
        using opcode::operator();
        
        OPC_INDEX();
        const char *name() const override { return "LABEL"; }

        opc_label() = default;

        // Constructor used to programatically create symbols
        // Examples of use:
        //  1. converting local common -> blocks starting with symbol (BSS)
        //  2. generating dwarf programs

        void operator()(data_t& data, core_symbol& sym, short binding = STB_INTERNAL) const
        {
            if (auto msg = sym.make_label(binding)) { 
                //throw std::logic_error(std::string(__FUNCTION__) + ": " + msg);
                std::cout << __FUNCTION__ << ": " << msg << std::endl;
            }
            
            // for case of block starting with symbol
            data.size = sym.size();
        }

        void operator()(data_t& data, symbol_ref ref, short binding = STB_INTERNAL) const
        {
            (*this)(data, ref.get(), binding);
        }

        // ctor used for `labels`
        void proc_args(data_t& data, symbol_ref&& ref, uint32_t size = 0)
        {
            if (auto msg = ref.get().make_label(STB_LOCAL))
               return make_error(data, msg, ref);

            if (auto msg = ref.get().size(size))
                return make_error(data, msg, ref);

            data.size = size;
        }

        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            base << emit_addr << 0;

            // XXX override filler pattern
            static constexpr uint16_t filler = 0;

            auto n = data.size();
            if (n)
                base << emit_filler(n, filler);
        }
    };

    struct opc_equ : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "EQU"; }

        void proc_args(data_t& data, symbol_ref&& ref, expr_t&& expr)
        {
            auto& di = data.di();

            data.fixed.sym = ref;
            *di++ = std::move(expr);
            auto& label = ref.get();
            if (auto msg = label.set_value(di.last()))
                make_error(data, "EQU previously defined as different type", ref);
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            auto iter = data.iter();
            
            auto sym = data.fixed.sym;
            sym.get().print(os);

            os << " = " << *iter;
        }

        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            // use `core_data_size_t` to size (listing) output
            auto iter = data.iter();
            base << core::set_size(sizeof_data_t) << emit_expr << *iter;
        }
    };

    struct opc_common : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "COMM"; }

        void proc_args(data_t& data, short binding, short comm_size, short align
                       , core_symbol& sym, kas_loc const& loc)
        {
            if (auto result = sym.make_common(comm_size, binding, align))
                return make_error(data, result, loc);

            data.fixed.sym = sym.ref();
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            data.fixed.sym.get().print(os);
        }
    };

    struct opc_sym_binding : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "SYM"; }
        
        auto gen_proc_one(data_t& data, short binding)
        {
            return [&,binding=binding](expr_t&& e, kas_loc const &loc) -> std::size_t
                {
                    auto sym_ref_p = e.template get_p<symbol_ref>();
                    
                    if (sym_ref_p) {
                        sym_ref_p->get().set_binding(binding);
                        std::cout << "opc_sym_binding: " << *sym_ref_p;
                        std::cout << ": " << sym_ref_p->get().name() << " -> " << binding << std::endl;
                    }
                    else
                        *data.di() = kas_diag::error("Symbol argument required", loc);

                    return 0;
                };
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            auto iter = data.iter();
            auto cnt  = data.cnt;
            while(cnt--)
                os << *iter++ << " ";
        }

        void emit(data_t const& data, emit_base& base, core_expr_dot const *) const override
        {
            // just error messages
            auto iter = data.iter();
            auto cnt  = data.cnt;
            while(cnt--)
                base << *iter++;
        }

    };

    struct opc_sym_file : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "FILE"; }

        // need std::string && loc as args
        // XXX why rvalue?
        void proc_args(data_t& data, expr_t&& file)
        {
            if (auto file_name_p = file.get_p<e_string_t>()) {
                // fixed_p->sym = core_symbol::add(*file_name_p);
                // if (auto err = core_symbol::set_file(fixed_p->sym))
                //     make_error(err);
            } else {
                make_error(data, "string value required", *file.get_loc_p());
            }
        }

        void fmt(data_t const& data, std::ostream& os) const override
        {
            os << data.fixed.sym.get().name();
        }
    };

    struct opc_sym_type : opcode
    {
        OPC_INDEX();

        const char *name() const override { return "TYPE"; }

        void proc_args(data_t& data, int8_t type, core_symbol& sym, kas_loc const& loc)
        {
            if (auto msg = sym.set_type(type))
                make_error(data, msg, loc);
        }
    };

    struct opc_sym_size : opcode
    {
        OPC_INDEX();

        const char *name() const override { return "SIZE"; }

        void proc_args(data_t& data, core_symbol& sym, expr_t&& value, kas_loc const& loc)
        {
            std::cout << "opc_sym_size: " << sym.name() << " -> " << value << std::endl;

            // copy expr to persistent memory
            auto& di = data.di();
            *di++ = std::move(value);
            if (auto msg = sym.size(di.last()))
                return make_error(data, msg, loc);
                
            data.fixed.fixed = sym.index();
        }
        
        void fmt(data_t const& data, std::ostream& os) const override
        {
            auto iter = data.iter();
            auto& sym = core_symbol::get(data.fixed.fixed); 
            os << sym.name();

            if (data.cnt)
                os << ": raw = " << *iter;
            else
                os << ": fixed = " << sym.size();
        }
        
        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            auto iter = data.iter();

            // use `core_data_size_t` to size (listing) output
            base << core::set_size(sizeof_data_t) << emit_expr;
            
            auto& sym = core_symbol::get(data.fixed.fixed); 
            if (data.cnt)
                base << *iter;
            else
                base << sym.size();
        }
    };

}

#endif
