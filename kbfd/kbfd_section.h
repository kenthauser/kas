#ifndef KBFD_KBFD_SECTION_H
#define KBFD_KBFD_SECTION_H


#include "kbfd_object.h"

#include <string>

namespace kbfd
{

struct kbfd_section
{
    // standarized error message
    struct section_error : std::runtime_error
    {
        section_error(kbfd_section const& section, std::string fn, const char *msg)
            : section(section)
            , std::runtime_error("kbfd_section::"+fn+": "+section.name+": "+msg) 
            {}

        kbfd_section const& section;
    };

    // assembler constructor
    // generate corresponding ELF64 in host format
    kbfd_section(kbfd_object& obj
                , Elf64_Word sh_type
                , std::string const& name
                , unsigned alignment = {});
    
    // append binary data to buffer
    void put(void const *p, std::size_t n);
    void put(std::pair<void const *, std::size_t> const& s)
    {
        put(s.first, s.second);
    }

    // set size in bytes
    void set_size(Elf64_Xword new_size);
    
    // size in bytes
    std::size_t size() const
    {
        return s_header.sh_size;
    }

    bool empty() const
    {
        return !data_endb;
    }

    // calculate current position in section
    Elf64_Xword position() const
    {
        // NB: also works for `SHT_NOBITS` case
        return data_p - data_base;
    }

    // allow data insertion via iterators
    const char *begin() const
    {
        return data_base ? data_base : data.data();
    }
    
    const char *end() const
    {   
        return begin() + position();
    }

    // NB: data is not `unsigned` because ostream::write complains
    // buffer can be pre-allocated or dynamically allocated in `vector`
    // If pre-allocated, `data_base` is set non-zero. If not pre-allocated
    // buffer will expand as required
    kbfd_object&        object;         // containing object
    std::vector<char>   data;           // memory allocator
    char *              data_base {};
    char *              data_p    {};
    char *              data_endb {};

    std::string         name;
    Elf64_Shdr          s_header  {};   // host format 64-bit header
    uint16_t            index     {};   // 0-based index of section in `kbfd_object`
    uint16_t            padding   {};   // number of padding chars required
};
}
#endif
