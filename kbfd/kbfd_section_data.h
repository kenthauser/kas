#ifndef KBFD_KBFD_SECTION_DATA_H
#define KBFD_KBFD_SECTION_DATA_H

#include "kbfd_section.h"
#include "kbfd_section_sym.h"
#include "kbfd_convert.h"

#include <deque>

namespace kbfd
{

namespace detail
{
    // create REL or RELA section based on `Reloc`
    // store RELOCs in host format. Convert to target on write.
    // use `ks_symbol` as model
    template <typename Reloc>
    struct ks_reloc : kbfd_section
    {
        ks_reloc(kbfd_object& obj
                , Elf64_Word sh_type, std::string const& name, unsigned data_index)
            : kbfd_section(obj, sh_type, name)
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
        static void gen_target_data(kbfd_object& obj, kbfd_section *s)
        {
            static_cast<ks_reloc*>(s)->do_gen_target(obj);
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
        void do_gen_target(kbfd_object& obj)
        {
            // XXX if passthru just modify pointers...
            auto cnt = host_table.size();           // get entry count...
            set_size(cnt * s_header.sh_entsize);    // ... and allocate memory
            
            // convert host -> target
            for (auto& s : host_table)
                kbfd_section::put(obj.cvt(s));
        }

        std::deque<Reloc> host_table;
    };
}

struct ks_data : kbfd_section
{
    ks_data(kbfd_object& obj
          , Elf64_Word sh_type
          , std::string const& name
          , unsigned alignment  = {}
          ) 
        : kbfd_section(obj, sh_type, name, alignment)          
        {}

    // append data into section. byte swap from host to target
    void put_int(int64_t data, uint8_t width)
    {
        this->put(object.swap(data, width), width);     
    }

    // put data (from memory array) into section (with byte-swapping)
    void put_data(void const *p, uint8_t width, unsigned count)
    {
        while (count--)
            this->put(object.swap(p, width), width);
    }

    // put raw data into buffer (no byte-swapping)
    void put_raw(void const *p, unsigned count)
    {
        this->put(p, count);
    }

    // put reloc using reloc (two flavors)
    template <typename Rel = Elf64_Rel>
    void put_reloc(kbfd_target_reloc const& info
                 , uint32_t sym_num)
    {
        // ignore if SHT_NOBITS
        if (s_header.sh_type == SHT_NOBITS)
            return;
        if (!reloc_p)      // allocate relocation segment if required
            reloc_p = new detail::ks_reloc<Rel>(object
                                              , SHT_REL
                                              , ".rel" + name
                                              , index
                                              );

        // emit reloc (generate in host format)
        reloc_p->add(object.cvt.create_reloc<Rel>(info, sym_num, position()));
    }

    template <typename Rel = Elf64_Rela>
    void put_reloc_a(kbfd_target_reloc const& info
                 , uint32_t sym_num
                 , int64_t  data)
    {
        // ignore if SHT_NOBITS
        if (s_header.sh_type == SHT_NOBITS)
            return;
        if (!reloc_a_p)      // allocate relocation segment if required
            reloc_a_p = new detail::ks_reloc<Rel>(object
                                              , SHT_RELA
                                              , ".rela" + name
                                              , index
                                              );

        // emit reloc (generate in host format)
        reloc_a_p->add(object.cvt.create_reloc<Rel>(info, sym_num
                                                  , position(), data));
    }

    Elf64_Word   sym_num   {};          // STT_SECTION entry for this section
    detail::ks_reloc<Elf64_Rel>  *reloc_p {};   // section for REL reloctions
    detail::ks_reloc<Elf64_Rela> *reloc_a_p {}; // section for REL reloctions
};


}
#endif
