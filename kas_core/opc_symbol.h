#ifndef KAS_CORE_OPC_SYMBOL_H
#define KAS_CORE_OPC_SYMBOL_H

#include "opcode.h"
#include "core_symbol.h"
#include "core_section.h"

namespace kas::core::opc
{
using expression::e_fixed_t;
using expression::e_string_t;

// define labels & equ's
struct opc_label : opcode
{
    // inherit default case
    using opcode::operator();
    
    OPC_INDEX();
    const char *name() const override { return "LABEL"; }

    opc_label() = default;

    // method used to programatically create symbols
    // Examples of use:
    //  1. converting local common -> blocks starting with symbol (BSS)
    //  2. generating dwarf programs

    const char *operator()(data_t& data, core_symbol_t& sym, short binding = STB_INTERNAL) const
    {
        // define symbol address as pointing to `core_addr_t::get_dot()`
        if (auto msg = sym.make_label(binding))
        { 
            //throw std::logic_error(std::string(__FUNCTION__) + ": " + msg);
            std::cout << __FUNCTION__ << ": " << msg << std::endl;
            return msg;
        }
        
        // for case of block starting with symbol
        data.size = sym.size();
        return {};
    }

    const char *operator()(data_t& data, symbol_ref ref, short binding = STB_INTERNAL) const
    {
        return (*this)(data, ref.get(), binding);
    }

    const char *operator()(data_t& data, core_addr_t& addr) const
    {
        data.fixed.fixed = addr.index();    // non-zero
        return {};                          // won't fail
    }

    const char *operator()(data_t& data, addr_ref ref) const
    {
        return (*this)(data, ref.get());
    }


    // ctor used for `labels`
    void proc_args(data_t& data, core_symbol_t& sym, kas_loc const& loc
                 , uint16_t size = 0)
    {
        std::cout << "opc_label::proc_args: sym = " << sym << ", loc = " << loc.get() << std::endl;
        if (auto msg = sym.make_label(STB_LOCAL))
           return make_error(data, msg, loc);
        if (size)
            if (auto msg = sym.size(size))
                return make_error(data, msg, loc);
        data.size = size;
    }

    void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
    {
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

    void proc_args(data_t& data, symbol_ref const& ref, kas_token const& tok)
    {
        auto& di = data.di();

        data.fixed.sym = ref;
        *di++ = tok.expr();
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

    void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
    {
        // use `sizeof addr` to size (listing) output
        // sizeof_addr always larger than sizeof_data
        auto iter = data.iter();
        base << emit_expr << *iter;
    }
};

struct opc_common : opcode
{
    OPC_INDEX();
    const char *name() const override { return "COMM"; }

    void proc_args(data_t& data, short binding, short comm_size, short align
                   , core_symbol_t& sym, kas_loc const& loc)
    {
        if (auto result = sym.make_common(&loc, comm_size, binding, align))
            return make_error(data, result, loc);

        data.fixed.sym = sym.ref();
    }

    void fmt(data_t const& data, std::ostream& os) const override
    {
        data.fixed.sym.get().print(os);
    }
    
    void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
    {
        // show symbol location in address field of listing
        auto& sym = data.fixed.sym.get();
        base << emit_addr << sym;
        //base << emit_expr << sym.size();
    }
};

struct opc_sym_binding : opcode
{
    OPC_INDEX();
    const char *name() const override { return "SYM"; }
    
    auto gen_proc_one(data_t& data, short binding)
    {
        return [&,binding=binding](auto& e, kas_loc const &loc) -> std::size_t
            {
                auto sym_ref_p = e.template get_p<symbol_ref>();
                
                if (sym_ref_p)
                {
                    sym_ref_p->get().set_binding(binding);
                   // std::cout << "opc_sym_binding: " << *sym_ref_p;
                   // std::cout << ": " << sym_ref_p->get().name() << " -> " << binding << std::endl;
                }
                else
                    *data.di() = kas_diag_t::error("Symbol argument required", loc);

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

    void emit(data_t const& data, core_emit& base, core_expr_dot const *) const override
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

    void proc_args(data_t& data, int8_t type, core_symbol_t& sym, kas_loc const& loc)
    {
        if (auto msg = sym.set_type(type))
            make_error(data, msg, loc);
    }
};

struct opc_sym_size : opcode
{
    OPC_INDEX();

    const char *name() const override { return "SIZE"; }

    void proc_args(data_t& data, core_symbol_t& sym, expr_t const& value, kas_loc const& loc)
    {
        //std::cout << "opc_sym_size: " << sym.name() << " -> " << value << std::endl;

        // copy expr to persistent memory
        auto& di = data.di();
        *di++ = value;
        if (auto msg = sym.size(di.last()))
            return make_error(data, msg, loc);
            
        data.fixed.fixed = sym.index();
    }
    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        auto iter = data.iter();
        auto& sym = core_symbol_t::get(data.fixed.fixed); 
        os << sym.name();

        if (data.cnt)
            os << ": raw = " << *iter;
        else
            os << ": fixed = " << sym.size();
    }
    
    void emit(data_t const& data, core_emit& base, core_expr_dot const *dot_p) const override
    {
        auto iter = data.iter();

        // use `core_addr_size_t` to size (listing) output
        base << core::set_size(sizeof(expression::e_addr_t)) << emit_expr;
        
        auto& sym = core_symbol_t::get(data.fixed.fixed); 
        if (data.cnt)
            base << *iter;
        else
            base << sym.size();
    }
};

}

#endif
