#ifndef KAS_KBFD_TARGET_Z80_ELF_H
#define KAS_KBFD_TARGET_Z80_ELF_H

#include "kas_core/emit_object.h"      // for `obj_format` template
#include "kbfd/kbfd_format_aout.h"
#include "kbfd/kbfd_format_aout_write.h"

namespace kas::kbfd::z80
{

// Z80 relocations 
constexpr kas_reloc_info z80_aout_relocs[] =
{
    {  0 , "R_Z80_NONE"     , { K_REL_NONE      ,  0, 0 }}
  , {  1 , "R_Z80_32"       , { K_REL_ADD       , 32, 0 }}
  , {  2 , "R_Z80_16"       , { K_REL_ADD       , 16, 0 }}
  , {  3 , "R_Z80_8"        , { K_REL_ADD       ,  8, 0 }}
  , {  4 , "R_Z80_32PC"     , { K_REL_ADD       , 32, 1 }}
  , {  5 , "R_Z80_16PC"     , { K_REL_ADD       , 16, 1 }}
  , {  6 , "R_Z80_8PC"      , { K_REL_ADD       ,  8, 1 }}
  , {  7 , "R_Z80_GOT32"    , { K_REL_GOT       , 32, 1 }}
  , {  8 , "R_Z80_GOT16"    , { K_REL_GOT       , 16, 1 }}
  , {  9 , "R_Z80_GOT8"     , { K_REL_GOT       ,  8, 1 }}
  , { 10 , "R_Z80_GOT32O"   , { K_REL_GOT       , 32, 0 }}
  , { 11 , "R_Z80_GOT16O"   , { K_REL_GOT       , 16, 0 }}
  , { 12 , "R_Z80_GOT8O"    , { K_REL_GOT       ,  8, 0 }}
  , { 13 , "R_Z80_PLT32"    , { K_REL_PLT       , 32, 1 }}
  , { 14 , "R_Z80_PLT16"    , { K_REL_PLT       , 16, 1 }}
  , { 15 , "R_Z80_PLT8"     , { K_REL_PLT       ,  8, 1 }}
  , { 16 , "R_Z80_PLT32O"   , { K_REL_PLT       , 32, 0 }}
  , { 17 , "R_Z80_PLT16O"   , { K_REL_PLT       , 16, 0 }}
  , { 18 , "R_Z80_PLT8O"    , { K_REL_PLT       ,  8, 0 }}
  , { 19 , "R_Z80_COPY"     , { K_REL_COPY      , 32, 0 }}
  , { 20 , "R_Z80_GLOB_DAT" , { K_REL_GLOB_DAT  , 32, 0 }}
  , { 21 , "R_Z80_JMP_SLOT" , { K_REL_JMP_SLOT  , 32, 0 }}
};

struct z80_aout : kbfd_format_aout<std::endian::little>
{
    using base_t = kbfd_format_aout<std::endian::little>;
    using elf_use_rela = std::false_type;

    constexpr z80_aout()
        : base_t(relocs, EM_Z80) {}     // NB: EM_Z80 is `elf` name

    // relocs is `constexpr`
    static constexpr elf_reloc_t relocs { z80_aout_relocs, reloc_ops };
};
}

// set object format
namespace kas::core::detail
{
    template <> struct obj_format<void> : meta::id<kbfd::z80::z80_aout> {};
}

#endif
