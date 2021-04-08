#ifndef KBFD_KBFD_CONVERT_H
#define KBFD_KBFD_CONVERT_H

// Convert ELF Class (ELF32/ELF64) and data ENDIAN between host and target formats
//
// When headers are in memory, they are stored as ELF64 with host-endian.
// Data in sections is always stored in target format (class/endian)
// `kbfd_convert` supports both host->target & target->host translations
// of headers & byte-swap for endian conversion of data emited to data sections.

//
// Notes on ELF_CONVERT:
//
// could be member of `kbfd_format<>`. Can be member of kbfd_object
//
// Best if `kbfd_convert` is not a templated type. 
//
// Uses: convert relocations, convert symbols, convert headers
//       used for read and write.
//
//  when used in `assembler`, need to convert from Elf64 header to
//  { void const *, size_t } pair. Always used for writing. Target
//  type can always be inferred.
//
//  when used in `linker` or other utility, need to convert when
//  reading target format from specified Elf64 header. Source could
//  productively be specified as pointer incremented by bytes consumed.
//  Output will be Elf64 object. Can be passed by reference as arg.



#include "kbfd_external.h"
#include "elf_common.h"
#include "kbfd_endian.h"         // byte swapping
#include "kbfd_target_reloc.h"

#include <type_traits>

namespace kbfd
{

// external interface -- non-templated
struct kbfd_convert
{
    struct cvt_fns 
    {
        // pick up values from `kbfd_target_format.h`
        using host_hdrs = detail::host_hdrs;
        using hdrs_size = meta::size<host_hdrs>;
       
        // convert src to dest
        //using cvt_src_rt = std::pair<void const *, std::size_t>;
        using src_fn     = cvt_src_rt(*)(kbfd_convert const&, void const *);

        // instantiate all six conversions
        // XXX should use metafn to extract host header types
        template <typename TGT_HDRS>
        cvt_fns(TGT_HDRS)
        {
            src[0] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Ehdr>, Elf64_Ehdr>;
            src[1] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Shdr>, Elf64_Shdr>;
            src[2] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Sym >, Elf64_Sym >;
            src[3] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Rel >, Elf64_Rel >;
            src[4] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Rela>, Elf64_Rela>;
            src[5] = kbfd_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Phdr>, Elf64_Phdr>;
        }
        template <typename HOST_HDR>
        auto get_src() const
        {
            return src[meta::find_index<host_hdrs, HOST_HDR>::value];
        }

    private:
        src_fn src[hdrs_size::value];
    };

#if 0
    template <typename ELF_FORMAT, typename TGT_HDRS = typename ELF_FORMAT::headers>
    kbfd_convert(kbfd_format const& fmt, swap_endian const& swap, ELF_FORMAT)
            : fmt(fmt)
            , swap(swap)
            , fns(TGT_HDRS{})
    {
    }
#else
    template <typename TGT_HDRS>
    kbfd_convert(swap_endian const& swap, TGT_HDRS const& hdrs)
            : swap(swap)
            , fns(hdrs)
    {
    }

#endif
    // convert "host" type to "return-type" (pointer/size pair)
    template <typename HOST_HDR>
 /* XX   cvt_fns:: */ cvt_src_rt operator()(HOST_HDR const& host) const
    {
        return fns.get_src<HOST_HDR>()(*this, &host);
    }

    template <typename HOST_HDR>
    std::size_t tgt_size(HOST_HDR const& host = {}) const
    {
        return (*this)(host).second;
    }
//
// Support for `padding`. Create a `zero` array sized to largest header
//
    // XXX consider move zero to detail::host. 
    // ostream::write requires `char`, not `unsigned char`
    // pick large alignment
    static constexpr auto MAX_ALIGN   = 6;        // 64 bytes
    static constexpr auto MAX_PADDING = (1 << MAX_ALIGN);
    static constexpr char zero[MAX_PADDING] = {};

    constexpr unsigned padding(unsigned align, unsigned long offset) const
    {
        if (align > 1)
        {
            auto mask = align - 1;  // convert power-of-two to mask
            offset &= mask;         // get lsbs
            if (offset)
                return align - offset;
        }
        return 0;
    }

// generate operations
// used by "assemblers" to create `kas::obj` types

#if 0
    auto gen_reloc(kbfd_target_reloc const& info
                 , uint32_t  sym_num
                 , uint8_t   offset
             //  , uint64_t  data
                 ) const
    {
        Elf64_Rel reloc;
        create_reloc(reloc, info, sym_num, offset);
        return reloc;
    }
    
    auto gen_reloc_a(kbfd_target_reloc const& info
                   , uint32_t  sym_num
                   , uint8_t   offset
                   , uint64_t  data
                   ) const
    {
        Elf64_Rela reloc;
        create_reloc(reloc, info, sym_num, offset, data);
        return reloc;
    }
#endif
    template <typename HOST_REL>
    HOST_REL create_reloc(kbfd_target_reloc const& info
                        , uint32_t sym_num
                        , uint64_t position
                        , uint8_t  offset
                        , int64_t  data = 0
                        ) const;


// convert operations
//
// used to convert SRC value to DST for writing. 
// SRC is in host format, implied DST may require `swap`
//
// used when writing target data

    // XXX can SRC be void? if not, pass `SRC const&`
    template <typename TGT, typename SRC>
    static /* XXX cvt_fns:: */ cvt_src_rt cvt_src(kbfd_convert const& cvt, void const *p) 
    {
        auto& src = *static_cast<SRC const *>(p);
        if constexpr (std::is_void_v<TGT>)
            return {"not supported by target", 0};
        else if constexpr (std::is_same_v<TGT, SRC> && cvt.swap.passthru)
            return {&src, sizeof(src)};
        else
        {
            static TGT tgt; 
            cvt.do_cvt<SRC>(tgt, src, true);
            return {&tgt, sizeof(tgt)};
        }
    }

    // trampoline methods. You can't partially specialize
    // function templates, so just "name" all six conversions
    template <typename HDR, typename DST, typename SRC>
    void do_cvt(DST& d, SRC const& s, bool src_is_host) const
    {
        if constexpr(std::is_same_v<HDR, Elf64_Ehdr>)
            cvt_ehdr(d, s, src_is_host);
        if constexpr(std::is_same_v<HDR, Elf64_Shdr>)
            cvt_shdr(d, s, src_is_host);
        if constexpr(std::is_same_v<HDR, Elf64_Sym >)
            cvt_sym (d, s, src_is_host);
        if constexpr(std::is_same_v<HDR, Elf64_Rel >)
            cvt_rel (d, s, src_is_host);
        if constexpr(std::is_same_v<HDR, Elf64_Rela>)
            cvt_rela(d, s, src_is_host);
        if constexpr(std::is_same_v<HDR, Elf64_Phdr>)
            cvt_phdr(d, s, src_is_host);
    }

    template <typename DST, typename SRC>
    void cvt_ehdr(DST& d, SRC const& s, bool src_is_host) const;
    template <typename DST, typename SRC>
    void cvt_shdr(DST& d, SRC const& s, bool src_is_host) const;
    template <typename DST, typename SRC>
    void cvt_sym (DST& d, SRC const& s, bool src_is_host) const;
    template <typename DST, typename SRC>
    void cvt_rel (DST& d, SRC const& s, bool src_is_host) const;
    template <typename DST, typename SRC>
    void cvt_rela(DST& d, SRC const& s, bool src_is_host) const;
    template <typename DST, typename SRC>
    void cvt_phdr(DST& d, SRC const& s, bool src_is_host) const;
    

    // perform swap to "host" for member (ie. *no* swap if src is host)
    template <typename T>
    constexpr T swap_src(T member, bool src_is_host) const
    {
        if (swap.passthru)
            return member;
        if (src_is_host)
            return member;
        return swap(member);
    }

    // perform swap to "target" for member (ie. *do* swap if src is host)
    template <typename T>
    constexpr T swap_dst(T member, bool src_is_host) const
    {
        if (swap.passthru)
            return member;
        if (!src_is_host)
            return member;
        return swap(member);
    }

    // convert hdr source field to (swapped) header dst field
    template <typename DST, typename SRC>
    constexpr void assign(DST& dst_member, SRC const& src_member, bool src_is_host) const
    {
        if (swap.passthru)
        {
            dst_member = src_member;
        }
        else
        {
            // get SRC into host format
            auto data = swap_src(src_member, src_is_host);

            // cast to DST type & swap
            dst_member = swap_dst(static_cast<DST>(data), src_is_host);
        }
    }

    //kbfd_target_format const& fmt;
    swap_endian const& swap;
    cvt_fns fns;
};
}

#endif
