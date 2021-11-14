#ifndef KAS_CORE_SYMBOL_IMPL_H
#define KAS_CORE_SYMBOL_IMPL_H

#include "expr/expr.h"
#include <boost/variant/get.hpp>

#include <iomanip>

namespace kas::core
{

template <>
template <>
e_fixed_t const *core_symbol<symbol_ref>::get_p<e_fixed_t>() const
{
    if (auto p = value_p())
        return p->get_fixed_p();
    return nullptr;
}

// setters to modify symbol
template <typename Ref>
const char *core_symbol<Ref>::set_type(int8_t new_type)
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

template <typename Ref>
const char *core_symbol<Ref>::set_value(expression::ast::expr_t& value, int8_t type)
{
    if (auto err = set_type(type))
        return err;

    if (binding() < 0)          // equ's are not just "Tokens"
        set_binding(STB_LOCAL);

    if (s_addr_p)
        return "Symbol previously defined";

    s_value_p = &value;
    return nullptr;
}

template <typename Ref>
const char *core_symbol<Ref>::make_label(uint32_t binding, parser::kas_loc const *loc_p)
{
    // if internal pseudo-binding, use new binding
    if (s_binding < 0)
        s_binding = binding;
    
    // test if converting common to Block-Starting-with-Symbol
    if (s_type == STT_COMMON)
    {
        s_type = STT_OBJECT;
        s_addr_p = {};
    }

    if (s_addr_p)
        return "Symbol previously defined";

    if (loc_p)
        defined_loc = *loc_p;

    s_addr_p = &core_addr_t::get_dot();
    return nullptr;
}


template <typename Ref>
const char *core_symbol<Ref>::make_common(parser::kas_loc const *loc_p
                                        , uint32_t comm_size
                                        , int8_t binding
                                        , uint16_t align)
{
    if (auto err = set_type(STT_COMMON))
        return err;

    if (auto err = size(comm_size))
        return "Common previously declared with different size";

    // allow previous ".local" or ".globl" to prevail
    if (s_binding < 0)
        s_binding = binding;

    if (loc_p)
        defined_loc = *loc_p;


    s_align = align;
    return {};
}

template <typename Ref>
const char *core_symbol<Ref>::make_error(const char *msg, parser::kas_loc const *loc_p)
{
    // make global so backend symbol created (gen object on error...)
#if 0
    s_binding = STB_GLOBAL;
    s_type    = STT_ERROR; 
#endif
    if (!loc_p)
        loc_p = &this->loc();
    s_value_p = new expr_t(parser::kas_diag_t::error(msg, *loc_p));
    return msg;
}


#ifdef ENFORCE_FILE_FIRST_LOCAL
// static
template <typename Ref>
const char *core_symbol<Ref>::set_file(symbol_ref ref)
{
    if (file_sym_p)
        return "File previously defined";

    file_sym_p = &ref.get();
    return {};
}
#endif

// kbfd getters/setters
template <typename Ref>
void core_symbol<Ref>::set_sym_num(uint32_t num)
{
    if (s_symnum)
        throw std::runtime_error{"core_symbol: " + s_name + ": sym_num already set"};
    s_symnum = num;
}

// special getter for `value_p`. If value is undefined, 
// change symbol binding from `TOKEN` so symbol is emitted.
template <typename Ref>
expr_t const* core_symbol<Ref>::value_p() const
{ 
    if (s_value_p)
        return s_value_p;
    if (s_binding == STB_TOKEN)
        s_binding = STB_UNKN;
    return {};
}


template <typename Ref>
unsigned const core_symbol<Ref>::size() const
{
    // test if fixed size or missing expr_p
    if (f_size)
        return f_size;
    if (!e_size_p)
        return 0;

    //std::cout << "core_symbol<Ref>::size(): " << name() << ": " << *e_size_p << std::endl;
    if (auto p = e_size_p->get_fixed_p()) {
        return *p;
    } else if (auto p = e_size_p->template get_p<core_expr_t>()) {
        // this isn't obvious. if no relocs, test offset
        // expressions are only "matched" when dot is present
        // the `opc_sym_size::calc_size` trick makes this work.
        if (p->num_relocs() == 0)
            return p->get_offset()();
    }
    return 0;
}

template <typename Ref>
const char *core_symbol<Ref>::size(expr_t& new_size)
{
    if (f_size || e_size_p)
        return "Symbol size can't be altered";
    if (auto p = new_size.get_fixed_p())
        f_size = *p;
    else
        e_size_p = &new_size;
    return {};
}

template <typename Ref>
const char *core_symbol<Ref>::size(uint32_t new_size)
{
    if (f_size || e_size_p)
        if (new_size != f_size)
            return "Symbol size can't be altered";
    f_size = new_size;
    return {};
}

template const char *core_symbol<symbol_ref>::set_type(int8_t new_type);
template const char *core_symbol<symbol_ref>::set_value(expression::ast::expr_t& value, int8_t type);
template const char *core_symbol<symbol_ref>::make_label(uint32_t binding, parser::kas_loc const *);
template const char *core_symbol<symbol_ref>::make_common(parser::kas_loc const *, uint32_t comm_size, int8_t binding, uint16_t align);
template const char *core_symbol<symbol_ref>::make_error(const char *msg, parser::kas_loc const *);
template void core_symbol<symbol_ref>::set_sym_num(uint32_t num);

template expr_t const*  core_symbol<symbol_ref>::value_p() const;
template unsigned const core_symbol<symbol_ref>::size()    const;
template const char *core_symbol<symbol_ref>::size(expr_t& new_size);
template const char *core_symbol<symbol_ref>::size(uint32_t new_size);

#ifdef ENFORCE_FILE_FIRST_LOCAL
template const char *core_symbol<symbol_ref>::set_file(symbol_ref ref);
#endif
}

#endif
