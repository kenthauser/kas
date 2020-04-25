#ifndef KAS_ELF_ELF_EXTERNAL_H
#define KAS_ELF_ELF_EXTERNAL_H

// describe the ELF file structures

// source: "System V Application Binary Interface - DRAFT - 24 April 2001"
// retrieved from http://refspecs.linuxfoundation.org/elf/gabi4+

// Add member type `elf_header_t` to facilitate EI_CLASS conversions in `elf_convert`.

// `elf_header_t` defined as the largest coresponding ELF header type.
// This facilitates EI_CLASS conversions in `elf_convert`.

#include <cstdint>

namespace kas::elf
{
// declare placeholder types for each ELF structure
// NB: declare as largest version of each structure
using Elf_Ehdr = struct Elf32_Ehdr;
using Elf_Shdr = struct Elf32_Shdr;
using Elf_Sym  = struct Elf32_Sym;
using Elf_Rel  = struct Elf32_Rel;
using Elf_Rela = struct Elf32_Rela;
using Elf_Phdr = struct Elf32_Phdr;

// declare each ELF data type in terms of c++ standard types
using Elf32_Addr    = std::uint32_t;
using Elf32_Off     = std::uint32_t;
using Elf32_Half    = std::uint16_t;
using Elf32_Word    = std::uint32_t;
using Elf32_Sword   = std::int32_t;

using Elf64_Addr    = std::uint64_t;
using Elf64_Off     = std::uint64_t;
using Elf64_Half    = std::uint16_t;
using Elf64_Word    = std::uint32_t;
using Elf64_Sword   = std::int32_t;
using Elf64_Xword   = std::uint64_t;
using Elf64_Sxword  = std::int64_t;

// ELF Header
static const unsigned EI_NIDENT = 16;

struct Elf32_Ehdr {
    using elf_header_t = Elf_Ehdr;
    unsigned char   e_ident[EI_NIDENT];
    Elf32_Half      e_type;
    Elf32_Half      e_machine;
    Elf32_Word      e_version;
    Elf32_Addr      e_entry;
    Elf32_Off       e_phoff;
    Elf32_Off       e_shoff;
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;
    Elf32_Half      e_phentsize;
    Elf32_Half      e_phnum;
    Elf32_Half      e_shentsize;
    Elf32_Half      e_shnum;
    Elf32_Half      e_shstrndx;
};

struct Elf64_Ehdr {
    using elf_header_t = Elf_Ehdr;
    unsigned char   e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
};


// ELF Section Header
struct Elf32_Shdr {
    using elf_header_t = Elf_Shdr;
    Elf32_Word      sh_name;
    Elf32_Word      sh_type;
    Elf32_Word      sh_flags;
    Elf32_Addr      sh_addr;
    Elf32_Off       sh_offset;
    Elf32_Word      sh_size;
    Elf32_Word      sh_link;
    Elf32_Word      sh_info;
    Elf32_Word      sh_addralign;
    Elf32_Word      sh_entsize;
};

struct Elf64_Shdr {
    using elf_header_t = Elf_Shdr;
    Elf64_Word      sh_name;
    Elf64_Word      sh_type;
    Elf64_Xword     sh_flags;
    Elf64_Addr      sh_addr;
    Elf64_Off       sh_offset;
    Elf64_Xword     sh_size;
    Elf64_Word      sh_link;
    Elf64_Word      sh_info;
    Elf64_Xword     sh_addralign;
    Elf64_Xword     sh_entsize;
};

// Special Section Indexes
#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_LOOS        0xff20
#define SHN_HIOS        0xff3f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_XINDEX      0xffff
#define SHN_HIRESERVE   0xffff


// ELF Symbol Table Entry
struct Elf32_Sym {
    using elf_header_t = Elf_Sym;
    Elf32_Word      st_name;
    Elf32_Addr      st_value;
    Elf32_Word      st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf32_Half      st_shndx;
};

struct Elf64_Sym {
    using elf_header_t = Elf_Sym;
    Elf64_Word      st_name;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf64_Half      st_shndx;
    Elf64_Addr      st_value;
    Elf64_Xword     st_size;
};


//  ELF Relocation Entries
struct Elf32_Rel {
    using elf_header_t = Elf_Rel;
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
};
struct Elf32_Rela {
    using elf_header_t = Elf_Rela;
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
    Elf32_Sword     r_addend;
};

struct Elf64_Rel {
    using elf_header_t = Elf_Rel;
    Elf64_Addr      r_offset;
    Elf64_Xword     r_info;
};
struct Elf64_Rela {
    using elf_header_t = Elf_Rela;
    Elf64_Addr      r_offset;
    Elf64_Xword     r_info;
    Elf64_Sxword    r_addend;
};


// ELF Program Table
struct Elf32_Phdr {
    using elf_header_t = Elf32_Phdr;
    Elf32_Word      p_type;
    Elf32_Off       p_offset;
    Elf32_Addr      p_vaddr;
    Elf32_Addr      p_paddr;
    Elf32_Word      p_filesz;
    Elf32_Word      p_memsz;
    Elf32_Word      p_flags;
    Elf32_Word      p_align;
};

struct Elf64_Phdr {
    using elf_header_t = Elf32_Phdr;
    Elf64_Word      p_type;
    Elf64_Word      p_flags;
    Elf64_Off       p_offset;
    Elf64_Addr      p_vaddr;
    Elf64_Addr      p_paddr;
    Elf64_Xword     p_filesz;
    Elf64_Xword     p_memsz;
    Elf64_Xword     p_align;
};

}
#endif
