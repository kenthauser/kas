#ifndef KAS_ELF_ELF_FORMAT_H
#define KAS_ELF_ELF_FORMAT_H

#include "elf_external.h"       // headers
#include "elf_common.h"         // magic numbers
#include "elf_reloc.h"
#include "kas/endian.h" 


namespace kas::elf
{

// internally KAS uses ELF headers. 
// provide useful definitions/metafunctions to facility manipulation
namespace detail
{
    using namespace meta;
    using elf32_hdrs  = list<Elf32_Ehdr, Elf32_Shdr, Elf32_Sym
                                , Elf32_Rel, Elf32_Rela, Elf32_Phdr>;
    using elf64_hdrs  = list<Elf64_Ehdr, Elf64_Shdr, Elf64_Sym
                                , Elf64_Rel, Elf64_Rela, Elf64_Phdr>;

    // nominate largest supported ELF type as host type
    using host_hdrs = elf64_hdrs;

    // select appropriate (matching) header from HDR_LIST. 
    // Input type must be in either HDR_LIST or `host_hdrs`

    template <typename HDR_LIST, typename HDR>
    using hdr_idx = if_<in<host_hdrs, HDR>
                      , find_index<host_hdrs, HDR>
                      , find_index<HDR_LIST, HDR>
                      >;

    template <typename HDR_LIST, typename HDR>
    using TGT4HOST = at<HDR_LIST, hdr_idx<HDR_LIST, HDR>>;
    
    template <typename HDR_LIST, typename HDR>
    using HOST4TGT = at<host_hdrs, hdr_idx<HDR_LIST, HDR>>;
}

// header translation routines
using detail::TGT4HOST;
using detail::HOST4TGT;

// forward declare `string` elf_section XXX why?
struct es_string;

// base type `elf_format` only idenifies ENDIAN & relocations
// XXX need to make `elf_format` virtual type, not templated...
template <std::endian ENDIAN, typename HEADERS>
struct elf_format
{
    using headers = HEADERS;
    static constexpr std::endian endian = ENDIAN;

    // declare these as int to suppress narrowing messages.
    elf_format(elf_reloc_t const& relocs, int e_machine)
        : relocs(relocs)
    {
        header.e_type    = ET_REL;      // Relocatable file
        header.e_machine = e_machine;
        header.e_version = EV_CURRENT;
    }

    elf_reloc_t relocs;     // supported relocations
    Elf64_Ehdr  header{};   // prototype header
};
}

#endif
