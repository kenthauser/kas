#ifndef KBFD_KBFD_EXTERNAL_H
#define KBFD_KBFD_EXTERNAL_H

// Describe the structures which define KBFD sub-objects

// KBFD XXX

// internally KAS uses ELF headers. 
// provide useful definitions/metafunctions to facility manipulation



#include "elf_external.h"
#include <meta/meta.hpp>
#include <cstdint>

namespace kbfd
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

// header conversion routine return type
using cvt_src_rt = std::pair<void const *, std::size_t>;

}
#endif
