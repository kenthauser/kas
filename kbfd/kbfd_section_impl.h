#ifndef KBFD_KBFD_SECTION_IMPL_H
#define KBFD_KBFD_SECTION_IMPL_H

//
// implement non-trivial methods
//

#include "kbfd_section.h"

#include <string>

namespace kbfd
{

// generate corresponding ELF64 in host format
kbfd_section::kbfd_section(kbfd_object& obj
                , Elf64_Word sh_type
                , std::string const& name
                , unsigned alignment)
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
void kbfd_section::put(void const *p, std::size_t n)
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

void kbfd_section::set_size(Elf64_Xword new_size)
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
}
#endif
