#ifndef KBFD_KBFD_SECTION_IMPL_H
#define KBFD_KBFD_SECTION_IMPL_H

//
// implement non-trivial methods
//

#include "kbfd_section.h"

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
    // if not enough room in buffer, expand buffer
    while ((data_p + n) > data_endb)
    {
        // if pre-allocated, no-room is logic error
        if (data_base)
            throw section_error(*this, __FUNCTION__, "buffer overflow");

        // if dynamically allocated, double memory allocated each iteration
        static constexpr auto INITIAL_ALLOCATION = 1024;

        // if not first allocation, double buffer size
        if (auto buf_size = data_endb - data_base)
            data_endb += buf_size;
        else
            data_endb += INITIAL_ALLOCATION;

        // extend buffer to new size
        if (s_header.sh_type != SHT_NOBITS)
            data.reserve(data_endb - data_base);
    }
    
    // write data to buffer or `std::vector` as appropriate
    if (data_base)
        std::memcpy(data_p, p, n);
    else if (s_header.sh_type != SHT_NOBITS)
    {
        // c++: void * arithmetic not allowed...sigh
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
