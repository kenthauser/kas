#ifndef KBFD_KBFD_FORMAT_ELF_H
#define KBFD_KBFD_FORMAT_ELF_H

#include "kbfd_format.h"
#include "kbfd_convert_elf.h"

namespace kbfd
{

template <std::endian ENDIAN, typename HEADERS, typename...Ts>
struct kbfd_format_elf : kbfd_format //kbfd_format<ENDIAN, HEADERS>
{
    using base_t = kbfd_format;//<ENDIAN, HEADERS>;

    // translate template arguments to ELF values
    static constexpr auto ei_data = 
          ENDIAN == std::endian::little ? ELFDATA2LSB
        : ENDIAN == std::endian::big    ? ELFDATA2MSB
        :                                 ELFDATANONE
        ;
    static constexpr auto ei_class = 
          std::is_same_v<HEADERS, detail::elf32_hdrs> ? ELFCLASS32
        : std::is_same_v<HEADERS, detail::elf64_hdrs> ? ELFCLASS64
        :                                               ELFCLASSNONE
        ;

    // declare these as int to suppress narrowing messages.
    constexpr kbfd_format_elf(kbfd_target_reloc const *relocs
                  , std::size_t num_relocs
                  , int   e_machine
                  , int   ei_flags = {}
                  , int   os_abi = {}
                  , int   abi_version = {}
                  )
        : e_machine(e_machine),  base_t(relocs, num_relocs, ENDIAN, HEADERS()) {}


    // allow initialization via actual array of `kbfd_target_reloc`
    template <std::size_t N, typename...Args>
    constexpr kbfd_format_elf(kbfd_target_reloc const relocs[N], Args&&...args)
        : kbfd_format_elf(relocs, N, std::forward<Args>(args)...)
        {}


    Elf64_Ehdr init_header() const override
    {
        static constexpr unsigned char e_ident[EI_NIDENT] = 
                { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3
                  , static_cast<uint8_t>(ei_class)
                  , static_cast<uint8_t>(ei_data)
                  , EV_CURRENT
#if 0
                  , static_cast<uint8_t>(os_abi)
                  , static_cast<uint8_t>(abi_version)
#endif
                  };

        Elf64_Ehdr header;
        std::memcpy(header.e_ident, e_ident, EI_NIDENT);

        header.e_type    = ET_REL;      // Relocatable file
        header.e_machine = e_machine;
        header.e_version = EV_CURRENT;
        // XXX header.e_flags   = ei_flags;

        // no `cvt` object, but could use `ei_class`
        // calculated header size is 32/64 depending on ei_class
        if constexpr(ei_class == ELFCLASS64)
        {
            header.e_ehsize    = sizeof(Elf64_Ehdr);
            header.e_phentsize = sizeof(Elf64_Phdr);
            header.e_shentsize = sizeof(Elf64_Shdr);
        }
        // guess it must be ELF32...
        else
        {
            header.e_ehsize    = sizeof(Elf32_Ehdr);
            header.e_phentsize = sizeof(Elf32_Phdr);
            header.e_shentsize = sizeof(Elf32_Shdr);
        }
        
        return header;
    }
    
    // write ELF object
    void write(kbfd_object&, std::ostream&) const override;

    int e_machine {};
};

template <std::endian ENDIAN, typename...Ts>
using elf32_format = kbfd_format_elf<ENDIAN, detail::elf32_hdrs, Ts...>;

template <std::endian ENDIAN, typename...Ts>
using elf64_format = kbfd_format_elf<ENDIAN, detail::elf64_hdrs, Ts...>;


}

#endif
