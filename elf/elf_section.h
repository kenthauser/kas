#ifndef KAS_ELF_ELF_SECTION_H
#define KAS_ELF_ELF_SECTION_H


#include "elf_object.h"

namespace kas::elf
{

struct elf_section
{
    // standarized error message
    struct section_error : std::runtime_error
    {
        section_error(elf_section const& section, std::string fn, const char *msg)
            : section(section)
            , std::runtime_error("elf_section::"+fn+": "+section.name+": "+msg) 
            {}

        elf_section const& section;
    };

    // assembler constructor
    // generate corresponding ELF64 in host format
    elf_section(elf_object& obj
                , Elf64_Word sh_type
                , std::string const& name
                , unsigned alignment = {})
            : object(obj), name(name)
    {
        index = obj.add_section_p(this);    // allocate slot in object

        // intialize header
        s_header.sh_type      = sh_type;
#if 0
        if (!alignment)
        {
            // calculate default alignment. limit according to ELF type
            // allow alignment only if ent_size is power of 2
            
            if (ent_size & (ent_size - 1))
                alignment = 1;
            else if (ent_size)
                alignment = ent_size;
            else
                alignment = 1;
        }
#endif
        s_header.sh_addralign = alignment;
    }
    
    // append binary data to buffer
    void put(void const *p, std::size_t n)
    {
        // validate room in buffer
        while ((data_p + n) > data_endb)
        {
            // if not pre-allocated, resize vector
            if (data_base)
                throw section_error(*this, __FUNCTION__, "buffer overflow");

            // double memory each alloc
            static constexpr auto INITIAL_ALLOC = 1024;
            auto alloc_size = data_endb - data_base;
            if (alloc_size == 0)
                alloc_size = INITIAL_ALLOC;

            data_endb += alloc_size;
            if (s_header.sh_type != SHT_NOBITS)
                data.reserve(data_endb - data_base);
        }
        
        // write data to buffer or `std::vector` as appropriate
        if (data_base)
            std::memcpy(data_p, p, n);
        else if (s_header.sh_type != SHT_NOBITS)
        {
            // void * arithmetic not allowed...sigh
            auto first = static_cast<const char *>(p);
            data.insert(data.end(), first, first + n);
        }

        // accumulate offset even for `SHN_NOBITS`
        data_p += n;
    }

    void put(std::pair<void const *, std::size_t> const& s)
    {
        put(s.first, s.second);
    }

    // set size in bytes
    void set_size(Elf64_Xword new_size)
    {
        // can't resize
        if (s_header.sh_size)
            throw section_error(*this, __FUNCTION__, "can't resize");

        // can't set size after writing begins
        if (data_endb)
            throw section_error(*this, __FUNCTION__, "can't set size after writing");

        // XXX should modify `new_size `for `ent_size` & alignment
        // XXX also set `padding`
        if (s_header.sh_type != SHT_NOBITS)
        {
            data.reserve(new_size);
            data_base = data_p = data.data();
        }

        data_endb = data_base + new_size;
        s_header.sh_size = new_size;
    }

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
    elf_object&         object;         // containing object
    std::vector<char>   data;           // memory allocator
    char *              data_base {};
    char *              data_p    {};
    char *              data_endb {};

    std::string         name;
    Elf64_Shdr          s_header  {};   // host format 64-bit header
    uint16_t            index     {};   // 0-based index of section in `elf_object`
    uint16_t            padding   {};   // number of padding chars required
};
}
#endif
