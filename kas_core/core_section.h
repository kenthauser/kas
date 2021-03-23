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
 * subsection code is appended in subsection order to make up the section.
 *
 * `core_section` uses `kbfd_section_defns` to help normalize section
 * names and metadata.
 *
 * To support this abstraction, the assembler defines three concepts:
 *
 * core_section: the named grouping of code, divided into subsections.
 * core_segment: a subsection into which code is assembled.
 * core_fragment: consecutive instructions in the same segment.
 *
 * The `core_section` holds the name and meta-data about section.
 * The meta-data includes all values needed to properly construct
 * the `kbfd` section. It does not hold info about the contents of the section.
 *
 * The `core_fragment` holds the object code as generated. This object code
 * can be constant or relocatable values. Consecutive instructions are
 * located sequentually in a fragment, until a new fragment is created.
 * New fragments are created when assembler instruction required new fragment
 * (eg .align) or the fragment "fills".
 *
 * The `core_segment` manages the `core_fragment` as a linked list.
 * One `core_segment` is created for each `section` (and subsection). 
 * The actual assembly of code uses `core_segments`s not `core_section`s.
 * 
 *
 */


#include "kas_object.h"
#include "opcode.h"
#include "core_fragment.h"
#include "utility/print_type_name.h"

#include "kbfd/kbfd_section_defns.h"        // XXX kbfd forward

#include <map>

namespace kas::core 
{

struct core_section : kas_object<core_section>
{
    core_section(
            std::string const& sh_name
          , kbfd::kbfd_word    sh_type  = {}
          , kbfd::kbfd_word    sh_flags = {}
        ) :
            sh_name(sh_name)
          , sh_type(sh_type)
          , sh_flags(sh_flags)
    {
        set_align();        // XXX 
    }

    // initialize sections according to selected `kbfd` target format
    static void init(kbfd::kbfd_object const& obj);

    // find special sections (with kbfd help)
    static core_section& get_initial();     // where assembler begins
    static core_section& get_lcomm();       // section for local commons

    // lookup if "name/subsection" is well-known
    static auto lookup(const char *name, unsigned sub_section = {})
    {
        return defn_p->get_section_defn(name, sub_section);
    }

public:
    using base_t::get;
    using base_t::for_each;

    // delete `add`: interface, use `get`
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

    // by default: translation from section -> segment is via subsection zero
    operator core_segment& () { return segment(); }

    // core_segment hold actual data for section
    core_segment& segment(unsigned subsection = {}) const
    {
        auto& seg_p = segments[subsection];
        if (!seg_p)
            seg_p = &core_segment::add(*this, subsection);
        return *seg_p;
    }
    
    // setters for optional values
    void set_entsize(unsigned size)
    {
        sh_entsize = size;
    }
    void set_align() 
    {
        kas_align = 4;  // XXX target alignment
    }
    void set_group(core_segment const& seg)
    {
        kas_group_p = &seg;
    }
    void set_linkage(unsigned linkage)
    {
        kas_linkage = linkage;
    }
    
    // getters
    auto& name()    const { return sh_name;       }
    auto ent_size() const { return sh_entsize; }
    auto align()    const { return kas_align;     }
    auto group_p()  const { return kas_group_p;   }
    auto linkage()  const { return kas_linkage;   }

    // allow iteration over sub-sections
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
    friend std::ostream& operator<<(std::ostream& os, core_section const& obj)
    {
        obj.print(os); return os;
    }
    
    // map `name` to `core_section`
    static inline std::map<std::string, core_section *> sections;

    // allow multiple "subsections" -- store in ordered map
    mutable std::map<unsigned, core_segment *> segments;

    std::string     sh_name;
    kbfd::kbfd_word sh_type     {};
    kbfd::kbfd_word sh_flags    {};
    kbfd::kbfd_word sh_entsize  {};
    kbfd::kbfd_word kas_linkage {};
    kbfd::kbfd_word kas_align   {};
    core_segment const *kas_group_p {};

    // backend call-back hook to map `kas_section` to `kbfd_section`
    mutable void *_kbfd_callback {};

    // kbfd support for section defns
    static inline kbfd::kbfd_target_sections const *defn_p {};

    // support test fixture: clear statics
    static void clear()
    {
        defn_p = {};
    }
};

}


#endif
