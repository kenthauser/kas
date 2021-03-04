#ifndef KAS_CORE_CORE_SECTION_H
#define KAS_CORE_CORE_SECTION_H

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

#include "kbfd/kbfd_section_defns.h"        // XXX kbfd forward

#include <map>
#include <deque>

namespace kas::core 
{

struct core_section : kas_object<core_section>
{
    // not sure where this table should go
    static const auto is_reserved(std::string const& name)
    {
        struct kbfd_section
        {
            const char      *sh_name;
            kbfd::kbfd_word  sh_type;
            kbfd::kbfd_word  sh_flags;
        };

        // NB: not all specal sections listed. Only those for assembled data
        // source: System V Application Binary Interface - DRAFT - 24 April 2001
        static constexpr kbfd_section const kbfd_sections[] =
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

        for (auto& entry : kbfd_sections)
            if (name == entry.sh_name)
                return &entry;
        return static_cast<kbfd_section const*>(nullptr);
    }
   
    core_section(
            std::string const& sh_name
          , kbfd::kbfd_word    sh_type     = {}
          , kbfd::kbfd_word    sh_flags    = {}
        //  , kbfd::kbfd_word    sh_entsize  = {}
          , std::string        kas_group   = {}
          , kbfd::kbfd_word    kas_linkage = {}
        ) :
            sh_name(sh_name)
          , sh_type(sh_type)
          , sh_flags(sh_flags)
        //  , sh_entsize(sh_entsize)
          , kas_group(kas_group)
          , kas_linkage(kas_linkage)
    {
        if (auto p = is_reserved(sh_name)){
            this->sh_type   = p->sh_type;
            this->sh_flags  = p->sh_flags;
            // XXX if (p->sh_flags & SHF_ALLOC)
                set_align();    // align all
        }
    }

    // initialize sections according to selected `kbfd` target format
    static void init(kbfd::kbfd_object const& obj);

    // find special sections
    static core_segment& get_initial();     // where assembler begins
    static core_segment& get_lcomm();       // section for local commons

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

    // allow iteration over sub-sections
    // XXX used in relax...
    auto begin() const { return segments.begin(); }
    auto end()   const { return segments.end();   }

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
            //os << " "  << std::setw(2) << std::right  << s.sh_entsize;
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
    void  set_kbfd_callback(void *p) const { _kbfd_callback = p;    }
    void *kbfd_callback()            const { return _kbfd_callback; } 

public:
    static void clear()
    {
        //std::cout << "sections: clear" << std::endl;
        sections.clear();
    }
    
    friend std::ostream& operator<<(std::ostream& os, core_section const& obj)
    {
        obj.print(os); return os;
    }
    
    // map `name` to `core_section`
    static inline std::map<std::string, core_section *> sections;

    // map `subsection` to `core_segment *`
    std::map<uint32_t, core_segment *> segments;

    std::string     sh_name;
    kbfd::kbfd_word sh_type     {};
    kbfd::kbfd_word sh_flags    {};
    //kbfd::kbfd_word sh_entsize  {};
    std::string     kas_group   {};
    kbfd::kbfd_word kas_linkage {};
    kbfd::kbfd_word kas_align   {};

    // backend call-back hook to map `kas_section` to `kbfd_section`
    mutable void *_kbfd_callback {};

    // kbfd support for section defns
    static inline kbfd::kbfd_target_sections const *defn_p {};

    // support test fixture
    static inline core::kas_clear _c{base_t::obj_clear};
};

}


#endif
