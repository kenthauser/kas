#ifndef KAS_CORE_SYMBOL_H
#define KAS_CORE_SYMBOL_H


#include "expr/expr.h"
#include "core_addr.h"
#include "kas_object.h"

// Use ELF values for symbol type & symbol binding.
// BSD pseudo ops can specify type & binding by numeric value, so hard to fake.
#include "elf/elf_common.h"


// ELF requires first local symbol be the one-and-only `STT_FILE` symbol.
// Compilers generally just emit a `.file` symbol first to comply.
// Assembler can force compilance using a `static` pointer, but can be overkill
// Define `ENFORCE_FILE_FIRST_LOCAL` to use static pointer method
//#define ENFORCE_FILE_FIRST_LOCAL

namespace kas::core
{
// elf defines `bindings` greater than zero. declare internal values < 0
static constexpr int8_t STB_TOKEN    = -1;  // initial binding when parsed
static constexpr int8_t STB_UNKN     = -2;  // initial binding when value accessed
static constexpr int8_t STB_INTERNAL = -3;  // used internally: never exported

template <typename REF>
struct core_symbol : kas_object<core_symbol<REF>, REF>
{
    using base_t      = kas_object<core_symbol<REF>, REF>;
    using emits_value = std::true_type;
    using expr_t      = expression::ast::expr_t;

    // expose protected methods
    using base_t::for_each_if;
    using base_t::for_each;

#ifdef ENFORCE_FILE_FIRST_LOCAL
private:
    // only one symbol can be of type "FILE", per ELF.
    // declare static for that symbol.
    inline static core_symbol *file_sym_p;

    friend base_t;
    static void clear()
    {
        // clear statics
        file_sym_p       = {};
        base_t::clear();
    }
#endif

public:
    // default: unnamed internal symbols (used by dwarf, etc)
    core_symbol(std::string name = {}
              , parser::kas_loc loc  = {}
              , int8_t binding = STB_INTERNAL
              , int8_t type = STT_NOTYPE
              )
            : s_name(std::move(name))
            , s_binding(binding)
            , s_type(type)
            , base_t(loc)
            {}
    
    // setters to modify symbol
    const char *set_type(int8_t new_type);
    const char *set_value(expression::ast::expr_t& value, int8_t type = STT_NOTYPE);
    void set_binding   (int8_t value) { s_binding    = value; }
    void set_visibility(int8_t value) { s_visibility = value; }

    // methods to convert symbol type (returning error string)
    const char *make_label (uint32_t binding = STB_LOCAL);
    const char *make_common(uint32_t comm_size, int8_t binding, uint16_t align = 0);
    const char *make_error( const char *msg = "Invalid Symbol");

#ifdef ENFORCE_FILE_FIRST_LOCAL
    // ELF requires `file` symbol to be first. Methods to enforce this...
    char *set_file_symbol();
    static auto const  file() { return file_sym_p; }
#endif

    // getters for symbol inspection
    auto const  addr_p()     const { return s_addr_p;     }
    auto        kind()       const { return s_type;       }
    auto        binding()    const { return s_binding;    }
    auto const& name()       const { return s_name;       }
    auto        align()      const { return s_align;      }
    auto        visibility() const { return s_visibility; }

    // special getter for `value_p`. If value is undefined, 
    // change symbol binding from `TOKEN` so symbol is emitted.
    expr_t const* value_p() const;
    e_fixed_t *get_fixed_p() const;

    // elf getters/setters
    auto sym_num()    const { return s_symnum; }
    void sym_num(uint32_t num);

    unsigned const size() const;
    const char *size(expr_t& new_size);
    const char *size(uint32_t new_size);

    // specialize for allowed types (eg e_fixed_t, kas_loc)
    template <typename T>
    T const* get_p() const
    {
        return nullptr;
    }

    // possibly need a more general `for_each_if`
    template <typename FN, typename...Ts>
    static void for_each_emitted(FN fn, Ts&&...args);

    template <typename OS> static void dump(OS& os);
    template <typename OS> void print(OS&) const;

private:
#if 0
    template <typename OS>
    friend OS& operator<<(OS& os, core_symbol const& sym)
        { sym.print(os); return os; }
#endif       
    std::string  s_name;
    typename addr_ref::object_t   *s_addr_p     {};
    expr_t      *s_value_p    {};
    expr_t      *e_size_p     {};   // size as expr pointer
    uint32_t     f_size       {};   // size as fixed value
    uint8_t      s_align      {};   // alignment for `comm` symbols
    // binding is mutable to allow `value_p` getter to mark symbol as referenced
    mutable int8_t  s_binding;      // set by ctor
    int8_t       s_type;            // set by ctor:e 
    int8_t       s_visibility {};   // used by back end

    mutable uint32_t s_symnum {};   // assigned by "back end"
    static inline core::kas_clear _c{base_t::obj_clear};
};

// XXX get explicit instatantion error if moved to IMPL
template <typename REF>
template <typename OS>
void core_symbol<REF>::print(OS& os) const
{
    os << "[" << s_name << "(index: " << this->index() << ")]";
}

template <typename REF>
template <typename OS>
void core_symbol<REF>::dump(OS& os)
{
    auto print_sym = [&](auto const& s)
    {
        // print symbol # in decimal, everything else in hex.
        os << std::dec;
        os <<         std::right << std::setw(4)  << s.index();
        os << std::hex;
        os << ": " << std::left  << std::setw(20) << s.s_name;
        os << "IUXLGW"[s.s_binding+3]  << " OFSfCTRS"[s.s_type] << ": ";
        if (s.s_addr_p)
            os << "addr = "  << *s.s_addr_p;
        else if (s.s_type == STT_FILE)
            os << "file = " << s.name();
        else if (s.s_value_p)
            os << "value = " << *s.s_value_p;
        else
            os << "*undef*";

        // extra attributes
        if (s.f_size)
            os << " f_size = "  <<  s.f_size;
        if (s.e_size_p)
            os << " e_size = "  << *s.e_size_p;
        if (s.s_align)
            os << " align = "   <<  +s.s_align;
        if (s.s_symnum)
            os << " sym_num = " <<  s.s_symnum;
        os << std::endl;
    };

    os << "symbols:" << std::endl;
    for_each(print_sym);
    os << std::endl;
}

namespace detail 
{
    // really just a stub at present
    static bool is_emitted(core_symbol_t const& sym) 
    {
#if 0
        switch (s.binding()) {
        case STB_INTERNAL:
            return false;
        case STB_TOKEN:
            return false;
        case STB_LOCAL:
            return true;
        case STB_GLOBAL:
            return true;
        default:
            return false;
        }
#else
        // true iff ELF binding (eg, not KAS)
        return sym.binding() >= 0;
#endif
    }
}

// possibly need a more general `for_each_if`
template <typename REF>
template <typename FN, typename...Ts>
void core_symbol<REF>::for_each_emitted(FN fn, Ts&&...args)
{
    for_each([&fn](core_symbol<REF>& s, auto&&...fn_args)
        {
            if (detail::is_emitted(s))
                fn(s, std::forward<decltype(fn_args)>(fn_args)...);
        }, std::forward<Ts>(args)...);
}
}

#endif
