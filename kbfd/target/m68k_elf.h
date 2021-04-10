#ifndef KAS_KBFD_TARGET_M68K_ELF_H
#define KAS_KBFD_TARGET_M68K_ELF_H

#include "m68k.h"
#include "kbfd/kbfd_target_reloc.h"
#include "kbfd/kbfd_format_elf.h"
#include "kbfd/kbfd_format_elf_write.h"

namespace kbfd::m68k
{

// M68K relocations: fields = WKN Name Action width pc-rel
static constexpr kbfd_target_reloc m68k_elf_relocs[] =
{
    {  0 , "R_68K_NONE"     , K_REL_NONE()    ,  0, 0 }
  , {  1 , "R_68K_32"       , K_REL_ADD()     , 32, 0 }
  , {  2 , "R_68K_16"       , K_REL_ADD()     , 16, 0 }
  , {  3 , "R_68K_8"        , K_REL_ADD()     ,  8, 0 }
  , {  4 , "R_68K_32PC"     , K_REL_ADD()     , 32, 1 }
  , {  5 , "R_68K_16PC"     , K_REL_ADD()     , 16, 1 }
  , {  6 , "R_68K_8PC"      , K_REL_ADD()     ,  8, 1 }
#if 0
  , {  7 , "R_68K_GOT32"    , K_REL_GOT()     , 32, 1 }
  , {  8 , "R_68K_GOT16"    , K_REL_GOT()     , 16, 1 }
  , {  9 , "R_68K_GOT8"     , K_REL_GOT()     ,  8, 1 }
  , { 10 , "R_68K_GOT32O"   , K_REL_GOT()     , 32, 0 }
  , { 11 , "R_68K_GOT16O"   , K_REL_GOT()     , 16, 0 }
  , { 12 , "R_68K_GOT8O"    , K_REL_GOT()     ,  8, 0 }
  , { 13 , "R_68K_PLT32"    , K_REL_PLT()     , 32, 1 }
  , { 14 , "R_68K_PLT16"    , K_REL_PLT()     , 16, 1 }
  , { 15 , "R_68K_PLT8"     , K_REL_PLT()     ,  8, 1 }
  , { 16 , "R_68K_PLT32O"   , K_REL_PLT()     , 32, 0 }
  , { 17 , "R_68K_PLT16O"   , K_REL_PLT()     , 16, 0 }
  , { 18 , "R_68K_PLT8O"    , K_REL_PLT()     ,  8, 0 }
  , { 19 , "R_68K_COPY"     , K_REL_COPY()    , 32, 0 }
  , { 20 , "R_68K_GLOB_DAT" , K_REL_GLOB_DAT(), 32, 0 }
  , { 21 , "R_68K_JMP_SLOT" , K_REL_JMP_SLOT(), 32, 0 }
#endif
};

struct m68k_elf : elf32_format<std::endian::big>
{
    using base_t = elf32_format<std::endian::big>;
    using elf_use_rela = std::false_type;

    constexpr m68k_elf()
        : base_t(m68k_elf_relocs
               , std::extent_v<decltype(m68k_elf_relocs)>
               , EM_68K) {}
};

// register target format
template <> struct m68k_formats_v<FORMAT_ELF> : m68k_elf {};
}

#endif
