#ifndef KBFD_KBFD_FORMAT_H
#define KBFD_KBFD_FORMAT_H

#include "kbfd_external.h"      // kbfd section headers
#include "elf_common.h"         // elf magic numbers
#include "kbfd_reloc.h"
#include "kbfd_convert.h"
#include "kas/endian.h"         // temp implementation of c++20 routine

namespace kbfd
{
// forward declare `string` kbfd_section XXX why?
struct ks_string;
struct kbfd_object;

#if 0
// base type `kbfd_format` only idenifies ENDIAN & relocations
// XXX need to make `kbfd_format` virtual type, not templated...
template <std::endian ENDIAN, typename HEADERS>
struct kbfd_format
{
    using headers = HEADERS;
    static constexpr std::endian endian = ENDIAN;

    // declare these as int to suppress narrowing messages.
    // XXX needs work for constexpr xlate
    kbfd_format(elf_reloc_t const& relocs, int e_machine = {})
        : relocs(relocs)
    {
        header.e_type    = ET_REL;      // Relocatable file
        header.e_machine = e_machine;
        header.e_version = EV_CURRENT;
    }

    void write(kbfd_object&, std::ostream& os) const;


    elf_reloc_t relocs;     // supported relocations
    Elf64_Ehdr  header{};   // prototype header
};
#else
// XXX declare lookup methods & return values
struct elf_reloc_t;
struct Elf64_Ehdr;
struct kbfd_format
{
    template <typename HEADERS>
    constexpr kbfd_format(elf_reloc_t const& relocs
                        , std::endian end 
                        , HEADERS const& hdrs
                        , int = {})
        : relocs(relocs)
        , swap(end)
        , cvt(/* *this,*/ swap, hdrs)
        {}

    virtual const char *config(kbfd_object&, const char *item, uint64_t value) const
    {
        return {};
    };

    virtual Elf64_Ehdr init_header() const = 0;
    virtual void write(kbfd_object&, std::ostream&) const = 0;

    elf_reloc_t const& relocs;
    swap_endian         swap;
    kbfd_convert        cvt;
};


#endif


// require ostream methods for symbols & relocs
template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Sym const&>);

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Rel const&>);

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM&, std::pair<kbfd_object const&, Elf64_Rela const&>);

}

#endif
