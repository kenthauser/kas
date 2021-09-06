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
 * The `core_segment` manages the `core_fragment` objects as a linked list.
 * One `core_segment` is created for each `section` (and subsection). 
 * The actual assembly of code uses `core_segments`s not `core_section`s.
 * 
 */


#include "kas_object.h"
#include "opcode.h"
#include "core_fragment.h"
#include "utility/print_type_name.h"

#include "kbfd/kbfd_object.h"
#include "kbfd/kbfd_section_defns.h"

#include <map>

namespace kas::core 
{

struct core_section : kas_object<core_section>
{
    // forward declare type for deferred section data generation.
    // used in `assemble`. `core_section` knows it only as pointer
    struct deferred_ops;

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

    // convenience method to initialize well-known sections
    static core_section& get(kbfd::kbfd_section_defn const&);

    // `core_segment` holds actual data for section
    core_segment& segment(unsigned subsection = {}) const;
    
    // by default: translation from section -> segment is via subsection zero
    operator core_segment& () const { return segment(); }

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
    
    auto set_deferred_ops(deferred_ops& ops)
    {
        deferred_ops_p = &ops;
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

    std::size_t size() const;
    
    template <typename OS> static void dump(OS& os);
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

    // for deferred generation support
    deferred_ops *deferred_ops_p {};

    // backend call-back hook to map `kas_section` to `kbfd_section`
    mutable void *_kbfd_callback {};

    // kbfd support for section defns
    static inline kbfd::kbfd_target_sections const *defn_p;

    // support test fixture: clear statics
    static void clear()
    {
        defn_p = {};
    }
};


// Declare structure for deferred opertions.
// These sections hold debug and attribute information. 

struct core_section::deferred_ops
{
    virtual void end_of_parse(core_section&) {}

    // method to emit deferred data to insn container
    virtual void do_gen_data()
    {
        std::cout << "deferred_ops::do_gen_data" << std::endl;
    }
  
    // returns true if data generated.
    // normally just want to generate data once. 
    // override `do_gen_data` for normal single generation case.
    virtual bool gen_data() 
    {   
        std::cout << "deferred_ops::gen_data" << std::endl;
        
        static bool done;
        if (!done)
        {
            do_gen_data();
            return done = true;
        }
        return false;       // no data emitted
    }

    // interact with `deferred_emit` ecostructure
    template <typename INSERTER>
    void set_inserter(INSERTER& i)
    {}

    void *emit_p {};
};


}


#endif
