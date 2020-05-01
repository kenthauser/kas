#ifndef KAS_M68K_M68K_REG_DEFN_H
#define KAS_M68K_M68K_REG_DEFN_H

// Define each m68k register as a "NAME/CLASS/VALUE/TST" type list

#include "m68k_reg_types.h"
#include "target/tgt_reg_trait.h"

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

// floating point user registers
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

    // coldfire MAC
    , reg<REG_STR("acc"),   RC_CPU, REG_CPU_ACC     , hw::mac>
    , reg<REG_STR("macsr"), RC_CPU, REG_CPU_MACSR   , hw::mac>
    , reg<REG_STR("mask"),  RC_CPU, REG_CPU_MASK    , hw::mac>
    , reg<REG_STR("<<"),    RC_CPU, REG_CPU_SF_LEFT , hw::mac>  // XXX tokens?
    , reg<REG_STR(">>"),    RC_CPU, REG_CPU_SF_RIGHT, hw::mac>

    // coldfire eMAC
    , reg<REG_STR("acc0"), RC_CPU, REG_CPU_ACC0, hw::emac> 
    , reg<REG_STR("acc1"), RC_CPU, REG_CPU_ACC1, hw::emac> 
    , reg<REG_STR("acc2"), RC_CPU, REG_CPU_ACC2, hw::emac> 
    , reg<REG_STR("acc3"), RC_CPU, REG_CPU_ACC3, hw::emac> 
    , reg<REG_STR("acc_ext01"), RC_CPU, REG_CPU_ACC_EXT01, hw::emac> 
    , reg<REG_STR("acc_ext23"), RC_CPU, REG_CPU_ACC_EXT23, hw::emac> 
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

// delare alternate names for registers
using m68k_reg_aliases_l = list<
      list<REG_STR("a6"), REG_STR("fp")>
    , list<REG_STR("a7"), REG_STR("sp")>
    >;

#undef REG_STR

// combine all classes into single list
using m68k_all_reg_l = concat<list<>
                            , data_reg_l
                            , addr_reg_l
                            , supv_reg_l
                            , zaddr_reg_l
                            , fp_reg_l
                            , ctrl_reg_l
                            , fctrl_reg_l
                            >;

}
#endif
