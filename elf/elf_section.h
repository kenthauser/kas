#ifndef KAS_ELF_ELF_SECTION_H
#define KAS_ELF_ELF_SECTION_H


#include "elf_external.h"
#include "elf_common.h"
#include "elf_convert.h"

#include "kas_core/core_section.h"
#include "kas_core/core_symbol.h"


namespace kas::elf
{

struct elf_section : core::kas_object<elf_section>
{
    elf_section(Elf64_Word sh_type
                , std::string const& name
                , Elf64_Word ent_size = {}
                , unsigned alignment = {})
            : name(name)
        {
            s_header.sh_type      = sh_type;
            s_header.sh_entsize   = ent_size;

            if (!alignment) {
                // calculate default alignment. limit according to ELF type
                // allow alignment only if ent_size is power of 2
                
                if (ent_size & (ent_size - 1))
                    alignment = 1;
                else if (ent_size)
                    alignment = ent_size;
                else
                    alignment = 1;

                auto MAX_ALIGN = cvt_p->header_align<decltype(s_header)>();
                if (alignment > MAX_ALIGN)
                    alignment = MAX_ALIGN;
            }

            s_header.sh_addralign = alignment;
        }
    
    
    // expose protected methods
    using base_t::index;
    using base_t::for_each;

    // put binary data to buffer
    auto put(void const *begin, std::size_t n)
    {
        auto old = data.size();
        auto s = static_cast<char const *>(begin);
        data.insert(data.end(), s, s + n);
        return old;
    }

    std::size_t size() const
    {
        // XXX
        return s_header.sh_size ? s_header.sh_size : data.size();
        return data.size();
    }

    // XXX need custom allocator for `data` which doesn't initialize memory
    // NB: data is not `unsigned` because ostream::write complains
    std::vector<char> data;
    
    std::string name;
    Elf64_Shdr  s_header {};
    uint16_t    padding  {};        // number of padding chars required

    // XXX move static to `section_base` as they are assembler only.
    // `section index` of symbol table needed for some linked sections
    inline static index_t symtab_index;
    inline static elf_convert const *cvt_p;
    static inline core::kas_clear _c{base_t::obj_clear};
};

struct elf_section_base
{
    // NB: member variables are initialized in order declared. Need `section` to be first
    // as it is used in ctor to iniialize other member variables
   elf_section& section;

    auto& cvt() const { return *elf_section::cvt_p; }

    // ctor: pass args to `elf_section` & establish convenience references
    elf_section_base(Elf64_Word sh_type
                    , std::string const& name
                    , std::size_t ent_size = {}
                    , std::size_t ent_align = {})
        : section(elf_section::add(sh_type, name, ent_size, ent_align))
        , data(section.data)
        , s_header(section.s_header)
        {}

    // convenience ctor: allow 3rd parameter to be `entry` type
    template <typename T = unsigned char, typename = std::enable_if_t<!std::is_integral<T>::value>>
    elf_section_base(Elf64_Word sh_type, std::string const& name, T)
        : elf_section_base(sh_type, name, cvt().header_size<T>(), cvt().header_align<T>())
        {}
    
    auto put(void const *begin, std::size_t n)
    {
        return section.put(begin, n);
    }

    // declare refs to save some typing
    decltype(elf_section::data)&     data;
    decltype(elf_section::s_header)& s_header;
};


struct es_string : elf_section_base
{
    using elf_section_base::put;

    es_string(std::string name) : elf_section_base(SHT_STRTAB, name)
    {
        data.push_back('\0');
    }

    auto put(std::string const& s)
    {
        return put(s.data(), s.size() + 1);
    }

    auto put(const char *p)
    {
        return put(p, std::strlen(p) + 1);
    }
};

namespace detail
{
    template <typename Reloc, unsigned SHT>
    struct es_reloc_base : elf_section_base
    {
        using elf_section_base::put;

        es_reloc_base(std::string const& name, unsigned data_index)
            : elf_section_base(SHT, name, Reloc{})
        {
            s_header.sh_link   = section.symtab_index;
            s_header.sh_info   = data_index;
            s_header.sh_flags |= SHF_INFO_LINK;
        }

        void put(Reloc const& reloc)
        {
            put(cvt().cvt(reloc), s_header.sh_entsize);
        }
    };
}

// binutils defn uses `bfd_vma`. override with ELF standard defn
#undef  ELF64_R_INFO
#define ELF64_R_INFO(s,t)   (((s) << 32) + (t))

using es_reloc   = detail::es_reloc_base<Elf64_Rel,  SHT_REL>;
using es_reloc_a = detail::es_reloc_base<Elf64_Rela, SHT_RELA>;


struct es_data : elf_section_base
{
    using elf_section_base::put;

    es_data(core::core_section& s)
        : elf_section_base(s.sh_type, s.sh_name, s.sh_entsize, s.kas_align)
        , name(s.sh_name)
        {
            s_header.sh_flags   = s.sh_flags;

            // XXX group/linkage?
            // XXX st_link, st_info

            set_size(s.size());
            s.es_data_p = this;
        }

    // override base class method to write in pre-allocated memory
    void put(void const *p, std::size_t n)
    {
        // write data to buffer
        if (data_base)
            std::memcpy(data_p, p, n);

        // accumulate offset even for `SHN_NOBITS`
        data_p += n;
    }

    void advance(std::size_t n, uint64_t filler_c, std::size_t filler_w)
    {
        // write filler `n` bytes of filler data, made up of groups of
        // `filler_c` "characters" which are `filler_w` wide.

        // XXX for now, just advance write pointer.
        data_p += n;
    }

    // calculate current position in section
    // NB: also works for ST_NOBITS
    Elf64_Addr position() const
    {
        return data_p - data_base;
    }

    // put reloc (two flavors)
    void put_reloc(Elf64_Word r_type, Elf64_Word sym)
    {
        if (!data_base)    // ignore if SHN_NOBITS
            return;
        if (!reloc_p)      // allocate relocation segment if required
            reloc_p = new es_reloc(".rel" + section.name, section.index());

        // create & emit
        Elf64_Xword r_sym  = sym;
        Elf64_Xword r_info = ELF64_R_INFO(r_sym, r_type);

        Elf64_Rel reloc{ position(), r_info };
        reloc_p->put(reloc);
    }

    // NB: `addend` is a mutable reference. Clear `addend` after emiting reloc
    void put_reloc_a(Elf64_Word r_type, Elf64_Word sym, Elf64_Sxword& addend)
    {
        if (!data_base)    // ignore if SHN_NOBITS
            return;
        if (!reloc_a_p)    // allocate relocation segment if required
            reloc_a_p = new es_reloc_a(".rela" + section.name, section.index());

        // create & emit
        Elf64_Xword r_sym  = sym;
        Elf64_Xword r_info = ELF64_R_INFO(r_sym, r_type);

        Elf64_Rela reloc{ position(), r_info, addend };
        reloc_a_p->put(reloc);
        addend = 0;
    }

    // buffer size must be set before data written
    void set_size(std::size_t n)
    {
        // allocate data for section
        // NB: use std::vector to allocate memory, but not store data
        if (s_header.sh_type != SHT_NOBITS) {
            data.reserve(n);
            data_base = data_p = data.data();
        }
        s_header.sh_size = n;
    }

    // utilize `friend` status to convert core_section -> es_data
    static es_data& core2es(core::core_section const& core)
    {
        auto p = core.es_data_p;
        if (!p)
            throw std::logic_error("core2es: no elf section defined for " + core.name());
        return *p;
    }
    
    using data_ptr_t = typename decltype(elf_section::data)::pointer;

    Elf32_Word  sym_num   {};
    data_ptr_t  data_base {};
    data_ptr_t  data_p    {};
    es_reloc   *reloc_p   {};
    es_reloc_a *reloc_a_p {};
    std::string name;
};

struct es_symbol : elf_section_base
{
    using elf_section_base::put;

    // default initializer -- system symbol table
    es_symbol() : es_symbol(".symtab", ".strtab") {}

    // actual initializer
    es_symbol(std::string tab_name, std::string str_name)
        : elf_section_base(SHT_SYMTAB, tab_name, cvt().header_size<Elf_Sym>())
        , sym_string(str_name)
    {
        s_header.sh_link  = sym_string.section.index();

        // initial symbol is zero entry
        put(cvt().zero, s_header.sh_entsize);
    }

    // defined in elf_symbol_util.h
    uint32_t add(core::core_symbol&);

    // create a `ST_SECTION` symbol for a data section
    uint32_t add(es_data& d)
    {
        Elf64_Sym sym {};

        // initialize non-default fields 
        // XXX st_name is convenience for debugging
        sym.st_name  = sym_string.put(d.section.name);
        sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
        sym.st_shndx = d.section.index();
        put(cvt().cvt(sym), section.s_header.sh_entsize);

        // return symbol number && increment symbol count
        d.sym_num = ++sym_num;

        // all `ST_SECTION` symbols are local
        s_header.sh_info = sym_num + 1;
        return sym_num;;
    }

    // utility to decide if symbol should be emitted...
    bool should_emit_local(core::core_symbol& s) const;
    bool should_emit_non_local(core::core_symbol& s) const;

private:
    // symbol string table
    es_string sym_string;
    Elf64_Word sym_num {};
};


}
#endif
