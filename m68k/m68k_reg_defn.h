#ifndef KAS_M68K_M68K_REG_DEFN_H
#define KAS_M68K_M68K_REG_DEFN_H

#include "m68k_reg_types.h"
#include "target/tgt_reg_trait.h"

// XXX reg_impl?
// Define `m68k_reg_t::format_name` to handler register prefix
namespace kas::m68k
{
    // method to generate canonical & alternate names
    // (ie with & without '%' prefix)
    const char *m68k_reg_t::format_name(const char *orig, unsigned i)
    {
#if 0
        // make "with-%" canonical with `PFX_ALLOW`
        auto offset = (reg_pfx == PFX_NONE);
        if (i == 0)
            return orig + offset;
        else if (i == 1 && (reg_pfx == PFX_ALLOW))
            return orig + !offset;
        return {};
#else
        // make "without-%" canonical with `PFX_ALLOW`
        auto offset = (reg_pfx == PFX_NONE);
        if (i == 0)
            return orig + !offset;
        else if (i == 1 && (reg_pfx == PFX_ALLOW))
            return orig + offset;
        return {};
#endif
    }

    // method to calculate general register value
    uint16_t m68k_reg_t::gen_reg_value() const
    {
        auto& defn = select_defn();
        if (defn.reg_class == RC_DATA)
            return defn.reg_num;
        if (defn.reg_class == RC_ADDR)
            return defn.reg_num + 8;
        return 0;
    }
}
 
// Define each m68k register as a "NAME/CLASS/VALUE/TST" type list
namespace kas::m68k::reg_defn
{
using namespace tgt::reg_defn;

// prepend "%" to all register names
#define REG_STR(x) KAS_STRING("%" x)

// declare general and floating point registers
using data_reg_l  = make_reg_seq<reg_seq<RC_DATA>, REG_STR("d"), 8>;
using addr_reg_l  = make_reg_seq<reg_seq<RC_ADDR>, REG_STR("a"), 8>;
using fp_reg_l    = make_reg_seq<reg_seq<RC_FLOAT, hw::fpu>,        REG_STR("fp"), 8>;
using zaddr_reg_l = make_reg_seq<reg_seq<RC_ZADDR, hw::index_full>, REG_STR("za"), 8>;
 
// floating point user registers (values match register-list constants)
using fctrl_reg_l = list<
      reg<REG_STR("fpcr"),  RC_FCTRL, REG_FPCTRL_CR,  hw::fpu>
    , reg<REG_STR("fpsr"),  RC_FCTRL, REG_FPCTRL_SR,  hw::fpu>
    , reg<REG_STR("fpiar"), RC_FCTRL, REG_FPCTRL_IAR, hw::fpu>
     >;
 
// cpu registers
// NB: CPU register "VALUES" are kas internal only, and are arbitrary
using supv_reg_l = list<
      reg<REG_STR("pc"),  RC_PC>
    , reg<REG_STR("zpc"), RC_ZPC, 0, hw::index_full>
    , reg<REG_STR("sr"),  RC_CPU, REG_CPU_SR>
    , reg<REG_STR("ccr"), RC_CPU, REG_CPU_CCR>
    , reg<REG_STR("usp"), RC_CPU, REG_CPU_USP>
>;

using coldfire_mac_l = list<
    // coldfire MAC
      reg<REG_STR("acc"),  RC_MAC, REG_MAC_ACC,  hw::mac>
    
    // coldfire eMAC
    , reg<REG_STR("acc0"), RC_MAC, REG_MAC_ACC0, hw::emac> 
    , reg<REG_STR("acc1"), RC_MAC, REG_MAC_ACC1, hw::emac> 
    , reg<REG_STR("acc2"), RC_MAC, REG_MAC_ACC2, hw::emac> 
    , reg<REG_STR("acc3"), RC_MAC, REG_MAC_ACC3, hw::emac> 
    , reg<REG_STR("acc_ext01"), RC_MAC, REG_MAC_ACC_EXT01, hw::emac> 
    , reg<REG_STR("acc_ext23"), RC_MAC, REG_MAC_ACC_EXT23, hw::emac> 
    
    // coldfire MAC/eMAC
    , reg<REG_STR("macsr"), RC_MAC, REG_MAC_MACSR    , hw::coldfire>
    , reg<REG_STR("mask"),  RC_MAC, REG_MAC_MASK     , hw::coldfire>

    , reg<REG_STR("<<"),    RC_SHIFT, REG_SHIFT_LEFT , hw::coldfire>  // NB: comma-separated
    , reg<REG_STR(">>"),    RC_SHIFT, REG_SHIFT_RIGHT, hw::coldfire>
    , reg<REG_STR("*SF_N"), RC_SHIFT, REG_SHIFT_NONE , hw::coldfire>
    , reg<REG_STR("*SF_R"), RC_SHIFT, REG_SHIFT_RSVD , hw::coldfire>
>;

// cpu control registers (see movec for values)
using ctrl_reg_l = list<
    // 010/020/030/040/EC040/060/EC060/CPU32
      reg<REG_STR("sfc"), RC_CTRL, 0x000, hw::m68010>
    , reg<REG_STR("dfc"), RC_CTRL, 0x001, hw::m68010>
    , reg<REG_STR("usp"), RC_CTRL, 0x800, hw::m68010>   // name shared with RC_CPU reg
    , reg<REG_STR("vbr"), RC_CTRL, 0x801, hw::m68010>

    // 020/030/040/EC040/060/EC060
    , reg<REG_STR("cacr"), RC_CTRL, 0x002, hw::m68020>

    // 020/030/040/EC040
    , reg<REG_STR("caar"), RC_CTRL, 0x802, hw::m68020>
    , reg<REG_STR("msp"),  RC_CTRL, 0x803, hw::m68020>
    , reg<REG_STR("isp"),  RC_CTRL, 0x804, hw::m68020>

    // EC040
    // these mirror '040 regs: itt0/1 dtt0/1
    // First to match "hw" is disassembled.
    // Place *before* generic '040 to match special case of EC040
    , reg<REG_STR("iacr0"), RC_CTRL, 0x004, hw::m68ec040>
    , reg<REG_STR("iacr1"), RC_CTRL, 0x005, hw::m68ec040>
    , reg<REG_STR("dacr0"), RC_CTRL, 0x006, hw::m68ec040>
    , reg<REG_STR("dacr1"), RC_CTRL, 0x007, hw::m68ec040>

    // 040/LC040/060/EC060
    , reg<REG_STR("tc"),   RC_CTRL, 0x003, hw::m68040>
    , reg<REG_STR("itt0"), RC_CTRL, 0x004, hw::m68040>
    , reg<REG_STR("itt1"), RC_CTRL, 0x005, hw::m68040>
    , reg<REG_STR("dtt0"), RC_CTRL, 0x006, hw::m68040>
    , reg<REG_STR("dtt1"), RC_CTRL, 0x007, hw::m68040>

    // MMU Feature
    , reg<REG_STR("mmusr"), RC_CTRL, 0x805, hw::mmu>

    // 040/LC040/060/EC060
    , reg<REG_STR("urp"),  RC_CTRL, 0x806, hw::m68040>
    , reg<REG_STR("srp"),  RC_CTRL, 0x807, hw::m68040>

    // 060/EC060
    , reg<REG_STR("pcr"), RC_CTRL, 0x808, hw::m68060>

    // coldfire control registers
    , reg<REG_STR("cacr"), RC_CTRL, 0x002, hw::coldfire>
    , reg<REG_STR("asid"), RC_CTRL, 0x003, hw::coldfire>
    , reg<REG_STR("acr0"), RC_CTRL, 0x004, hw::coldfire>
    , reg<REG_STR("acr1"), RC_CTRL, 0x005, hw::coldfire>
    , reg<REG_STR("acr2"), RC_CTRL, 0x006, hw::coldfire>
    , reg<REG_STR("acr3"), RC_CTRL, 0x007, hw::coldfire>
    , reg<REG_STR("mmubar"), RC_CTRL, 0x008, hw::coldfire>

    , reg<REG_STR("vbr"), RC_CTRL, 0x801, hw::coldfire>    // also m68010 reg
    , reg<REG_STR("pc"),  RC_CTRL, 0x80f, hw::coldfire>    // name shared with RC_CPU reg

    , reg<REG_STR("rombar0"),  RC_CTRL, 0xc00, hw::coldfire>
    , reg<REG_STR("rombar1"),  RC_CTRL, 0xc01, hw::coldfire>
    , reg<REG_STR("rambar0"),  RC_CTRL, 0xc04, hw::coldfire>
    , reg<REG_STR("rambar1"),  RC_CTRL, 0xc05, hw::coldfire>
    , reg<REG_STR("mpcr"),     RC_CTRL, 0xc0c, hw::coldfire>
    , reg<REG_STR("edrambar"), RC_CTRL, 0xc0d, hw::coldfire>
    , reg<REG_STR("secbar"),   RC_CTRL, 0xc0e, hw::coldfire>
    , reg<REG_STR("mbar"),     RC_CTRL, 0xc0f, hw::coldfire>

    , reg<REG_STR("pcr1u0"),  RC_CTRL, 0xd02, hw::coldfire>
    , reg<REG_STR("pcr1l0"),  RC_CTRL, 0xd03, hw::coldfire>
    , reg<REG_STR("pcr2u0"),  RC_CTRL, 0xd04, hw::coldfire>
    , reg<REG_STR("pcr2l0"),  RC_CTRL, 0xd05, hw::coldfire>
    , reg<REG_STR("pcr3u0"),  RC_CTRL, 0xd06, hw::coldfire>
    , reg<REG_STR("pcr3l0"),  RC_CTRL, 0xd07, hw::coldfire>
    , reg<REG_STR("pcr1u1"),  RC_CTRL, 0xd0a, hw::coldfire>
    , reg<REG_STR("pcr1l1"),  RC_CTRL, 0xd0b, hw::coldfire>
    , reg<REG_STR("pcr2u1"),  RC_CTRL, 0xd0c, hw::coldfire>
    , reg<REG_STR("pcr2l1"),  RC_CTRL, 0xd0d, hw::coldfire>
    , reg<REG_STR("pcr3u1"),  RC_CTRL, 0xd0e, hw::coldfire>
    , reg<REG_STR("pcr3l1"),  RC_CTRL, 0xd0f, hw::coldfire>
    >;

// NB: KAS register "values" are internal only. 
// NB: MMU register values are a tuple of three nibbles:
//  { immediate width in bytes, PMOVE PREG value, PMOVE NUM value }

// declare m68851 BAD/BAC breakpoint registers
using mmu_bad = make_reg_seq<reg_seq<RC_MMU_68851, hw::m68851>
                           , REG_STR("bad"), 8, 0x2c0>;
using mmu_bac = make_reg_seq<reg_seq<RC_MMU_68851, hw::m68851>
                           , REG_STR("bac"), 8, 0x2d0>;

using mmu51_reg_l = list<
      reg<REG_STR("tc")  , RC_MMU_68851, 0x400, hw::m68851>
    , reg<REG_STR("drp") , RC_MMU_68851, 0x810, hw::m68851>
    , reg<REG_STR("srp") , RC_MMU_68851, 0x820, hw::m68851>
    , reg<REG_STR("crp") , RC_MMU_68851, 0x830, hw::m68851>
    , reg<REG_STR("cal") , RC_MMU_68851, 0x140, hw::m68851>
    , reg<REG_STR("val") , RC_MMU_68851, 0x150, hw::m68851>
    , reg<REG_STR("scc") , RC_MMU_68851, 0x160, hw::m68851>
    , reg<REG_STR("ac")  , RC_MMU_68851, 0x270, hw::m68851>
    , reg<REG_STR("psr") , RC_MMU_68851, 0x280, hw::m68851>
    , reg<REG_STR("pscr"), RC_MMU_68851, 0x290, hw::m68851>
    >;
 

// delare alternate names for registers
using m68k_reg_aliases_l = list<
      list<REG_STR("a6"), REG_STR("fp")>
    , list<REG_STR("a7"), REG_STR("sp")>
    >;
 
// combine all classes into single list
using m68k_all_reg_l = concat<list<>
                            , data_reg_l
                            , addr_reg_l
                            , supv_reg_l
                            , zaddr_reg_l
                            , fp_reg_l
                            , ctrl_reg_l
                            , fctrl_reg_l
                            , coldfire_mac_l
                            , mmu51_reg_l
                            , mmu_bad
                            , mmu_bac
                            >;

}

#undef REG_STR

#endif
