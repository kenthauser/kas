#ifndef KAS_TARGET_TGT_REG_TYPE_H
#define KAS_TARGET_TGT_REG_TYPE_H

//
// Target "register" definition
//
// All processors have several named registers. Since these registers
// are used in the `expression` subsystem, use the PIMPL method to prevent
// having to include complete definition everywhere.
//
// This implementation allows for mulitple unrelated registers to have the
// same name. This occurs frequently when, for instance the "PC" register
// has one meaning when used in effective address calculations, and another
// meaning when accessed as a "special" register.
// 
// `tgt_reg` utilizes the CRTP template pattern to simplify modifications.
// No code bloat as only a single register implementation per target


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

template <typename Derived> 
struct tgt_reg
{
    using base_t      = tgt_reg;
    using derived_t   = Derived;
    
    // Declare `constexpr defn` type
    using defn_t      = tgt_reg_defn<Derived>;

    // declare default types
    using reg_defn_idx_t   = uint8_t;
    using reg_name_idx_t   = uint8_t;

    using reg_defn_class_t = int8_t;
    using reg_defn_value_t = uint16_t;
    using hw_tst           = uint16_t;

    static constexpr auto MAX_REG_ALIASES = 1;

    // default: return name unaltered
    static const char *format_name(const char *n, unsigned i = 0)
    {
        if (i == 0)
            return n;
        return {};
    }


private:
    static inline defn_t const  *insns;
    static inline reg_defn_idx_t insns_cnt;

    static defn_t const& get_defn(reg_defn_idx_t n);
    defn_t const&        select_defn(int reg_class = -1) const;

public:
    static void set_insns(decltype(insns) _insns, unsigned _cnt)
    {
        insns = _insns;
        insns_cnt = _cnt;
    }

    // used in X3 expression
    tgt_reg() = default;

    // create new register from class/data pair
    // NB: used primarily for disassembly
    tgt_reg(reg_defn_idx_t reg_class, uint16_t value);

    // used to initialize `m68k_reg` structures
    template <typename T> void add(T const& d, reg_defn_idx_t n);
    
    // methods to examine register
    uint16_t const  kind(int reg_class = -1)  const;
    uint16_t const  value(int reg_class = -1) const;

    const char *validate(int reg_class = -1) const;
    const char     *name()                    const;

    template <typename OS> void print(OS& os) const
    {
        os << name();
    }

private:
    static reg_defn_idx_t find_data(reg_defn_idx_t rc, uint16_t rv);
    const char *validate_msg() const;

    template <typename OS>
    friend OS& operator<<(OS& os, tgt_reg const& d)
    {
        d.print(os); 
        return os;
    }

    // reg_ok is really a bool. 
    reg_defn_idx_t  reg_0_index {};
    reg_defn_idx_t  reg_0_ok    {};
    reg_defn_idx_t  reg_1_index {};
    reg_defn_idx_t  reg_1_ok    {};
};

}


#endif
