#ifndef KAS_TARGET_TGT_REG_TYPE_H
#define KAS_TARGET_TGT_REG_TYPE_H

//
// Target "register" definition
//
// All processors have several named registers. Since these registers
// are used in the `expression` subsystem, use the PIMPL method to prevent
// having to include complete definition everywhere.
//
// This implementation allows for multiple unrelated registers to have the
// same name. This occurs frequently when, for instance the "PC" register
// has one meaning when used in effective address calculations, and another
// meaning when accessed as a "special" register.
// 
// `tgt_reg` utilizes the CRTP template pattern to simplify modifications.
// No code bloat as only a single register implementation per target

// registers also need to be expressed as tokens
#include "parser/token_defn.h"

namespace kas::tgt
{

////////////////////////////////////////////////////////////////////////////
//
// definition of target register
//
////////////////////////////////////////////////////////////////////////////

// forward declare "definition" structure
template <typename Reg_t>
struct tgt_reg_defn;

//
// Declare the run-time class stored in the variant.  
// Store up to two "defn" indexes with "hw_tst" in 64-bit value
//

template <typename Derived
        , typename REGSET = void
        , typename BASE_T = uint16_t
        , unsigned REG_C_BITS = 8
        , unsigned VALUE_BITS = std::numeric_limits<BASE_T>::digits - REG_C_BITS> 
struct tgt_reg
{
    using base_t      = tgt_reg;
    using derived_t   = Derived;
    using regset_t    = REGSET;

    // be sure bits fit into single `BASE_T` value
    static_assert(VALUE_BITS > 0,
                "target::tgt_reg: invalid definition for register value");
    static_assert(std::numeric_limits<BASE_T>::digits >= (REG_C_BITS + VALUE_BITS),
                "target::tgt_reg: invalid base type for register definition");
    
    // actual types are needed for template instantiation (typedefs not allowed)
    using mcode_sz_t  = BASE_T;
    using reg_c_bits  = std::integral_constant<unsigned, REG_C_BITS>;
    using value_bits  = std::integral_constant<unsigned, VALUE_BITS>;
    
    // Declare `constexpr defn` type
    using defn_t      = tgt_reg_defn<Derived>;

    // declare default types
    using reg_defn_idx_t   = uint8_t;
    using reg_name_idx_t   = uint8_t;

    // should calculate `T` from `BITS`
    using reg_class_t      = BASE_T;
    using reg_value_t      = BASE_T;
    using hw_tst           = uint16_t;

    // default: allow a single alias 
    // default: allow two different reg defns to have same name
    // example: SP can be an general register & also name a control register
    static constexpr auto MAX_REG_ALIASES    = 1;
    static constexpr auto MAX_REG_NAME_DEFNS = 2;

    using map_key = std::pair<reg_class_t, reg_value_t>;

    // default: return name unaltered
    static const char *format_name(const char *n, unsigned i = 0)
    {
        if (i == 0)
            return n;
        return {};
    }

    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }


protected:
    static inline defn_t const  *insns;
    static inline reg_defn_idx_t insns_cnt;

    static defn_t const& get_defn(reg_defn_idx_t n);
    defn_t const&        select_defn(int reg_class = -1) const;

    // access parser to convert name->reg_t (for `get` method)`
    static inline derived_t const *(*lookup)(const char *);
    
    static auto& obstack()
    {
        static auto deque_ = new std::deque<derived_t>;
        return *deque_;
    }

    static auto& defn_map()
    {
        static auto map_ = new std::map<map_key, derived_t const *>;
        return *map_;
    }

public:
    // unfortunate name; record base of definitions
    static void set_insns(decltype(insns) _insns, unsigned _cnt)
    {
        insns     = _insns;
        insns_cnt = _cnt;
    }

    // record x3 parser instance to convert name -> derived_t
    template <typename PARSER_P>
    static void set_lookup(PARSER_P p)
    {
        static auto parser_p = p;
        lookup = [](const char *name) -> derived_t const *
            { return *parser_p->find(name); };
    }

    // used in X3 expression
    tgt_reg() = default;

    // actual ctor for static instances
    tgt_reg(const char *name, reg_name_idx_t idx) : _idx(idx) {}

    // use to create `reg_t` instances
    static derived_t& create(const char *name)
    {
        auto& s = obstack();
        return s.emplace_back(name, s.size()+1);
    }

    static derived_t& get(reg_name_idx_t idx)
    {
        return obstack()[idx-1];
    }

    // create new register from class/data pair
    // NB: used primarily for disassembly
    //tgt_reg(reg_defn_idx_t reg_class, uint16_t value);
    static derived_t const& get(reg_defn_idx_t reg_class, uint16_t value);

    // used to initialize `tgt_reg` structures
    template <typename T> void add_defn(T const& d, reg_defn_idx_t n);

    // used to verify instance is a register
    //bool valid() const { return reg_0_index; }
    bool valid() const { return defns[0]; }
    
    // methods to examine register
    uint16_t const  kind(int reg_class = -1)  const;
    uint16_t const  value(int reg_class = -1) const;

    const char *validate(int reg_class = -1) const;
    const char     *name()                    const;

    template <typename OS> void print(OS& os) const
    {
        os << name();
    }

    auto index() const { return _idx; }

private:
    static reg_defn_idx_t find_data(reg_defn_idx_t rc, uint16_t rv);
    const char *validate_msg() const;

    template <typename OS>
    friend OS& operator<<(OS& os, tgt_reg const& d)
    {
        d.print(os); 
        return os;
    }
#if 0
    // reg_ok is really a bool. 
    reg_defn_idx_t  reg_0_index {};
    reg_defn_idx_t  reg_0_ok    {};
    reg_defn_idx_t  reg_1_index {};
    reg_defn_idx_t  reg_1_ok    {};
#else
    std::array<reg_defn_idx_t, MAX_REG_NAME_DEFNS> defns;

    // can't have more `registers` than names
    reg_name_idx_t  _idx;
#endif
};

}


#endif
