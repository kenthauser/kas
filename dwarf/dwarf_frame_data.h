#ifndef KAS_DWARF_DWARF_FRAME_DATA_H
#define KAS_DWARF_DWARF_FRAME_DATA_H

#include "dwarf_emit.h"
#include "kas_core/kas_map_object.h"
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

using df_cmd_types = meta::list<
#define DWARF_CALL_FRAME(name, ...) struct df_ ## name ## _t,
#include "dwarf_defns.inc"
    struct dummy_df_cmd       // handle trailing ','
>;

static constexpr auto NUM_DWARF_CALL_FRAME_CMDS = meta::size<df_cmd_types>::value - 1;

enum df_cmds {
#define DWARF_CALL_FRAME(name, high, low, ...)  \
            DF_ ## name = (high << 6) + low,
#include "dwarf_defns.inc"
};

// declare name array as type
struct df_cmd_names
{
    using type = df_cmd_names;
    static constexpr std::array<const char *, NUM_DWARF_CALL_FRAME_CMDS> value = {
#define DWARF_CALL_FRAME(name, ...) #name,
#include "dwarf_defns.inc"
        };
};


struct df_insn_data : core::kas_object<df_insn_data>
{
    df_insn_data(uint32_t cmd, uint32_t arg1 = {}, uint32_t arg2 = {})
        : cmd(cmd), arg1(arg1), arg2(arg2) {}

    void set_addr(core::addr_ref ref)
    {
        addr = ref;
    }

    static auto size()
    {
        return obstack().size();
    }

    static auto get_iter(uint32_t n)
    {
        auto iter = obstack().begin();
        std::advance(iter, n);
        return iter;
    }


//private:
    core::addr_ref addr;
    uint32_t cmd;
    uint32_t arg1;
    uint32_t arg2;
    static inline core::kas_clear _c{base_t::obj_clear};
};


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


}

#endif
