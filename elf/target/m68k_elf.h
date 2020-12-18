#ifndef KAS_ELF_TARGET_M68K_ELF_H
#define KAS_ELF_TARGET_M68K_ELF_H

#include "kas_core/emit_object.h"       // for `obj_format` template
#include "m68k.h"
#include "elf/elf_reloc.h"
#include "elf/elf_format_elf.h"         // XXX needs to be "relative"
#include "elf/elf_format_elf_write.h"

namespace kas::elf::m68k
{

// M68K relocations: fields = WKN Name Action width pc-rel
constexpr kas_reloc_info const m68k_elf_relocs[] =
{
#if 1
    {  0 , "R_68K_NONE"     , { K_REL_NONE      ,  0, 0 }}
  , {  1 , "R_68K_32"       , { K_REL_ADD       , 32, 0 }}
  , {  2 , "R_68K_16"       , { K_REL_ADD       , 16, 0 }}
  , {  3 , "R_68K_8"        , { K_REL_ADD       ,  8, 0 }}
  , {  4 , "R_68K_32PC"     , { K_REL_ADD       , 32, 1 }}
  , {  5 , "R_68K_16PC"     , { K_REL_ADD       , 16, 1 }}
  , {  6 , "R_68K_8PC"      , { K_REL_ADD       ,  8, 1 }}
  , {  7 , "R_68K_GOT32"    , { K_REL_GOT       , 32, 1 }}
  , {  8 , "R_68K_GOT16"    , { K_REL_GOT       , 16, 1 }}
  , {  9 , "R_68K_GOT8"     , { K_REL_GOT       ,  8, 1 }}
  , { 10 , "R_68K_GOT32O"   , { K_REL_GOT       , 32, 0 }}
  , { 11 , "R_68K_GOT16O"   , { K_REL_GOT       , 16, 0 }}
  , { 12 , "R_68K_GOT8O"    , { K_REL_GOT       ,  8, 0 }}
  , { 13 , "R_68K_PLT32"    , { K_REL_PLT       , 32, 1 }}
  , { 14 , "R_68K_PLT16"    , { K_REL_PLT       , 16, 1 }}
  , { 15 , "R_68K_PLT8"     , { K_REL_PLT       ,  8, 1 }}
  , { 16 , "R_68K_PLT32O"   , { K_REL_PLT       , 32, 0 }}
  , { 17 , "R_68K_PLT16O"   , { K_REL_PLT       , 16, 0 }}
  , { 18 , "R_68K_PLT8O"    , { K_REL_PLT       ,  8, 0 }}
  , { 19 , "R_68K_COPY"     , { K_REL_COPY      , 32, 0 }}
  , { 20 , "R_68K_GLOB_DAT" , { K_REL_GLOB_DAT  , 32, 0 }}
  , { 21 , "R_68K_JMP_SLOT" , { K_REL_JMP_SLOT  , 32, 0 }}
#elif 1
    {  0 , "R_68K_NONE"     , { K_REL_NONE()    ,  0, 0 }}
  , {  1 , "R_68K_32"       , { K_REL_ADD()     , 32, 0 }}
  , {  2 , "R_68K_16"       , { K_REL_ADD()     , 16, 0 }}
  , {  3 , "R_68K_8"        , { K_REL_ADD()     ,  8, 0 }}
  , {  4 , "R_68K_32PC"     , { K_REL_ADD()     , 32, 1 }}
  , {  5 , "R_68K_16PC"     , { K_REL_ADD()     , 16, 1 }}
  , {  6 , "R_68K_8PC"      , { K_REL_ADD()     ,  8, 1 }}
  , {  7 , "R_68K_GOT32"    , { K_REL_GOT()     , 32, 1 }}
  , {  8 , "R_68K_GOT16"    , { K_REL_GOT()     , 16, 1 }}
  , {  9 , "R_68K_GOT8"     , { K_REL_GOT()     ,  8, 1 }}
  , { 10 , "R_68K_GOT32O"   , { K_REL_GOT()     , 32, 0 }}
  , { 11 , "R_68K_GOT16O"   , { K_REL_GOT()     , 16, 0 }}
  , { 12 , "R_68K_GOT8O"    , { K_REL_GOT()     ,  8, 0 }}
  , { 13 , "R_68K_PLT32"    , { K_REL_PLT()     , 32, 1 }}
  , { 14 , "R_68K_PLT16"    , { K_REL_PLT()     , 16, 1 }}
  , { 15 , "R_68K_PLT8"     , { K_REL_PLT()     ,  8, 1 }}
  , { 16 , "R_68K_PLT32O"   , { K_REL_PLT()     , 32, 0 }}
  , { 17 , "R_68K_PLT16O"   , { K_REL_PLT()     , 16, 0 }}
  , { 18 , "R_68K_PLT8O"    , { K_REL_PLT()     ,  8, 0 }}
  , { 19 , "R_68K_COPY"     , { K_REL_COPY()    , 32, 0 }}
  , { 20 , "R_68K_GLOB_DAT" , { K_REL_GLOB_DAT(), 32, 0 }}
  , { 21 , "R_68K_JMP_SLOT" , { K_REL_JMP_SLOT(), 32, 0 }}
#else
    kas_reloc_info<K_REL_NONE    >( 0 , "R_68K_NONE"     ,  0, false)
  , kas_reloc_info<K_REL_ADD     >( 1 , "R_68K_32"       , 32, false)
  , kas_reloc_info<K_REL_ADD     >( 2 , "R_68K_16"       , 16, false)
  , kas_reloc_info<K_REL_ADD     >( 3 , "R_68K_8"        ,  8, false)
  , kas_reloc_info<K_REL_ADD     >( 4 , "R_68K_32PC"     , 32, true )
  , kas_reloc_info<K_REL_ADD     >( 5 , "R_68K_16PC"     , 16, true )
  , kas_reloc_info<K_REL_ADD     >( 6 , "R_68K_8PC"      ,  8, true )
  , kas_reloc_info<K_REL_GOT     >( 7 , "R_68K_GOT32"    , 32, true )
  , kas_reloc_info<K_REL_GOT     >( 8 , "R_68K_GOT16"    , 16, true )
  , kas_reloc_info<K_REL_GOT     >( 9 , "R_68K_GOT8"     ,  8, true )
  , kas_reloc_info<K_REL_GOT     >(10 , "R_68K_GOT32O"   , 32, false)
  , kas_reloc_info<K_REL_GOT     >(11 , "R_68K_GOT16O"   , 16, false)
  , kas_reloc_info<K_REL_GOT     >(12 , "R_68K_GOT8O"    ,  8, false)
  , kas_reloc_info<K_REL_PLT     >(13 , "R_68K_PLT32"    , 32, true )
  , kas_reloc_info<K_REL_PLT     >(14 , "R_68K_PLT16"    , 16, true )
  , kas_reloc_info<K_REL_PLT     >(15 , "R_68K_PLT8"     ,  8, true )
  , kas_reloc_info<K_REL_PLT     >(16 , "R_68K_PLT32O"   , 32, false)
  , kas_reloc_info<K_REL_PLT     >(17 , "R_68K_PLT16O"   , 16, false)
  , kas_reloc_info<K_REL_PLT     >(18 , "R_68K_PLT8O"    ,  8, false)
  , kas_reloc_info<K_REL_COPY    >(19 , "R_68K_COPY"     , 32, false)
  , kas_reloc_info<K_REL_GLOB_DAT>(20 , "R_68K_GLOB_DAT" , 32, false)
  , kas_reloc_info<K_REL_JMP_SLOT>(21 , "R_68K_JMP_SLOT" , 32, false)
#endif
};

struct m68k_elf : elf32_format<std::endian::big>
{
    using base_t = elf32_format<std::endian::big>;
    using elf_use_rela = std::false_type;

    m68k_elf()
        : base_t(relocs, EM_68K)
    {
        // base.set_machine(xxx)

    }

    // relocs is `constexpr
    static constexpr elf_reloc_t relocs { m68k_elf_relocs, reloc_ops_p };
};


// expose obj format
template <> struct m68k_formats_v<FORMAT_ELF> : meta::id<m68k_elf> {};

}

// set object format
namespace kas::core::detail
{
    template <> struct obj_format<void> : meta::id<elf::m68k::m68k_elf> {};
}

#endif
