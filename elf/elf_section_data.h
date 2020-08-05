#ifndef KAS_ELF_ELF_SECTION_DATA_H
#define KAS_ELF_ELF_SECTION_DATA_H


#include "elf_external.h"
#include "elf_common.h"
#include "elf_section.h"
#include "elf_section_sym.h"

namespace kas::elf
{

namespace detail
{
    // create REL or RELA section based on `Reloc`
    template <typename Reloc>
    struct es_reloc : elf_section
    {
        es_reloc(elf_object& obj
                , Elf64_Word sh_type, std::string const& name, unsigned data_index)
            : elf_section(obj, sh_type, name)
        {
            s_header.sh_link    = obj.symtab_p->index;
            s_header.sh_info    = data_index;
            s_header.sh_flags  |= SHF_INFO_LINK;
            s_header.sh_entsize = obj.cvt.tgt_size<std::remove_reference_t<Reloc>>();
        }
    };
}

struct es_data : elf_section
{
    es_data(elf_object& obj
          , Elf64_Word sh_type
          , std::string const& name
          , unsigned alignment  = {}
          ) 
        : object(obj), elf_section(obj, sh_type, name, alignment)          
        {}


    // put reloc using reloc (two flavors)
    template <typename Rel = Elf64_Rel>
    void put_reloc(kas_reloc_info const& info
                 , uint32_t sym_num
                 , uint8_t  offset)
    {
        // ignore if SHT_NOBITS
        if (s_header.sh_type == SHT_NOBITS)
            return;
        if (!reloc_p)      // allocate relocation segment if required
            reloc_p = new detail::es_reloc<Rel>(object
                                              , SHT_REL
                                              , ".rel" + name
                                              , index
                                              );

        // emit reloc (generate in host format)
        reloc_p->put(object.cvt.create_reloc<Rel>(info, sym_num, position(), offset));
    }

    template <typename Rel = Elf64_Rela>
    void put_reloc_a(kas_reloc_info const& info
                 , uint32_t sym_num
                 , uint8_t  offset
                 , int64_t  data)
    {
        // ignore if SHT_NOBITS
        if (s_header.sh_type == SHT_NOBITS)
            return;
        if (!reloc_a_p)      // allocate relocation segment if required
            reloc_a_p = new detail::es_reloc<Rel>(object
                                              , SHT_RELA
                                              , ".rela" + name
                                              , index
                                              );

        // emit reloc (generate in host format)
        reloc_p->put(object.cvt.create_reloc<Rel>(info, sym_num
                                                , position(), offset, data));
    }

    Elf64_Word   sym_num   {};       // STT_SECTION entry for this section
    elf_section *reloc_p   {};       // section for REL relocations
    elf_section *reloc_a_p {};       // section for RELA relocations
    elf_object&  object;             // containing object
};


}
#endif
