#ifndef KAS_ELF_ELF_CONVERT_H
#define KAS_ELF_ELF_CONVERT_H

// Convert ELF Class (ELF32/ELF64) and data ENDIAN between host and target formats
//
// When headers are in memory, they are stored as ELF64 with host-endian.
// Data in sections is always stored in target format (class/endian)
// `elf_convert` supports both host->target & target->host translations
// of headers & byte-swap for endian conversion of data emited to data sections.

#include "elf_external.h"
#include "elf_common.h"

#include <type_traits>
#include "kas/endian.h"

namespace kas::elf {

static inline constexpr uint8_t HOST_ENDIAN() 
{
    return (std::endian::native == std::endian::little)
                ? ELFDATA2LSB : ELFDATA2MSB; 
};

namespace detail
{
    using namespace meta;
    using elf_hdr_idx = list<Elf_Ehdr,   Elf_Shdr,   Elf_Sym,   Elf_Rel,   Elf_Rela,   Elf_Phdr>;
    using elf32_hdrs  = list<Elf32_Ehdr, Elf32_Shdr, Elf32_Sym, Elf32_Rel, Elf32_Rela, Elf32_Phdr>;
    using elf64_hdrs  = list<Elf64_Ehdr, Elf64_Shdr, Elf64_Sym, Elf64_Rel, Elf64_Rela, Elf64_Phdr>;

    // nominate largest supported ELF type
    using elf_largest = elf64_hdrs;
    
    template <typename T, typename IDX = typename T::elf_header_t>
    constexpr auto hdr_index = find_index<elf_hdr_idx, IDX>::value;

    struct largest_type
    {
        template <typename T1, typename T2>
        using invoke = if_<less<sizeof_<T1>, sizeof_<T2>>
                          , T2
                          , T1
                          >;
    };

    // used to size `zero` paddinga
    using largest_hdr = fold<elf_largest, nil_, largest_type>;

    struct swap_endian
    {
        constexpr swap_endian(
                  uint8_t ext_endian
                , uint8_t host_endian = HOST_ENDIAN()
                ) : do_swap(ext_endian != host_endian)
            {}

        constexpr auto operator()(uint64_t value) const
        {
            if (!do_swap) return value;

            value = (value & 0x0000'0000'FFFF'FFFF) << 32
                  | (value & 0xFFFF'FFFF'0000'0000) >> 32;
            value = (value & 0x0000'FFFF'0000'FFFF) << 16
                  | (value & 0xFFFF'0000'FFFF'0000) >> 16;
            value = (value & 0x00FF'00FF'00FF'00FF) << 8
                  | (value & 0xFF00'FF00'FF00'FF00) >> 8;
            return value;
        }
        constexpr auto operator()(uint32_t value) const
        {
            if (!do_swap) return value;

            value = (value & 0x000'0FFFF) << 16
                  | (value & 0xFFF'F0000) >> 16;
            value = (value & 0x00F'F00FF) << 8
                  | (value & 0xFF0'0FF00) >> 8;
            return value;
        }
        constexpr auto operator()(uint16_t value) const
        {
            if (!do_swap) return value;

            value = (value & 0x00FF) << 8
                  | (value & 0xFF00) >> 8;
            return value;
        }
        constexpr auto operator()(uint8_t value) const
        {
            return value;
        }

        // forward signed types thru unsigned converter
        template <typename T>
        constexpr auto operator()(T value) const
            { return (*this)(std::make_unsigned_t<T>(value)); }

        const bool do_swap;
    };
}

struct elf_convert
{
    // ostream::write requires `char`
    using pad_t = char;
    static constexpr auto   MAX_PADDING = sizeof(detail::largest_hdr);
    static constexpr pad_t  zero[MAX_PADDING] = {};

    // pick large alignment
    static constexpr auto   MAX_ALIGN   = 6;
    static_assert(MAX_PADDING >= (1 << MAX_ALIGN));
        
    elf_convert(
              uint8_t ei_class                  // DST class
            , uint8_t ei_data                   // DST endian
            , uint8_t src_data = HOST_ENDIAN()  // SRC endian
            )
        : ei_class(ei_class)
        , swap_dst{ei_data, src_data}
        , swap_src{src_data, HOST_ENDIAN()}
        {}


    // swap endian (implied src->dst)
    template <typename T>
    constexpr auto swap(T value) const
    {
        return swap_dst(value);
    }

    // return size of 32/64 DST elf header given SRC header
    template <typename T>
    constexpr std::size_t header_size(T = {}) const
    {
        constexpr auto n = detail::hdr_index<T>;
        static_assert(n >= 0, "elf_convert::header_size");

        if (sizeof(T) == 1)
            return 1;

        switch (ei_class)
        {
            case ELFCLASS32:
                using DST32 = meta::at_c<detail::elf32_hdrs, n>;
                return sizeof(DST32);

            case ELFCLASS64:
                using DST64 = meta::at_c<detail::elf64_hdrs, n>;
                return sizeof(DST64);

            default:
                throw std::logic_error {"elf_convert::header_size: switch"};
        }
    }

    // return alignment of 32/64 DST elf header given SRC header
    template <typename T>
    constexpr std::size_t header_align(T = {}) const
    {
        constexpr auto n = detail::hdr_index<T>;
        static_assert(n >= 0, "elf_convert::header_align");

        if (sizeof(T) == 1)
            return 1;

        // NB: while this is the "host's" idea of alignment, binary
        // of header will not be correct if host alignment is non-standard
        switch (ei_class)
        {
            case ELFCLASS32:
                using DST32 = meta::at_c<detail::elf32_hdrs, n>;
                return alignof(DST32);

            case ELFCLASS64:
                using DST64 = meta::at_c<detail::elf64_hdrs, n>;
                return alignof(DST64);

            default:
                throw std::logic_error {"elf_convert::header_align: switch"};
        }
    }

    // utility to calculate padding
    static constexpr std::size_t padding(std::size_t alignment, std::size_t offset)
    {
        if (alignment < 2)
            return 0;

        auto mask = alignment - 1;
        auto n = mask & offset;
        return n ? (alignment - n) : 0;
    }

    // convert SRC elf header to binary in DST format
    template <typename T>
    void const *cvt(T const& src) const
    {
        constexpr auto n = detail::hdr_index<T>;
        static_assert(n >= 0, "elf_convert::cvt");
        switch (ei_class)
        {
            case ELFCLASS32:
                using DST32 = meta::at_c<detail::elf32_hdrs, n>;
                return hdr_cvt(DST32{}, src);

            case ELFCLASS64:
                using DST64 = meta::at_c<detail::elf64_hdrs, n>;
                return hdr_cvt(DST64{}, src);

            default:
                throw std::logic_error {"elf_convert::cvt: switch"};
        }
    }

private:
    // routines to convert ELF header members
    // convert endian src->host
    
    // convert hdr source field to (swapped) header dst field
    template <typename DST, typename SRC>
    constexpr void assign(DST& dst_member, SRC const& src_member) const
    {
        // get SRC into host format
        auto data = swap_src(src_member);

        // cast to DST type & swap
        dst_member = swap_dst(static_cast<DST>(data));
    }

    // overloads to help resolve `cvt`
    // NB: function overloadsi favor one with most template parameters
    // Generic
    template <typename T, typename = void, typename = void>
    void const *hdr_cvt(T, T const&) const;
    template <typename DST, typename SRC>
    void const *hdr_cvt(DST, SRC const&) const;

    // Per header type
    // XXX should third arg be "Elf_Ehdr" not Elf32_Ehdr" ???
    template <typename DST, typename SRC>
    void const *hdr_cvt(DST&, SRC const&, Elf_Ehdr) const;
    template <typename DST, typename SRC>
    void const *hdr_cvt(DST&, SRC const&, Elf_Shdr) const;
    template <typename DST, typename SRC>
    void const *hdr_cvt(DST&, SRC const&, Elf_Sym) const;
    template <typename DST, typename SRC>
    void const *hdr_cvt(DST&, SRC const&, Elf_Phdr) const;

    // Relocs are not as regular
    // This set converts endian
    template <typename T, typename = void, typename = void>
    void const *hdr_cvt(T&, T const&, Elf_Rel) const;
    template <typename T, typename = void, typename = void>
    void const *hdr_cvt(T&, T const&, Elf_Rela) const;

    // convert from Elf64 reloc
    template <typename DST, typename = void>
    void const *hdr_cvt(DST&, Elf64_Rel const& , Elf_Rel) const;
    template <typename DST, typename = void>
    void const *hdr_cvt(DST&, Elf64_Rela const&, Elf_Rela) const;

    // convert from ELF32 reloc
    template <typename DST, typename = void>
    void const *hdr_cvt(DST&, Elf32_Rel const& , Elf_Rel) const;
    template <typename DST, typename = void>
    void const *hdr_cvt(DST&, Elf32_Rela const&, Elf_Rela) const;

    const uint8_t ei_class;                 // target ei_class
    const detail::swap_endian swap_dst;     // swap host->dst
    const detail::swap_endian swap_src;     // swap src->host
};

template <typename T, typename, typename>
void const *elf_convert::hdr_cvt(T, T const& src) const
{
    // if native endian & dst uses the proxy T, no conversion needed
    // NB: important case: native assembler on Elf64 machine...
    if (!swap_dst.do_swap)
        return &src;
    return hdr_cvt<T, T>(T{}, src);
}

template <typename DST, typename SRC>
void const *elf_convert::hdr_cvt(DST, SRC const& src) const
{
    static DST dst;
    return hdr_cvt(dst, src, typename DST::elf_header_t{});
}

// make dst member `cast`ed & `swap`ed from src member.
#define DO_CVT(member)  assign(d.member, s.member)

// Elf Ehdr
template <typename DST, typename SRC>//, typename HDR_T>
void const *elf_convert::hdr_cvt(DST& d, SRC const& s, Elf_Ehdr) const
{
    std::memcpy(d.e_ident, s.e_ident, EI_NIDENT);
    DO_CVT(e_type);
    DO_CVT(e_machine);
    DO_CVT(e_version);
    DO_CVT(e_entry);
    DO_CVT(e_phoff);
    DO_CVT(e_shoff);
    DO_CVT(e_flags);
    DO_CVT(e_ehsize);
    DO_CVT(e_phentsize);
    DO_CVT(e_phnum);
    DO_CVT(e_shentsize);
    DO_CVT(e_shnum);
    DO_CVT(e_shstrndx);
    return &d;
}

// Elf Shdr
template <typename DST, typename SRC>
void const *elf_convert::hdr_cvt(DST& d, SRC const& s, Elf_Shdr) const
{
    DO_CVT(sh_name);
    DO_CVT(sh_type);
    DO_CVT(sh_flags);
    DO_CVT(sh_addr);
    DO_CVT(sh_offset);
    DO_CVT(sh_size);
    DO_CVT(sh_link);
    DO_CVT(sh_info);
    DO_CVT(sh_addralign);
    DO_CVT(sh_entsize);
    return &d;
}

// Elf Sym
template <typename DST, typename SRC>
void const *elf_convert::hdr_cvt(DST& d, SRC const& s, Elf_Sym) const
{
    DO_CVT(st_name);
    DO_CVT(st_value);
    DO_CVT(st_size);
    DO_CVT(st_info);
    DO_CVT(st_other);
    DO_CVT(st_shndx);
    return &d;
}

// Elf Phdr
template <typename DST, typename SRC>
void const *elf_convert::hdr_cvt(DST& d, SRC const& s, Elf_Phdr) const
{
    DO_CVT(p_type);
    DO_CVT(p_offset);
    DO_CVT(p_vaddr);
    DO_CVT(p_paddr);
    DO_CVT(p_filesz);
    DO_CVT(p_memsz);
    DO_CVT(p_flags);
    DO_CVT(p_align);
    return &d;
}

// Relocation conversions are not as straightforward.
// `r_info` is composite of `sym` & `type`.
// `r_info` format is different for ELF32 & ELF64

// Relocation conversion: SRC & DST are same ei_class
template <typename T, typename, typename>
void const *elf_convert::hdr_cvt(T& d, T const& s, Elf_Rel) const
{
    DO_CVT(r_offset);
    DO_CVT(r_info);
    return &d;
}
template <typename T, typename, typename>
void const *elf_convert::hdr_cvt(T& d, T const& s, Elf_Rela) const
{
    DO_CVT(r_offset);
    DO_CVT(r_info);
    DO_CVT(r_addend);
    return &d;
}

// Relocation conversion: DST = ELF32, SRC = ELF64
// NB: make a `template` for single definition purposes
// NB: second template argument makes it a worse match

template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf64_Rel const& s, Elf_Rel) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF64_R_SYM(host_info);
    auto type = ELF64_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF32_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    DO_CVT(r_offset);
    return &d;
}
template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf64_Rela const& s, Elf_Rela) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF64_R_SYM(host_info);
    auto type = ELF64_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF32_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    DO_CVT(r_offset);
    DO_CVT(r_addend);
    return &d;
}

// Relocation conversion: DST = ELF64, SRC = ELF32
// NB: make a `template` for single definition purposes
template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf32_Rel const& s, Elf_Rel) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF32_R_SYM(host_info);
    auto type = ELF32_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF64_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    DO_CVT(r_offset);
    return &d;
}
template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf32_Rela const& s, Elf_Rela) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF32_R_SYM(host_info);
    auto type = ELF32_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF64_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    DO_CVT(r_offset);
    DO_CVT(r_addend);
    return &d;
}


// remove macro
#undef DO_CVT

}

#endif
