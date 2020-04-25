#ifndef KAS_ELF_ELF_CONVERT_H
#define KAS_ELF_ELF_CONVERT_H

// Convert ELF Class (ELF32/ELF64) and data ENDIAN between host and target formats
//
// When headers are in memory, they are stored as ELF64 with host-endian.
// Data in sections is always stored in target format (class/endian)
// `elf_convert` supports both host->target & target->host translations
// of headers & byte-swap for endian conversion of data emited to data sections.

//
// Notes on ELF_CONVERT:
//
// could be member of `elf_format<>`. Can be member of elf_object
//
// Best if `elf_convert` is not a templated type. 
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



#include "elf_external.h"
#include "elf_common.h"
#include "elf_endian.h"         // byte swapping
#include "elf_format.h"

#include <type_traits>

namespace kas::elf
{
namespace detail
{
#if 0
    // helper type to do work of conversion
    struct do_cvt
    {
        constexpr do_cvt(elf_convert const& cvt)
            : cvt(cvt) {}

        // perform actual conversion 
        template <typename TGT_HDRS, typename DST, typename SRC>
        void operator()(TGT_HDRS, DST& dst, SRC const& src) const;

        template <typename T>
        constexpr auto swap_src(T member) const
        {
            if constexpr (SWAP_ENDIAN::passthru)
                return member;
            if (src_is_host)
                return member;
            return cvt.swap(member);
        }

        template <typename T>
        constexpr auto swap_dst(T member) const
        {
            if constexpr (SWAP_ENDIAN::passthru)
                return member;
            if (src_is_host)
                return cvt.swap(member);
            return member;
        }

        // convert hdr source field to (swapped) header dst field
        template <typename DST, typename SRC>
        constexpr void assign(DST& dst_member, SRC const& src_member) const
        {
            if constexpr (SWAP_ENDIAN::passthru)
            {
                dst_member = src_member;
            }
            else
            {
                // get SRC into host format
                auto data = swap_src(src_member);

                // cast to DST type & swap
                dst_member = swap_dst(static_cast<DST>(data));
            }
        }

        elf_convert const& cvt;
        bool               src_is_host;
    };
#endif
}


// external interface -- non-templated
struct elf_convert
{
    struct cvt_fns 
    {
        // pick up values from `elf_format.h`
        using host_hdrs = detail::host_hdrs;
        using hdrs_size = meta::size<host_hdrs>;
       
        // convert src to dest
        using src_fn = std::pair<void const *, std::size_t>(*)(elf_convert const&, void const *);

        template <typename TGT_HDRS>
        cvt_fns(TGT_HDRS)
        {
            src[0] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Ehdr>, Elf64_Ehdr>;
            src[1] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Shdr>, Elf64_Shdr>;
            src[2] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Sym >, Elf64_Sym >;
            src[3] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Rel >, Elf64_Rel >;
            src[4] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Rela>, Elf64_Rela>;
            src[5] = elf_convert::cvt_src<TGT4HOST<TGT_HDRS, Elf64_Phdr>, Elf64_Phdr>;
        }
        template <typename HOST_HDR>
        auto get_src() const
        {
            return src[meta::find_index<host_hdrs, HOST_HDR>::value];
        }

    private:
        src_fn src[hdrs_size::value];
    };


    template <typename ELF_FORMAT, typename TGT_HDRS = typename ELF_FORMAT::headers>
    elf_convert(swap_endian const& swap, ELF_FORMAT)
            : swap(swap)
            , fns(TGT_HDRS{})
    {
    }
  
    cvt_fns fns;

    template <typename HOST_HDR>
    std::pair<void const *, std::size_t> operator()(HOST_HDR const& host) const
    {
        return fns.get_src<HOST_HDR>()(*this, &host);
    }

    template <typename HOST_HDR>
    std::size_t tgt_size(HOST_HDR const& host = {})
    {
        return (*this)(host).second;
    }
//
// Support for `padding`. Create a `zero` sized to largest header
//
    // ostream::write requires `char`, not `unsigned char`
    static constexpr auto   MAX_PADDING = 64;       // large header
    static constexpr char   zero[MAX_PADDING] = {};

    // pick large alignment
    static constexpr auto   MAX_ALIGN   = 6;        // 64 bytes
    static_assert(MAX_PADDING >= (1 << MAX_ALIGN));
       
// convert operations
//
// used to convert SRC value to DST for writing. 
// SRC is in host format, implied DST may require `swap`
//
// used when writing target data

    template <typename TGT, typename SRC>
    static std::pair<void const *, std::size_t>
    cvt_src(elf_convert const& cvt, void const *p) 
    {
        auto& src = *static_cast<SRC const *>(p);
        if constexpr (std::is_same_v<TGT, SRC> && cvt.swap.passthru)
            return {&src, sizeof(src)};
        else
        {
            static TGT tgt; 
            cvt.do_cvt<SRC>(tgt, src, true);
            return {&tgt, sizeof(tgt)};
        }
    }

#if 0
//
// used to convert DST to host format
// SRC is in TGT format, and may require swap
//
// 

    template <typename DST, typename SRC>
    void cvt(DST& dst, SRC const& src, bool src_is_host = false) const
    {
        if constexpr (std::is_same_v<DST, SRC> && swap.passthru)
        {
            // no format change -- short circut with copy
            memcpy(&dst, &src, sizeof(dst));
        }
        
        else
        {
            // need to swap before evaluating or before writing
            do_cvt(*this, src_is_host)(dst, src);
        }
    }
   
#endif

    // trampoline method. You can't partially specialize
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
    

    template <typename T>
    constexpr T swap_src(T member, bool src_is_host) const
    {
        if (swap.passthru)
            return member;
        if (src_is_host)
            return member;
        return swap(member);
    }

    template <typename T>
    constexpr T swap_dst(T member, bool src_is_host) const
    {
        if (swap.passthru)
            return member;
        if (src_is_host)
            return swap(member);
        return member;
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

    swap_endian const& swap;
};
#if 0
// simplify declaration of actual conversion methods
#if 1
template <typename SWAP_ENDIAN, typename TGT_HDRS>
using DO_CVT = elf_convert<SWAP_ENDIAN, TGT_HDRS>;
#else
#define DO_CVT void template<typename S, typename T> \
                    void elf_convert<S,T>::do_cvt::operator()
#endif
#endif
}

#endif
