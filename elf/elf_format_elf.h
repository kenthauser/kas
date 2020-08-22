#ifndef KAS_ELF_ELF_FORMAT_ELF_H
#define KAS_ELF_ELF_FORMAT_ELF_H

#include "elf_format.h"
#include "elf_convert_elf.h"

namespace kas::elf
{

template <std::endian ENDIAN, typename HEADERS, typename...Ts>
struct elf_format_elf : elf_format<ENDIAN, HEADERS>
{
    using base_t = elf_format<ENDIAN, HEADERS>;

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
    elf_format_elf( elf_reloc_t const& relocs
                  , int   e_machine
                  , int   ei_flags = {}
                  , int   os_abi = {}
                  , int   abi_version = {}
                  )
        : base_t(relocs, e_machine)
    {

        unsigned char e_ident[EI_NIDENT] = 
                { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3
                  , static_cast<uint8_t>(ei_class)
                  , static_cast<uint8_t>(ei_data)
                  , EV_CURRENT
                  , static_cast<uint8_t>(os_abi)
                  , static_cast<uint8_t>(abi_version)
                  };

        std::memcpy(this->header.e_ident, e_ident, EI_NIDENT);

        this->header.e_type    = ET_REL;      // Relocatable file
        this->header.e_machine = e_machine;
        this->header.e_version = EV_CURRENT;
        this->header.e_flags   = ei_flags;

        // no `cvt` object, but could use `ei_class`
        // calculated header size is 32/64 depending on ei_class
        if constexpr(ei_class == ELFCLASS64)
        {
            this->header.e_ehsize    = sizeof(Elf64_Ehdr);
            this->header.e_phentsize = sizeof(Elf64_Phdr);
            this->header.e_shentsize = sizeof(Elf64_Shdr);
        }
        // guess it must be ELF32...
        else
        {
            this->header.e_ehsize    = sizeof(Elf32_Ehdr);
            this->header.e_phentsize = sizeof(Elf32_Phdr);
            this->header.e_shentsize = sizeof(Elf32_Shdr);
        }

    }
    
    // write ELF object
    void write(elf_object&, std::ostream&) const;
};

template <std::endian ENDIAN, typename...Ts>
using elf32_format = elf_format_elf<ENDIAN, detail::elf32_hdrs, Ts...>;

template <std::endian ENDIAN, typename...Ts>
using elf64_format = elf_format_elf<ENDIAN, detail::elf64_hdrs, Ts...>;


}

#endif
