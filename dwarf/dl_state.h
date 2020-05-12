#ifndef KAS_DWARF_DL_STATE_H
#define KAS_DWARF_DL_STATE_H

#include "dwarf_line_data.h"
#include "expr/expr_leb.h"
#include "meta/meta.hpp"

//
// data structures to hold dwarf_line information
//
// create a "kas_object" instance to hold "line #" & count of additional
// attributes for each ".loc" pseudo-op instruction.
//
// create single "uint8_t" deque to hold other-than-line attributes for
// all instructions. each "attribute" is stored as a leb128 attribute number,
// followed by a leb128 value. A "count" of attributes added by each ".loc"
// is stored in each ".loc" instance.
//
// With this arrangement, it is only possible to read the dwarf_line information
// from beginning to end. Fortunately, this is the only way the information is 
// read.
//
// The `address` of each instance is updated during `emit`. The `address` is used
// when the dwarf line program is generated after the main data emitted.
//

namespace kas::dwarf
{

struct dl_data : core::kas_object<dl_data>
{
    using dl_value_t = uint32_t;
    using dl_pair = std::pair<uint8_t, dl_value_t>;
    using leb128  = expression::sleb128<dl_value_t>;
    
    using NAME = KAS_STRING("dl_data");

private:
    static auto& dl_values()
    {
        static auto values_ = new std::deque<uint8_t>;
        return *values_;
    }

    // reset "reader" before each of the "for_each" functions
    static auto& dl_reader(bool re_init = false)
    {
        static auto reader = dl_values().begin();
        if (re_init)
            reader = dl_values().begin();
        return reader;
    }

public:

    // ctor: record actions for each "dwarf_line" operation
    dl_data(dl_value_t file_num, dl_value_t line_num, dl_pair const* values, uint32_t n)
        : dl_line_num(line_num)
    {
        // Don't record "file" in instance. It changes so infrequently
        // that it is better stored as data. Code as special case.
        
        // don't create inserter if no change except line #
        if (n || file_num != this->file_num)
        {
            auto inserter = std::back_inserter(dl_values());
            auto fn = [&inserter](auto n) { *inserter++ = n; };
            if (file_num != this->file_num) 
            {
                ++cnt;
                this->file_num = file_num;
                *inserter++ = DL_file;
                leb128::write(fn, file_num);
            }

            for(; n--; ++values)
            {
                ++cnt;
                *inserter++ = values->first;
                //if (!dl_is_bool[values->first]
                    leb128::write(fn, values->second);
            }
        }
    }
    
    static auto size()
    {
        return obstack().size();
    }
    
    // apply all dl_data (except line# & address)
    // NB: assumes in middle of "for_each" & dl_reader is properly set
    using apply_fn = void(*)(uint8_t dl_enum, dl_value_t dl_value);
    template <typename FN>
    void do_apply(FN&& apply_fn) const
    {
        if (cnt) {
            auto& reader = dl_reader();
            auto  read_fn = [&reader]() { return *reader++; };
            auto  n = cnt;

            while (n--) {
                auto dl_enum  = *reader++;
                auto dl_value = leb128::read(read_fn);
                apply_fn(dl_enum, dl_value);
            }
        }
    }

    // allow address & line# to be directly retrieved
    auto  line_num() const { return dl_line_num; }
    auto& address ()       { return dl_address;  }
    auto& section ()       { return dl_section;  }

    // expose lookup function from DL_STATE...
    static constexpr auto lookup = DL_STATE::lookup;

    // special method to append "End Sequence" entry to dl_data
    // Dwarf4: 6.2.5.3: Every program must end with ES with last address...
    static void mark_end(core::core_segment const& seg)
    {
        dl_pair value{DL_end_sequence, true};
        auto& d = add(file_num, 0, &value, 1);
        d.section() = seg.index();
        d.address() = seg.size()();
    }

    // override `base_t` methods to reset "reader" funtion
    template <typename FN>
    static void for_each(FN fn)
    {
        dl_reader(true);    // re-init reader
        base_t::for_each(fn);
    }

    template <typename OS>
    static void dump(OS& os)
    {
        std::cout << "DW_STATE: dump: cout = " << obstack().size() << std::endl;
        dl_reader(true);
        base_t::dump(os);
    }

    template <typename OS>
    void dump_one(OS& os) const
    {
        os << std::right << std::setw(4) << dl_line_num;

        if (dl_section) {
            os << " " << core::core_section::get(dl_section) << "+";
            os << std::hex << dl_address;
        }
        
        auto& p = dl_reader();
        auto  n = cnt;
        while (n--)
        {
            os << " ";
            os << dl_state_names::value[*p++];
            os << "=" << std::to_string(*p++);
        }
    }

    // clear local static for next run of test fixture:b
    static void clear()
    {
        file_num = {};
    }
private:
    dl_value_t dl_line_num;
    uint32_t   dl_address {};      // XXX should be 32-bit/64-bit based on address
    uint8_t    dl_section {};      // XXX should be ref::index_t
    uint8_t    cnt        {};
    static inline dl_value_t file_num;
    
    static inline core::kas_clear _c{base_t::obj_clear};
};


}

#endif
