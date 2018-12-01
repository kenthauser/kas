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
template <typename HW_TST, typename NAME_IDX_T = uint8_t>
struct tft_reg_defn;

//
// Declare the run-time class stored in the variant.  
// Store up to two "defn" indexes with "hw_tst" in 64-bit value
//

template <typename T> 
struct tgt_reg<typename T>
{
    using base_t    = tgt_reg;
    using derived_t = T;

    // declare default types
    using reg_defn_t = uint16_t;
    
private:
    static inline m68k_reg_defn const *insns;
    static inline reg_data_t          insns_cnt;

    static m68k_reg_defn const& get_defn(reg_data_t n);
    m68k_reg_defn const& select_defn() const;

public:
    static void set_insns(decltype(insns) _insns, unsigned _cnt)
    {
        insns = _insns;
        insns_cnt = _cnt;
    }

    m68k_reg() = default;

    // create new register from class/data pair
    // NB: used primarily for disassembly
    m68k_reg(reg_data_t reg_class, uint16_t value);

    // used to initialize `m68k_reg` structures
    template <typename T> void add(T const& d, reg_data_t n);
    
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
    static reg_data_t find_data(reg_data_t rc, uint16_t rv);
    const char *validate_msg() const;

    template <typename OS>
    friend OS& operator<<(OS& os, m68k_reg const& d)
    {
        d.print(os); 
        return os << std::endl;
    }

    // reg_ok is really a bool. 
    reg_data_t  reg_0_index {};
    reg_data_t  reg_0_ok    {};
    reg_data_t  reg_1_index {};
    reg_data_t  reg_1_ok    {};
};

}


#endif
