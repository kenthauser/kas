#ifndef KAS_CORE_SYMBOL_IMPL_H
#define KAS_CORE_SYMBOL_IMPL_H

#include "expr/expr.h"
#include <boost/variant/get.hpp>

#include <iomanip>

namespace kas::core
{
    
template <>
expression::e_fixed_t const *core_symbol::get_p<expression::e_fixed_t>() const
{
    if (auto p = value_p())
        return p->get_fixed_p();
    return nullptr;
}


// setters to modify symbol
const char *core_symbol::set_type(int8_t new_type)
{
    // NB: STT_NOTYPE is synonym for LABEL
    // allow LABEL -> FUNC, and absorb FUNC -> LABEL
    if (new_type != s_type && new_type != STT_NOTYPE) {
        if (s_type == STT_NOTYPE)
            s_type = new_type;
        else
            return "Symbol type can't be altered";
    }
    
    return nullptr;
}

const char *core_symbol::set_value(expression::ast::expr_t& value, int8_t type)
{
    if (auto err = set_type(type))
        return err;

    if (s_addr_p)
        return "Symbol previously defined";

    s_value_p = &value;
    return nullptr;
}

const char *core_symbol::make_label(uint32_t binding)
{
    // if TOKEN or UNDEF, apply binding
    if (s_binding == STB_TOKEN || s_binding == STB_UNKN)
        s_binding = binding;
    
    // test if converting common to Block-Starting-with-Symbol
    if (s_type == STT_COMMON)
    {
        s_type = STT_NOTYPE;
        s_addr_p = {};
    }

    if (s_addr_p)
        return "Symbol previously defined";

    s_addr_p = &core_addr::get_dot();
    return nullptr;
}

const char *core_symbol::make_common(uint32_t comm_size, int8_t binding, uint16_t align)
{
    if (auto err = set_type(STT_COMMON))
        return err;

    if (auto err = size(comm_size))
        return "Common previously declared with different size";

    // allow previous ".local" or ".globl" to prevail
    if (s_binding < 0)
        s_binding = binding;

    s_align = align;
    return {};
}


const char *core_symbol::make_error(const char *msg)
{
    // make global so backend symbol created (gen object on error...)
#if 0
    s_binding = STB_GLOBAL;
    s_type    = STT_ERROR; 
#endif
    s_value_p = new expr_t(parser::kas_diag::error(msg, s_loc));
    return msg;
}

#ifdef ENFORCE_FILE_FIRST_LOCAL
// static
const char *core_symbol::set_file(symbol_ref ref)
{
    if (file_sym_p)
        return "File previously defined";

    file_sym_p = &ref.get();
    return {};
}
#endif

// elf getters/setters
void core_symbol::sym_num(uint32_t num)
{
    if (s_symnum)
        throw std::runtime_error{"core_symbol: " + s_name + ": sym_num already set"};
    s_symnum = num;
}

// special getter for `value_p`. If value is undefined, 
// change symbol binding from `TOKEN` so symbol is emitted.
expr_t const* core_symbol::value_p() const
{ 
    if (s_value_p)
        return s_value_p;
    if (s_binding == STB_TOKEN)
        s_binding = STB_UNKN;
    return {};
}


unsigned const core_symbol::size() const
{
    // test if fixed size or missing expr_p
    if (f_size)
        return f_size;
    if (!e_size_p)
        return 0;

    //std::cout << "core_symbol::size(): " << name() << ": " << *e_size_p << std::endl;
    if (auto p = e_size_p->get_fixed_p()) {
        return *p;
    } else if (auto p = e_size_p->template get_p<core_expr>()) {
        // this isn't obvious. if no relocs, test offset
        // expressions are only "matched" when dot is present
        // the `opc_sym_size::calc_size` trick makes this work.
        if (p->num_relocs() == 0)
            return p->get_offset()();
    }
    return 0;
}

const char *core_symbol::size(expr_t& new_size)
{
    if (f_size || e_size_p)
        return "Symbol size can't be altered";
    if (auto p = new_size.get_fixed_p())
        f_size = *p;
    else
        e_size_p = &new_size;
    return {};
}

const char *core_symbol::size(uint32_t new_size)
{
    if (f_size || e_size_p)
        if (new_size != f_size)
            return "Symbol size can't be altered";
    f_size = new_size;
    return {};
}

}

#endif
