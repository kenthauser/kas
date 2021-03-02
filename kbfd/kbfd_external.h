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

// currently, KBFD uses `ELF64` as internal format. However, if support
// for a richer object format is required in the future, these definitions 
// can be modified & the `kbfd_convert` facilities extended.
using kbfd_ehdr = struct Elf64_Ehdr;
using kbfd_shdr = struct Elf64_Shdr;
using kbfd_sym  = struct Elf64_Sym;
using kbfd_rel  = struct Elf64_Rel;
using kbfd_rela = struct Elf64_Rela;
using kbfd_phdr = struct Elf64_Phdr;

// these are aligned with `Elf64`, but should be used for `kbfd` interface
using kbfd_addr    = std::uint64_t;
using kbfd_off     = std::uint64_t;
using kbfd_half    = std::uint16_t;
using kbfd_word    = std::uint32_t;
using kbfd_sword   = std::int32_t;
using kbfd_xword   = std::uint64_t;
using kbfd_sxword  = std::int64_t;

// provide useful definitions/metafunctions to facility header manipulation
namespace detail
{
    using namespace meta;
    
    // list all headers in "canonical" order (for xlate metafunctions)
    using host_hdrs = list<kbfd_ehdr, kbfd_shdr, kbfd_sym
                                , kbfd_rel, kbfd_rela, kbfd_phdr>;

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

// expose header translation routines
using detail::TGT4HOST;
using detail::HOST4TGT;

// header conversion routine return type
using cvt_src_rt = std::pair<void const *, std::size_t>;

}
#endif
