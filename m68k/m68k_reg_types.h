#ifndef KAS_M68K_M68K_REG_TYPES_H
#define KAS_M68K_M68K_REG_TYPES_H


//
// m68k register patterns:
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple < reg_class(ex RX_DATA), reg_num(eg 0), tst(eg: hw::index_full)
// 1) reg_class    ranges from 0..9. `reg_arg_validate` has a hardwired limit of 12.
// 2) The reg_num  ranges from 0..7 for eg data register; has 12-bit value for move.c
// 3) tst          16-bit hw_tst constexpr value
//
// Each register also has a name. Based on command line options, a leading `%` may
// be permitted or requied
//
// A few registers have aliases. Currently the only aliases are fp->a6 & sp->a7
//
// A few registers can have multiple definitions:
// Example: USP can be a `RC_CPU` register when determining if (eg.) move.l a0,usp is allowed
//          USP can also be used as a `RC_CTRL` register in (eg.)   move.c a0, usp
// The two USPs have different `tst` conditions.
//
// Register `PC` is also overloaded.

// Observations:
// - Only a single alias is permitted.
// - No more hand two register definitions can have the same name
// - The "second" register definition index cannot be zero (because defition
//   zero is processed first, it will always be a "first" definition). Thus
//   a index of zero can be used to tell second definition is not present.
// - KAS_STRING register name definitions should always prepend '%'. A simple +1
//   can be used to remove it.
// currently, for M68K there are ~100-120 register definitions

// RC_PC & RC_ZPC both have a single member. Can be merged into RC_CPU

// Also note: `m68k_reg`    is an expression type
//            `m68k_regset` is an expression type
//            `m68k_reg`    needs to export parser to `expr`

//#include "m68k_types.h"

#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"


namespace kas::m68k 
{

////////////////////////////////////////////////////////////////////////////
//
// Declare M68K register constants
//
////////////////////////////////////////////////////////////////////////////


// declare the classes of registers (data, address, fp, etc.)
// NB: RC_DATA & RC_ADDR must stay 0 & 1 (they nominate "general registers")
enum
{
      RC_DATA = 0
    , RC_ADDR = 1
    , RC_ZADDR
    , RC_PC         // register class has single member
    , RC_ZPC        // register class has single member
    , RC_CPU
    , RC_CTRL
    , RC_FLOAT
    , RC_FCTRL
    , NUM_REG_CLASS
};

// name special registers for easy access
// NB: REG_CPU_* values are completely arbitrary
enum
{
      REG_CPU_USP
    , REG_CPU_SR
    , REG_CPU_CCR
           
    // coldfire MAC /eMAC
    , REG_CPU_MACSR
    , REG_CPU_MASK
    , REG_CPU_SF_LEFT   // scale factor: "<<"
    , REG_CPU_SF_RIGHT  // scale factor: ">>"

    // coldfire MAC
    , REG_CPU_ACC
    
    // coldfire eMAC
    , REG_CPU_ACC0
    , REG_CPU_ACC1
    , REG_CPU_ACC2
    , REG_CPU_ACC3
    , REG_CPU_ACC_EXT01
    , REG_CPU_ACC_EXT23
};

// name fp control registers for easy access
// values match `fmove.l`
enum
{
      REG_FPCTRL_IAR = 1    // FP Instruction Address Register
    , REG_FPCTRL_SR  = 2    // FP Status Register
    , REG_FPCTRL_CR  = 4    // FP Control Register
};

////////////////////////////////////////////////////////////////////////////
//
// definition of m68k register
//
////////////////////////////////////////////////////////////////////////////
#if 0
// declare constexpr definition of register generated at compile-time
struct m68k_reg_defn
{
    // type definitions for `parser::sym_parser_t`
    // NB: ctor NAMES index list will include aliases 
    using NAME_LIST = meta::list<meta::int_<0>>;

    template <typename...NAMES, typename N, typename REG_C, typename REG_V, typename REG_TST>
    constexpr m68k_reg_defn(meta::list<meta::list<NAMES...>
                                     , meta::list<N, REG_C, REG_V, REG_TST>>)
        : names     { (NAMES::value + 1)... }
        , reg_class { REG_C::value          }
        , reg_num   { REG_V::value          }
        , reg_tst   { REG_TST()             }
    {}

    static constexpr auto MAX_REG_NAMES = 2;        // allow single alias
    static inline const char *const *names_base;

    // first arg is which alias (0 = canonical, non-zero = which alias)
    const char *name(unsigned n = 0, unsigned skip_initial = 0) const noexcept
    {
        if (n < MAX_REG_NAMES)
            if (auto idx = names[n])
                return names_base[idx-1] + skip_initial;
        return {};
    }

    // each defn is 4 16-bit words (ie 64-bits)
    uint8_t     names[MAX_REG_NAMES];
    uint8_t     reg_class;
    uint16_t    reg_num;
    hw::hw_tst  reg_tst;
  
    template <typename OS>
    friend OS& operator<<(OS& os, m68k_reg_defn const& d)
    {
        os << "reg_defn: " << std::hex;
        const char * prefix = "names= ";
        for (auto& name : d.names)
        {
            if (name)
                os << prefix << name;
            prefix = ",";
        }
        os << " class="  << +d.reg_class;
        os << " num="    << +d.reg_num;
        os << " tst="    <<  d.reg_tst;
        return os << std::endl;
    }
};
#endif

//
// Declare the run-time class stored in `m68k_arg_t`
// Store up to two "defn" indexes with "hw_tst" in 64-bit value
//


#if 1

enum m68k_reg_prefix { PFX_NONE, PFX_ALLOW, PFX_REQUIRE };
struct m68k_reg : tgt::tgt_reg<m68k_reg>
{
    using hw_tst         = hw::hw_tst;
    using reg_defn_idx_t = uint8_t;

    using base_t::base_t;
    
    // MIT syntax only for now
    static const char *format_name(const char *n, unsigned i = 0)
    {
        if (i == 0)
            return n + 1;
        return {};
    }

};

#else


struct m68k_reg
{
    using reg_data_t = uint8_t;
    
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
    uint16_t const  kind()     const;
    uint16_t const  value()    const;
    const char *name()     const;

    const char *validate() const
    {
        if (reg_0_ok || reg_1_ok)
            return {};
        return validate_msg();
    }

    template <typename OS>
    void print(OS& os) const
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
#endif

////////////////////////////////////////////////////////////////////////////
//
// definition of m68k register set
//
////////////////////////////////////////////////////////////////////////////

struct m68k_reg_set : tgt::tgt_reg_set<m68k_reg_set, m68k_reg, uint16_t>
{
    using base_t::base_t;

    uint16_t reg_kind(m68k_reg const& r) const
    {
        auto kind = r.kind();
        switch (kind)
        {
            case RC_ADDR:
                kind = RC_DATA;
                // FALLSTHRU
            case RC_DATA:
            case RC_FLOAT:
            case RC_FCTRL:
                return kind;
            default:
                return -1;
        }
    }
    
    // convert "register" to bit number in range [0-> (mask_bits - 1)]
    uint8_t reg_bitnum(m68k_reg const& r) const
    {
        switch (r.kind())
        {
            case RC_DATA:  return  0 + r.value();
            case RC_ADDR:  return  8 + r.value();
            case RC_FLOAT: return  0 + r.value();
            case RC_FCTRL:
                // floating point control registers are "special"
                switch (r.value())
                {
                    case REG_FPCTRL_CR:  return 12;
                    case REG_FPCTRL_SR:  return 11;
                    case REG_FPCTRL_IAR: return 10;
                }
                // FALLSTHRU
            default:       return {};
        }
    }

    
    std::pair<bool, uint8_t> rs_mask_bits(bool reverse) const
    {
        // For M68K CPU: sixteen bit mask with
        // Normal bit-order: D0 -> LSB, A7 -> MSB
        //
        // For FPU: 8 bit mask with
        // Normal bit-order: FP7 -> LSB, FP0 -> MSB
        //
        // Easiest solution: toggle reverse for FP

        const int mask_bits  = (kind() == RC_FLOAT) ? 8 : 16;
        const bool bit_order = (kind() == RC_FLOAT) ? RS_DIR_MSB0 : RS_DIR_LSB0;

        return { bit_order ^ reverse, mask_bits };
    }
};

#if 1
#if 1
inline auto&  operator- (m68k_reg const& l, m68k_reg_set const& r)
{
    //return l - m68k_reg_set(r);
    return m68k_reg_set::add(l) - r;
}
#else
inline auto&  operator- (m68k_reg const& l, m68k_reg const& r)
{
    return l - m68k_reg_set(r);
    //return m68k_reg_set::add(l) - r;
}
#endif

inline auto& operator/ (m68k_reg const& l, m68k_reg_set const& r)
{
    return m68k_reg_set::add(l) / r;
}
#endif

using m68k_rs_ref = typename m68k_reg_set::ref_loc_t;
}

namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    // declare parser for M68K token
    using m68k_reg_parser_p = x3::rule<struct X_reg, m68k_reg>;
    BOOST_SPIRIT_DECLARE(m68k_reg_parser_p)
}


#endif
