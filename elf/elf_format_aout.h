#ifndef KAS_ELF_ELF_FORMAT_AOUT_H
#define KAS_ELF_ELF_FORMAT_AOUT_H

#include "elf_format.h"
#include "elf_convert_aout.h"

namespace kas::elf
{

//using aout_hdrs  = meta::list<Elf64_Ehdr, Elf64_Shdr, nlist 
//                                , relocation_info, void, Elf64_Phdr>;
// no real headers...just symbols & relocations
using aout_hdrs  = meta::list<void, void, nlist, relocation_info, void, void>;


template <std::endian ENDIAN, typename HEADERS = aout_hdrs, typename...Ts>
struct elf_format_aout : elf_format<ENDIAN, HEADERS>
{
    using base_t = elf_format<ENDIAN, HEADERS>;

    // translate template arguments to ELF values
    static constexpr auto ei_data = 
          ENDIAN == std::endian::little ? ELFDATA2LSB
        : ENDIAN == std::endian::big    ? ELFDATA2MSB
        :                                 ELFDATANONE
        ;

    static constexpr auto ei_class = ELFCLASSNONE;

    // declare these as int to suppress narrowing messages.
    elf_format_aout( elf_reloc_t const& relocs
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
    
    // write a.out object
    void write(elf_object&, std::ostream&) const;
};
}

#endif
