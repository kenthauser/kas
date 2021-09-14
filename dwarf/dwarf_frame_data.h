#ifndef KAS_DWARF_DWARF_FRAME_DATA_H
#define KAS_DWARF_DWARF_FRAME_DATA_H

#include "dwarf_emit.h"
#include "kas_core/kas_map_object.h"
#include "kas/init_from_list.h"
#include "kas/kas_string.h"
#include "meta/meta.hpp"


namespace kas::dwarf
{

//////////////////////////////////////////////////////////////////////////////
//
// declare "dwarf_frame" commands as:
//   - types
//   - enums
//   - strings
//
// use "repeated include" technique

template <typename N, int CODE, typename...Ts>
using dw_frame_defn = meta::list<N, meta::int_<CODE>, Ts...>;

using df_cmd_types = meta::drop_c<meta::list<
    void   // dropped from list
#define DWARF_CALL_FRAME(name, ...) ,dw_frame_defn<KAS_STRING(#name),__VA_ARGS__>
#include "dwarf_defns.inc"
>, 1>;      // drop `void`. Macro syntax limitation

static constexpr auto NUM_DWARF_CALL_FRAME_CMDS = meta::size<df_cmd_types>::value - 1;

enum df_cmds {
#define DWARF_CALL_FRAME(name, ...)  DF_ ## name,
#include "dwarf_defns.inc"
           NUM_DWARF_FRAME_CMDS
};

// declare name array as type
// XXX can get names from `df_cmd_types` via init-from-list
struct df_cmd_names
{
    using type = df_cmd_names;
    static constexpr std::array<const char *, NUM_DWARF_CALL_FRAME_CMDS + 1> value = {
#define DWARF_CALL_FRAME(name, ...) #name,
#include "dwarf_defns.inc"
        };
};

struct dwarf_cmd_t
{
    constexpr static int MAX_ARGS = 2;

    template <typename N, typename CODE, typename...Args>
    constexpr dwarf_cmd_t(meta::list<N, CODE, Args...>) :
          name  { N::value        }
        , code  { CODE::value     }
        , arg_c { sizeof...(Args) }
        , args  { Args::value...  }
        {}

    const char *name;
    uint8_t     code;
    uint8_t     arg_c;
    //uint8_t     args[MAX_ARGS];
    const char *args[MAX_ARGS];
};  

using DWARF_CMDS = init_from_list<dwarf_cmd_t, df_cmd_types>;

struct dw_frame_data : core::kas_object<dw_frame_data>
{
    using df_value_t = uint32_t;
    using leb128     = expression::sleb128<df_value_t>;

    using NAME = KAS_STRING("df_data");

    // hold "frame" info
    struct frame_info
    {
        frame_info(uint32_t prologue)
            : prologue(prologue), delta(0) {}

        void set_start(uint32_t start)
        {
            this->start = start;
        }

        void set_end(uint32_t end)
        {
            delta = end - start;    // to allow bits for prologue...
        }

        // true iff `set_end()` has been driven
        bool has_end() const { return delta; }

        uint32_t start {};
        uint32_t delta    : 28;
        uint32_t prologue : 4;
    };

private:
    static auto& df_values()
    {
        static auto values_ = new std::deque<uint8_t>;
        return *values_;
    }

    // reset "reader" before each of the "for_each" functions
    static auto& df_reader(bool re_init = false)
    {
        static auto reader = df_values().begin();
        if (re_init)
            reader = df_values().begin();
        return reader;
    }

public:

    dw_frame_data(uint32_t cmd, std::vector<kas_token>&& args)
        : cmd(cmd)
    {
        auto& s = DWARF_CMDS::value[cmd];
        std::cout << "dw_frame_data::ctor:" << s.name;
        std::cout << ", args: ";
        if (s.arg_c)
        {
            auto p = s.args;
            for (auto n = s.arg_c; n--; ++p)
                std::cout << *p << " ";
        } else
            std::cout << "*none*";

        std::cout << ", actual:";
        for (auto const& tok : args)
            std::cout << " " << tok;
        std::cout << std::endl;
        // Don't record "file" in instance. It changes so infrequently
        // that it is better stored as data. Code as special case.
#if 0       
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
#endif
    }

    //
    // frame support: all public && static
    //
    static auto& frames()
    {
        static auto frames_ = new std::deque<frame_info>;
        return *frames_;
    }

    // add frame
    static auto& add_frame(uint32_t prologue)
    {
        return frames().emplace_back(prologue);
    }

    // get pointer to current frame
    static frame_info *get_frame_p()
    {
        auto& f = frames();
        if (f.empty()) return {};
        return &f.back();
    }

    //
    //
    //

    void print(std::ostream& os) const
    {
        auto s = DWARF_CMDS::value[cmd];
        os << s.name;
        auto n = s.arg_c;
        for (auto p = s.args; n--; ++p)
        {
            os << " " << *p;
            if (n) os << ",";
        }
    }
    
    static auto size()
    {
        return obstack().size();
    }
#if 0  
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
#endif
    // allow address directly retrieved
    auto& address ()       { return df_address;  }
    auto& segment ()       { return df_segment;  }
#if 0
    // expose lookup function from DL_STATE...
    static constexpr auto lookup = DL_STATE::lookup;

    // special method to append "End Sequence" entry to dl_data
    // Dwarf4: 6.2.5.3: Every program must end with ES with last address...
    static void mark_end(core::core_segment const& seg)
    {
        dl_pair value{DL_end_sequence, true};
        auto& d = add(file_num, 0, &value, 1);
        d.segment() = seg.index();
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

        if (dl_segment) {
            os << " " << core::core_segment::get(dl_segment) << "+";
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
#endif
    // clear local static for next run of test fixture:b
    static void clear()
    {
    }
//private:
    uint32_t   df_address {};      // XXX should be 32-bit/64-bit based on address
    uint8_t    df_segment {};      // XXX should be ref::index_t
    uint8_t    cmd        {};
    
    static inline core::kas_clear _c{base_t::obj_clear};
    

};

#if 0
struct df_data : core::kas_object<df_data>
{
    using base_t::for_each;

    df_data(bool omit_prologue = false) 
        : omit_prologue(omit_prologue)
        {
            df_current_frame_p = this;
        }
    
    static auto size()
    {
        return obstack().size();
    }


    static auto& current_frame_p()
    {
        return df_current_frame_p;
    }

    void set_begin(core::addr_ref const& ref) 
    {
        begin_ref = ref;
        begin_cmd = df_insn_data::size();
    }

    void set_end(core::addr_ref const& ref)
    {
        end_ref = ref;
        end_cmd = df_insn_data::size();
    }

    auto& begin_addr() const
    {
        return begin_ref.get();
    }

    auto range() const
    {
        auto& bgn = begin_ref.get();
        auto& end = end_ref.get();
        // XXX diagnostic if sections don't match.
        return end.offset()() - bgn.offset()();
    }

    uint32_t begin_cmd{};
    uint32_t end_cmd{};
    

private:
    static inline df_data* df_current_frame_p;
    core::addr_ref begin_ref{};
    core::addr_ref end_ref{};

    bool omit_prologue;
    static inline core::kas_clear _c{base_t::obj_clear};
};
#endif

}

#endif
