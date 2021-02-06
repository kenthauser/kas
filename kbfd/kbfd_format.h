#ifndef KBFD_KBFD_FORMAT_H
#define KBFD_KBFD_FORMAT_H

#include "kbfd_external.h"      // kbfd section headers
#include "elf_common.h"         // elf magic numbers
#include "kbfd_target_reloc.h"
#include "kbfd_convert.h"
#include "kas/endian.h"         // temp implementation of c++20 routine

namespace kbfd
{
// forward declare `string` kbfd_section XXX why?
struct ks_string;
struct kbfd_object;

// XXX declare lookup methods & return values
struct elf_reloc_t;
struct Elf64_Ehdr;
struct kbfd_format
{
    using target_reloc_index_t = typename kbfd_target_reloc::index_t;
    using reloc_hash_t         = typename kbfd_reloc::hash_t;
    using reloc_map_t          = std::map<reloc_hash_t, target_reloc_index_t>;

    template <typename HEADERS>
    constexpr kbfd_format(kbfd_target_reloc const *relocs
                        , std::size_t num_relocs
                        , std::endian end 
                        , HEADERS const& hdrs
                        , int = {})
        : relocs(relocs)
        , num_relocs(num_relocs)
        , swap(end)
        , cvt(/* *this,*/ swap, hdrs)
        {}

    // allow array "reloc[N]" to be passed directly
    template <typename HEADERS, std::size_t N, typename...Ts>
    constexpr kbfd_format(kbfd_target_reloc const relocs[N], Ts&&...args)
        : kbfd_format(relocs, N, std::forward<Ts>(args)...)
        {}

    // configure `kbfd` object using per-arch keyword/value pairs
    virtual const char *config(kbfd_object&, const char *item, uint64_t value) const
    {
        return {};
    };

    // basic target definition methods
    virtual Elf64_Ehdr init_header() const = 0;
    virtual void write(kbfd_object&, std::ostream&) const = 0;

    // provide interface to `target_reloc` tables
    virtual kbfd_target_reloc const *lookup(kbfd_reloc const& reloc) const;
    virtual kbfd_target_reloc const *get(target_reloc_index_t) const;

    kbfd_target_reloc   const *relocs;
    unsigned            num_relocs;
    swap_endian         swap;
    kbfd_convert        cvt;
};



// require ostream methods for symbols & relocs
template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Sym const&>);

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Rel const&>);

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Rela const&>);

}

#endif
