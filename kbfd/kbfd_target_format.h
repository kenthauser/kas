#ifndef KBFD_KBFD_TARGET_FORMAT_H
#define KBFD_KBFD_TARGET_FORMAT_H

#include "kbfd_external.h"      // kbfd section headers
#include "elf_common.h"         // elf magic numbers
#include "kbfd_target_reloc.h"
#include "kbfd_convert.h"
#include "kas/endian.h"         // temp implementation of c++20 routine

namespace kbfd
{
// describe relocation formats allowed by format
enum class kbfd_rela_fmt
{
      RELA_ALLOW        // use RELA if value doesn't fit
    , RELA_NONE         // don't allow RELA
    , RELA_REQUIRE      // always use RELA
};

// forward declare referenced types
struct kbfd_object;
struct kbfd_target_sections;

struct kbfd_target_format
{
    // work around g++ virtual base bug
    using vt_base_t = kbfd_target_format;

    using target_reloc_index_t = typename kbfd_target_reloc::index_t;
    using reloc_key_t          = typename kbfd_reloc::key_t;
    using reloc_map_t          = std::map<reloc_key_t, target_reloc_index_t>;

    template <typename HEADERS>
    constexpr kbfd_target_format(kbfd_target_reloc const *relocs
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
    constexpr kbfd_target_format(kbfd_target_reloc const relocs[N], Ts&&...args)
        : kbfd_target_format(relocs, N, std::forward<Ts>(args)...)
        {}

    // configure `kbfd` object using per-arch keyword/value pairs
    virtual const char *config(kbfd_object&, const char *item, uint64_t value) const
    {
        return {};
    };

    // basic target definition methods
    virtual kbfd_ehdr init_header() const = 0;
    virtual void write(kbfd_object&, std::ostream&) const = 0;

    // provide interface to `target_sections` definitions
    virtual kbfd_target_sections const& get_section_defns() const;

    // provide interface to `target_reloc` tables
    virtual kbfd_target_reloc const *lookup(kbfd_reloc const& reloc) const;
    virtual kbfd_target_reloc const *get_p(target_reloc_index_t) const;

    kbfd_target_reloc   const *relocs;
    target_reloc_index_t       num_relocs;
    target_reloc_index_t       max_relocs { num_relocs };
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
