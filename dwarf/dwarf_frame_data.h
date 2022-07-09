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
    const char *args[MAX_ARGS];
};  

using DWARF_CMDS = init_from_list<dwarf_cmd_t, df_cmd_types>;

struct dw_frame_data : core::kas_object<dw_frame_data>
{
    using df_value_t = uint32_t;
    using leb128     = expression::sleb128<df_value_t>;

    using NAME = KAS_STRING("df_data");
    using base_t::for_each;

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

        uint32_t get_end() const
        {
            return start + delta;
        }

        // true iff `set_end()` has been driven
        bool has_end() const { return delta; }

        // convenience methods to retrieve addresses
        core::core_addr_t const& begin_addr() const
        {
            return dw_frame_data::get(start).addr();
        }

        core::core_addr_t const& end_addr() const
        {
            return dw_frame_data::get(start + delta).addr();
        }

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
public:
    // `emit_frame_fde` needs to read data
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

        auto inserter = std::back_inserter(df_values());
        auto fn = [&inserter](auto n) { *inserter++ = n; };
        auto p = s.args;

        for (auto const& tok : args)
        {
            auto cmd = tok.src();
            if (!strcmp(cmd.c_str(), "TEXT"))
            {
                // generate & insert std::string
                for (auto c : tok.src())
                    fn(c);
                fn(0);      // null termination
            }
            else if (!strcmp(cmd.c_str(), "BLOCK"))
            {
                // insert arg as expression??
            }
            else
            {
                // insert arg as "SLEB"
                leb128::write(fn, *tok.get_fixed_p());
            }
            ++p;
        }
    }

    auto offset() const
    {
        return frag_p->base_addr() + frag_offset();
    }

    auto& segment() const
    {
        return frag_p->segment();
    }

    // return address of current insn
    core::core_addr_t const& addr() const
    { 
        return core::core_addr_t::add(frag_p, &frag_offset);
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
        auto fn = [&reader = df_reader()]()
        {
            return *reader++;
        };

        auto s = DWARF_CMDS::value[cmd];
        os << s.name;
        auto n = s.arg_c;
        for (auto p = s.args; n--; ++p)
        {
            os << " " << *p;
            
            if (!strcmp(*p, "TEXT"))
            {
                // retrieve null-terminated string
                std::string text{};
                while (auto c = fn())
                    text += c;
                os << " text = " << text;
            }
            else if (!strcmp(*p, "BLOCK"))
            {
                // insert arg as expression??
            }
            else
            {
                // insert arg as "SLEB"
                os << " int = " << leb128::read(fn);
            }
            ++p;
            
            if (n) os << ",";
        }

        if (frag_p)
            os << ", addr = " << addr();

    }
    
    static auto size()
    {
        return obstack().size();
    }
    
    // override `base_t` methods to reset "reader" funtion
    template <typename FN>
    static void for_each(FN fn)
    {
        df_reader(true);    // re-init reader
        base_t::for_each(fn);
        df_reader(true);    // re-init reader
    }

    template <typename OS>
    static void dump(OS& os)
    {
        for (auto& frame : frames())
        {
            os << "frame: start = " << frame.start;
            os << ", end = "        << frame.get_end();
            os << ", prologue = "   << frame.prologue << std::endl;
        }

        df_reader(true);    // re-init reader
        base_t::dump(os);
        df_reader(true);    // re-init reader
    }
    
    // clear local static for next run of test fixture
    static void clear()
    {
        frames().clear();
    }

    core::core_fragment const *frag_p {};
    core::frag_offset_t        frag_offset {};
    uint8_t    cmd    {};
    
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
