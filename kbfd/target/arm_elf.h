#ifndef KAS_KBFD_TARGET_ARM_ELF_H
#define KAS_KBFD_TARGET_ARM_ELF_H

#include "arm.h"
#include "kbfd/kbfd_target_reloc.h"
#include "kbfd/kbfd_format_elf.h"
#include "kbfd/kbfd_format_elf_write.h"

namespace kbfd::arm
{

// Source: Document: ARM IHI 0044F (ELF for the ARM Architecture)
// Issue : 24 November 2015

static constexpr auto T_DEPR        = kbfd::kbfd_reloc::RFLAGS_DEPR;
static constexpr auto SB_REL        = kbfd::kbfd_reloc::RFLAGS_SB_REL;
static constexpr auto TLS           = kbfd::kbfd_reloc::RFLAGS_TLS;
static constexpr auto ARM_G0        = kbfd::kbfd_reloc::RFLAGS_ARM_G0;
static constexpr auto ARM_G1        = kbfd::kbfd_reloc::RFLAGS_ARM_G1;
static constexpr auto ARM_G2        = kbfd::kbfd_reloc::RFLAGS_ARM_G2;
static constexpr auto ARM_G3        = kbfd::kbfd_reloc::RFLAGS_ARM_G3;
static constexpr auto ARM_NC        = kbfd::kbfd_reloc::RFLAGS_ARM_NC;

// ARM relocations: fields = WKN Name Action width pc-rel flags...
static constexpr kbfd_target_reloc arm_elf_relocs[] =
{
    {   0, "R_ARM_NONE"     , K_REL_NONE() }
  , {   1, "R_ARM_PC24"     , K_REL_NONE()      , 32, 1, T_ARM, T_DEPR }
  , {   2, "R_ARM_ABS32"    , K_REL_ADD()       , 32, 0 }
  , {   3, "R_ARM_REL32"    , K_REL_ADD()       , 32, 1 }
  , {   4, "R_ARM_LDR_PC_G0", ARM_REL_A32LDR()  , 32, 1, T_ARM, ARM_G0 }
  , {   5, "R_ARM_ABS16"    , K_REL_ADD()       , 16, 0 }
  , {   6, "R_ARM_ABS12"    , ARM_REL_A32LDR()  , 32, 0, T_ARM }    // XXX off12
  , {   7, "R_ARM_THM_ABS5" , ARM_REL_ABS5()    , 16, 0, T_T16 }
  , {   8, "R_ARM_ABS8"     , K_REL_ADD()       ,  8, 0 }
  , {   9, "R_ARM_SBREL32"  , K_REL_ADD()       , 32, 0, SB_REL }
  , {  10, "R_ARM_THM_CALL" , ARM_REL_T32JUMP24()  , 32, 1, T_T16, ARM_G1 }
  , {  11, "R_ARM_THM_PC8"  , ARM_REL_PC8()     , 16, 1, T_T16 }
  , {  12, "R_ARM_BREL_ADJ" , K_REL_NONE()  }
  , {  13, "R_ARM_TLS_DESC" , K_REL_NONE()  }
  , {  17, "R_ARM_TLS_DTPMOD32", K_REL_NONE()   , 32 }
  , {  18, "R_ARM_TLS_DTPOFF32", K_REL_NONE()   , 32 }
  , {  19, "R_ARM_TLS_TPOFF32" , K_REL_NONE()   , 32 }
  , {  20, "R_ARM_COPY"     , K_REL_COPY() }
  , {  21, "R_ARM_GLOB_DAT" , K_REL_GOT()  }
  , {  22, "R_ARM_JUMP_SLOT", K_REL_JMP_SLOT() }
  , {  23, "R_ARM_RELATIVE" , K_REL_NONE() }
  , {  24, "R_ARM_GOTOFF32" , K_REL_GOT() }
  , {  25, "R_ARM_BASE_PREL", K_REL_NONE() }
  , {  26, "R_ARM_GOT_BREL" , K_REL_NONE() }
  , {  27, "R_ARM_PLT32"    , K_REL_ADD()      , 32, 1, T_ARM, T_DEPR }
  // NB: G1/G2 flags are used to differentiate between _CALL & _JUMP24
  , {  28, "R_ARM_CALL"     , ARM_REL_A32JUMP(), 32, 1, T_ARM, ARM_G1 }
  , {  29, "R_ARM_JUMP24"   , ARM_REL_A32JUMP(), 32, 1, T_ARM, ARM_G2 }
  , {  30, "R_ARM_THM_JUMP24", ARM_REL_T32JUMP24()    , 32, 0, T_T32}
  , {  31, "R_ARM_BASE_ABS" , K_REL_ADD()      , 32, 0, SB_REL }
  , {  35, "R_ARM_LDR_SBREL_11_0_NC" , K_REL_NONE(), 32, 0, T_ARM, T_DEPR }
  , {  36, "R_ARM_LDR_SBREL_19_12_NC", K_REL_NONE(), 32, 0, T_ARM, T_DEPR }
  , {  37, "R_ARM_LDR_SBREL_27_20_NC", K_REL_NONE(), 32, 0, T_ARM, T_DEPR }
  // XXX target1 *could* be PCREL ??
  , {  38, "R_ARM_TARGET1"  , K_REL_NONE() }
  , {  39, "R_ARM_SBREL31"  , K_REL_SADD()          , 32, 0, SB_REL, T_DEPR }
  , {  40, "R_ARM_V4BX"     , ARM_REL_V4BX()        , 32, 0, T_ARM }
  , {  41, "R_ARM_TARGET2"  , K_REL_NONE() }
  , {  42, "R_ARM_PREL31"   , K_REL_SADD()          , 32, 1 }
  , {  43, "R_ARM_MOVW_ABS_NC", ARM_REL_MOVW()        , 32, 0, T_ARM, ARM_NC }
  , {  44, "R_ARM_MOVT_ABS" , ARM_REL_MOVT()          , 32, 0, T_ARM }
  , {  45, "R_ARM_MOVW_PREL_NC", ARM_REL_MOVW()       , 32, 1, T_ARM, ARM_NC }
  , {  46, "R_ARM_MOVT_PREL"   , ARM_REL_MOVT()       , 32, 1, T_ARM }
  , {  47, "R_ARM_THM_MOVW_ABS_NC", K_REL_NONE()    , 32, 0, T_T32 }
  , {  48, "R_ARM_THM_MOVT_ABS" , K_REL_NONE()      , 32, 0, T_T32 }
  , {  49, "R_ARM_THM_MOVW_ABS_NC", K_REL_NONE()    , 32, 1, T_T32 }
  , {  50, "R_ARM_THM_MOVT_ABS" , K_REL_NONE()      , 32, 1, T_T32 }
  , {  51, "R_ARM_THM_JUMP19"   , K_REL_NONE()      , 32, 1, T_T32 }
  , {  52, "R_ARM_THM_JUMP6"    , K_REL_NONE()      , 16, 1, T_T16 }
  , {  53, "R_ARM_THM_ALU_PREL_11_0", K_REL_NONE()  , 32, 1, T_T32 }
  , {  54, "R_ARM_THM_PC12" , K_REL_NONE()      , 32, 1, T_T32 }
  // XXX _NOI clears THUMB bit on FUNCT symbol value
  , {  55, "R_ARM_ABS32_NOI"    , ARM_REL_NO_TFUNC(), 32, 0 }
  , {  56, "R_ARM_REL32_NOI"    , ARM_REL_NO_TFUNC(), 32, 1 }
  , {  57, "R_ARM_ALU_PC_G0_NC" , ARM_REL_A32ALU()  , 32, 1, T_ARM, ARM_G0, ARM_NC }
  , {  58, "R_ARM_ALU_PC_G0"    , ARM_REL_A32ALU()  , 32, 1, T_ARM, ARM_G0 }
  , {  59, "R_ARM_ALU_PC_G1_NC" , ARM_REL_A32ALU()  , 32, 1, T_ARM, ARM_G1, ARM_NC }
  , {  60, "R_ARM_ALU_PC_G1"    , ARM_REL_A32ALU()  , 32, 1, T_ARM, ARM_G1 }
  , {  61, "R_ARM_ALU_PC_G2_NC" , ARM_REL_A32ALU()  , 32, 1, T_ARM, ARM_G2, ARM_NC }
  , {  62, "R_ARM_LDR_PC_G1"    , ARM_REL_A32LDR()  , 32, 1, T_ARM, ARM_G1 }
  , {  63, "R_ARM_LDR_PC_G2"    , ARM_REL_A32LDR()  , 32, 1, T_ARM, ARM_G2 }
  , {  64, "R_ARM_LDRS_PC_G0"   , ARM_REL_A32LDRS() , 32, 1, T_ARM, ARM_G0 }
  , {  65, "R_ARM_LDRS_PC_G1"   , ARM_REL_A32LDRS() , 32, 1, T_ARM, ARM_G1 }
  , {  66, "R_ARM_LDRS_PC_G2"   , ARM_REL_A32LDRS() , 32, 1, T_ARM, ARM_G2 }
  , {  67, "R_ARM_LDC_PC_G0"    , ARM_REL_A32LDC()  , 32, 1, T_ARM, ARM_G0 }
  , {  68, "R_ARM_LDC_PC_G1"    , ARM_REL_A32LDC()  , 32, 1, T_ARM, ARM_G1 }
  , {  69, "R_ARM_LDC_PC_G2"    , ARM_REL_A32LDC()  , 32, 1, T_ARM, ARM_G2 }
  , {  70, "R_ARM_ALU_SB_G0_NC" , ARM_REL_A32ALU()  , 32, 0, T_ARM, SB_REL, ARM_G0, ARM_NC }
  , {  71, "R_ARM_ALU_SB_G0"    , ARM_REL_A32ALU()  , 32, 0, T_ARM, SB_REL, ARM_G0 }
  , {  72, "R_ARM_ALU_SB_G1_NC" , ARM_REL_A32ALU()  , 32, 0, T_ARM, SB_REL, ARM_G1, ARM_NC }
  , {  73, "R_ARM_ALU_SB_G1"    , ARM_REL_A32ALU()  , 32, 0, T_ARM, SB_REL, ARM_G1 }
  , {  74, "R_ARM_ALU_SB_G2"    , ARM_REL_A32ALU()  , 32, 0, T_ARM, SB_REL, ARM_G2 }
  , {  75, "R_ARM_LDR_SB_G0"    , ARM_REL_A32LDR()  , 32, 0, T_ARM, SB_REL, ARM_G0 }
  , {  76, "R_ARM_LDR_SB_G1"    , ARM_REL_A32LDR()  , 32, 0, T_ARM, SB_REL, ARM_G1 }
  , {  77, "R_ARM_LDR_SB_G2"    , ARM_REL_A32LDR()  , 32, 0, T_ARM, SB_REL, ARM_G2 }
  , {  78, "R_ARM_LDRS_SB_G0"   , ARM_REL_A32LDRS() , 32, 0, T_ARM, SB_REL, ARM_G0 }
  , {  79, "R_ARM_LDRS_SB_G1"   , ARM_REL_A32LDRS() , 32, 0, T_ARM, SB_REL, ARM_G1 }
  , {  80, "R_ARM_LDRS_SB_G2"   , ARM_REL_A32LDRS() , 32, 0, T_ARM, SB_REL, ARM_G2 }
  , {  81, "R_ARM_LDC_SB_G0"    , ARM_REL_A32LDC()  , 32, 0, T_ARM, SB_REL, ARM_G0 }
  , {  82, "R_ARM_LDC_SB_G1"    , ARM_REL_A32LDC()  , 32, 0, T_ARM, SB_REL, ARM_G1 }
  , {  83, "R_ARM_LDC_SB_G2"    , ARM_REL_A32LDC()  , 32, 0, T_ARM, SB_REL, ARM_G2 }
  , {  84, "R_ARM_MOVW_BREL_NC" , ARM_REL_MOVW()  , 32, 1, T_ARM, SB_REL, ARM_NC  }
  , {  85, "R_ARM_MOVT_BREL"    , ARM_REL_MOVT()  , 32, 1, T_ARM, SB_REL }
  , {  86, "R_ARM_MOVW_BREL"    , ARM_REL_MOVW()  , 32, 1, T_ARM, SB_REL }
  , {  87, "R_ARM_THM_MOVW_BREL_NC" , K_REL_NONE()  , 32, 1, T_T32 }
  , {  88, "R_ARM_THM_MOVT_BREL"    , K_REL_NONE()  , 32, 1, T_T32 }
  , {  89, "R_ARM_THM_MOVW_BREL" , K_REL_NONE()  , 32, 1, T_T32  }
  , {  90, "R_ARM_TLS_GOTDESC"  , K_REL_NONE()}
  , {  91, "R_ARM_TLS_CALL"     , K_REL_NONE()}
  , {  92, "R_ARM_TLS_DESCSEQ"  , K_REL_NONE()}
  , {  93, "R_ARM_THM_TLS_CALL" , K_REL_NONE()  , 32, 0, T_T32 }
  , {  94, "R_ARM_PLT32_ABS"    , K_REL_NONE()  , 32, 0 }
  , {  95, "R_ARM_GOT_ABS"      , K_REL_NONE()  , 32, 0 }
  , {  96, "R_ARM_GOT_PREL"     , K_REL_NONE()  , 32, 1 }
  , {  97, "R_ARM_GOT_BREL12"   , K_REL_NONE()  , 32, 1 }
  , {  98, "R_ARM_GOTOFF12"     , K_REL_NONE()  , 32, 1 }
  , {  99, "R_ARM_GOTRELAX"     , K_REL_NONE()  , 32, 0 }
  , { 100, "R_ARM_GNU_VTENTRY"  , K_REL_NONE()  , 32, 0, T_DEPR }
  , { 101, "R_ARM_GNU_VTINHERIT", K_REL_NONE()  , 32, 0, T_DEPR }
  , { 102, "R_ARM_THM_JUMP11"   , ARM_REL_JUMP11(), 16, 1, T_T16 }
  , { 103, "R_ARM_THM_JUMP8"    , ARM_REL_JUMP8() , 16, 1, T_T16 }
  , { 104, "R_ARM_TLS_GD32"     , K_REL_NONE()  , 32, 1 }
  , { 105, "R_ARM_TLS_LDM32"    , K_REL_NONE()  , 32, 1 }
  , { 106, "R_ARM_TLS_LDO32"    , K_REL_NONE()  , 32, 1 }
  , { 107, "R_ARM_TLS_IE32"     , K_REL_NONE()  , 32, 1 }
  , { 108, "R_ARM_TLS_LE32"     , K_REL_NONE()  , 32, 1 }
  , { 109, "R_ARM_TLS_LDO12"    , K_REL_NONE()  , 32, 1 }
  , { 110, "R_ARM_TLS_LE12"     , K_REL_NONE()  , 32, 1 }
  , { 111, "R_ARM_TLS_IE12GP"   , K_REL_NONE()  , 32, 1 }
  , { 129, "R_ARM_THM_TLS_DESCSEQ16", K_REL_NONE()  , 16, 0, T_T16 }
  , { 130, "R_ARM_THM_TLS_DESCSEQ32", K_REL_NONE()  , 32, 0, T_T32 }
  , { 131, "R_ARM_THM_GOT_BREL12", K_REL_NONE()     , 32, 1, T_T32 }
  , { 132, "R_ARM_THM_ALU_ABS_G0_NC", K_REL_NONE()  , 16, 0, T_T16 }
  , { 133, "R_ARM_THM_ALU_ABS_G1_NC", K_REL_NONE()  , 16, 0, T_T16 }
  , { 134, "R_ARM_THM_ALU_ABS_G2_NC", K_REL_NONE()  , 16, 0, T_T16 }
  , { 135, "R_ARM_THM_ALU_ABS_G3"   , K_REL_NONE()  , 16, 0, T_T16 }

};

struct arm_elf : elf32_format<std::endian::little>
{
    using base_t = elf32_format<std::endian::little>;
    using elf_use_rela = std::false_type;

    constexpr arm_elf()
        : base_t(arm_elf_relocs
               , std::extent_v<decltype(arm_elf_relocs)>
               , EM_ARM) {}
};

// register target format
template <> struct arm_formats_v<FORMAT_ELF> : meta::id<arm_elf> {};
}

#endif
