#ifndef KAS_CORE_SECTIONS_H
#define KAS_CORE_SECTIONS_H

/*
 * support for assembler sections
 *
 * The assembler supports putting code (and other items) in mutilple
 * sections. In the original `a.out` format there were three sections:
 * '.text', '.data', and '.bss'. These sections held, respectively,
 * read-only code, read-write data, and zero-initialized data.
 *
 * The `coff` and `elf` formats added additional user-defined sections.
 * Integral subsections (eg text1 and data2) have long been supported:
 * subsection code is grouped together before being bundled together to
 * make up the section.
 *
 * To support this abstraction, the assembler defines three concepts:
 *
 * core_section: the named grouping of code, divided into subsections.
 * core_segment: a subsection into which code is assembled.
 * core_fragment: consecutive instructions in the same segment.
 *
 */


#include "kas_object.h"
#include "opcode.h"
#include "core_fragment.h"
#include "utility/print_type_name.h"

#include "elf/elf_common.h"
#include "elf/elf_external.h"

#include <map>
#include <deque>

namespace kas::core 
{

struct core_section : kas_object<core_section>
{
    // not sure where this table should go
    static const auto is_reserved(std::string const& name)
    {
        struct elf_section
        {
            const char      *sh_name;
            elf::Elf32_Word  sh_type;
            elf::Elf32_Word  sh_flags;
        };

        // NB: not all specal sections listed. Only those for assembled data
        // source: System V Application Binary Interface - DRAFT - 24 April 2001
        static constexpr elf_section const elf_sections[] =
        {
              {".text",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR }
            , {".data",     SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".bss",      SHT_NOBITS,     SHF_ALLOC+SHF_WRITE  }
            , {".comment",  SHT_PROGBITS }
            , {".data1",    SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".debug",    SHT_PROGBITS }
            , {".fini",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".fini_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".init",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".init_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".line",     SHT_PROGBITS }
            , {".note",     SHT_NOTE }
            , {".preinit_array", SHT_PREINIT_ARRAY, SHF_ALLOC+SHF_WRITE  }
            , {".rodata",   SHT_PROGBITS,   SHF_ALLOC }
            , {".rodata1",  SHT_PROGBITS,   SHF_ALLOC }
            , {".tbss",     SHT_NOBITS,     SHF_ALLOC+SHF_WRITE+SHF_TLS  }
            , {".tdata",    SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE+SHF_TLS  }
            , {".tdata1",   SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE+SHF_TLS  }
        };

        for (auto& entry : elf_sections)
            if (name == entry.sh_name)
                return &entry;
        return static_cast<elf_section const*>(nullptr);
    }
   
    core_section(
            std::string const& sh_name
          , elf::Elf32_Word    sh_type     = {}
          , elf::Elf32_Word    sh_flags    = {}
          , elf::Elf32_Word    sh_entsize  = {}
          , std::string        kas_group   = {}
          , elf::Elf32_Word    kas_linkage = {}
        ) :
            sh_name(sh_name)
          , sh_type(sh_type)
          , sh_flags(sh_flags)
          , sh_entsize(sh_entsize)
          , kas_group(kas_group)
          , kas_linkage(kas_linkage)
    {
        if (auto p = is_reserved(sh_name)){
            this->sh_type   = p->sh_type;
            this->sh_flags  = p->sh_flags;
            if (p->sh_flags & SHF_ALLOC)
                set_align();
        }
    }

public:
    using base_t::get;
    using base_t::for_each;

    // delete `add`: interface uses `get`
    template <typename...Ts> static auto& add(Ts&&...) = delete;

    // Principle interface function: `get`
    //
    // Look up section by name. Create if needed

    template <typename...Ts>
    static auto& get(std::string const& name, Ts&&...args)
    {
        // lookup via name using std::map 
        auto& s = sections[name];
        if (!s)
           s = &base_t::add(name, std::forward<Ts>(args)...);
        return *s;
    }

    //
    // lookup subsection (`core_segment`) by number. Create if needed
    //
    auto& operator[](uint32_t n)
    {
        // find `core_segment` for numbered "subsection"
        // NB: segment indexes are non-zero
        auto& seg = segments[n];
        
        if (!seg)
            seg = &core_segment::add(*this, n);

        return *seg;
    }

    void set_align() 
    {
        kas_align = 4;  // XXX target alignment
    }

    auto align() const
    {
        return kas_align;
    }

    auto& name() const
    {
        return sh_name;
    }
#if 1
    // allow iteration over sub-sections
    // XXX used in relax...
    auto begin() const { return segments.begin(); }
    auto end()   const { return segments.end();   }
#endif
    auto size() const
    {
        std::size_t size{};
        for (auto& seg : segments)
            size += seg.second->size()();
        return size;
    }

    template <typename OS>
    static void dump(OS& os)
    {
        auto print_section = [&](auto const& s)
        {
            // print section # in decimal, everything else in hex.
            os << std::dec;
            os <<         std::right << std::setw(4)  << s.index() - 1;
            os << std::hex;
            os << ": " << std::left  << std::setw(20) << s.sh_name;
            os << ": " << std::setw(2) << std::right  << s.sh_type;
            os << " "  << std::setw(8) << std::right  << s.sh_flags;
            os << " "  << std::setw(2) << std::right  << s.sh_entsize;
            os << " "  << std::setw(2) << std::right  << s.kas_linkage;
            os << " "  << std::setw(20) << std::left  << s.kas_group;
            os << std::endl;
        };

        os << "sections:" << std::endl;
        for_each(print_section);
        os << std::endl;
    }

    template <typename OS> void print(OS&) const;

    // support `set_section` in backend. manage single mutable, opaque value
    void  set_elf_callback(void *p) const { _elf_callback = p;    }
    void *elf_callback()            const { return _elf_callback; } 

public:
    static void clear()
    {
        //std::cout << "sections: clear" << std::endl;
        sections.clear();
    }
//private:
#if 0
    template <typename OS>//, typename = std::enable_if_t<std::is_base_of_v<std::ios_base, OS>>>
    friend OS& operator<<(OS& os, core_section const& obj)
    {
        obj.print(os); return os;
    }
#endif
    friend std::ostream& operator<<(std::ostream& os, core_section const& obj)
    {
        obj.print(os); return os;
    }
    
    // map `name` to `core_section`
    static inline std::map<std::string, core_section *> sections;

    // map `subsection` to `core_segment *`
    std::map<uint32_t, core_segment *> segments;

    std::string     sh_name;
    elf::Elf32_Word sh_type     {};
    elf::Elf32_Word sh_flags    {};
    elf::Elf32_Word sh_entsize  {};
    std::string     kas_group   {};
    elf::Elf32_Word kas_linkage {};
    elf::Elf32_Word kas_align   {};     // only "well-know sections"

    // backend hook
    mutable void *_elf_callback {};

    static inline core::kas_clear _c{base_t::obj_clear};
};

namespace opc
{

    struct opc_section : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "SEG"; }

        static inline core_section::index_t current, previous;

        opc_section() = default;

        void operator()(data_t& data, core_section::index_t index) const
        {
            data.fixed = index;
        }
        
        void operator()(data_t& data, core_segment const& seg) const
        {
            (*this)(data, seg.index());
        }

        void proc_args(data_t& data, core_section::index_t index)
        {
            previous = current;
            current  = index;
            data.fixed = index;
        }

#if 0
        // create an ELF section
        // NB: args other than `name` are ignored except when
        // section is first created
        // NB: see `core_section` ctor for arg list
        template <typename...Ts>
        void proc_args(Inserter& di, std::string sh_name, Ts&&...args)
        {
            auto& section = core::core_section::get(sh_name, std::forward<Ts>(args)...);
            this->fixed_p->fixed = section[0].index();
        }
#endif
        void fmt(data_t const& data, std::ostream& os) const override
        {
            auto index = data.fixed.fixed;
            os << index << ' ';
            os << core_segment::get(index);
        }

        void emit(data_t const& data, emit_base& base, core_expr_dot const *dot_p) const override
        {
            auto& seg = core_segment::get(data.fixed.fixed); 
            base.set_segment(seg);
        }
    };
}
}


#endif
