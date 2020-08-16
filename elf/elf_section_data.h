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
    // store RELOCs in host format. Convert to target on write.
    // use `es_symbol` as model
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

        // host objects use "add", not "put"
        void put(void const *, std::size_t) = delete;

        // generate target from host table
        // do `static_cast` in object, not calling location
        static void gen_target_data(elf_object& obj, elf_section *s)
        {
            static_cast<es_reloc*>(s)->do_gen_target(obj);
        }

        // make room for expected "entries"
        void reserve(Elf64_Word size)
        {
            host_table.reserve(size);
        }
        
        // add symbol to host symbol table: `Reloc`
        Elf64_Word add(Reloc const& new_obj)
        {
            // object number is index value (zero-based)
            auto obj_num = host_table.size();

            // create host object & return reference
            auto& obj = host_table.emplace_back(new_obj);

            return obj_num;
        }

    private:
        // convert "host" object to "target" object
        void do_gen_target(elf_object& obj)
        {
            // XXX if passthru just modify pointers...
            auto cnt = host_table.size();           // get entry count...
            set_size(cnt * s_header.sh_entsize);    // ... and allocate memory
            
            // convert host -> target
            for (auto& s : host_table)
                elf_section::put(obj.cvt(s));
        }

        std::deque<Reloc> host_table;
    };
}

struct es_data : elf_section
{
    es_data(elf_object& obj
          , Elf64_Word sh_type
          , std::string const& name
          , unsigned alignment  = {}
          ) 
        : elf_section(obj, sh_type, name, alignment)          
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
        reloc_p->add(object.cvt.create_reloc<Rel>(info, sym_num, position(), offset));
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
        reloc_a_p->add(object.cvt.create_reloc<Rel>(info, sym_num
                                                  , position(), offset, data));
    }

    Elf64_Word   sym_num   {};          // STT_SECTION entry for this section
    detail::es_reloc<Elf64_Rel>  *reloc_p {};   // section for REL reloctions
    detail::es_reloc<Elf64_Rela> *reloc_a_p {}; // section for REL reloctions
};


}
#endif
