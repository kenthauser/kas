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


    // put reloc (two flavors)
    template <typename Rel>
    void put_reloc(Rel&& reloc)
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

        // emit reloc (convert to target format)
        reloc_p->put(object.cvt(reloc));
    }

    template <typename Rela>
    void put_reloc_a(Rela&& reloc)
    {
        // ignore if SHT_NOBITS
        if (s_header.sh_type == SHT_NOBITS)
            return;
        if (!reloc_a_p)    // allocate relocation segment if required
            reloc_a_p = new detail::es_reloc<Rela>(object
                                                 , SHT_RELA
                                                 , ".rela" + name
                                                 , index
                                                 );

        // emit reloc (convert to target format)
        reloc_a_p->put(object.cvt(reloc));
    }
    
    Elf64_Word   sym_num   {};       // STT_SECTION entry for this section
    elf_section *reloc_p   {};       // section for REL relocations
    elf_section *reloc_a_p {};       // section for RELA relocations
    elf_object&  object;             // containing object
};


}
#endif
