#ifndef KAS_DWARF_DWARF_LINE_DATA_H
#define KAS_DWARF_DWARF_LINE_DATA_H

#include "kas_core/kas_map_object.h"
#include "meta/meta.hpp"


namespace kas::dwarf
{
//////////////////////////////////////////////////////////////////////////////
//
// declare "dwarf_line state" variables as
//   - types
//   - enums
//   - strings
//
// use "repeated include" technique

using dl_state_types = meta::list<
#define DWARF_LINE_STATE(name) struct name,
#include "dwarf_defns.inc"
    struct dummy_dl_state       // handle trailing ','
>;

enum dl_states {
#define DWARF_LINE_STATE(name) DL_ ## name,
#include "dwarf_defns.inc"
    NUM_DWARF_LINE_STATES
};

// declare name array as type
struct dl_state_names
{
    using type = dl_state_names;
    static constexpr std::array<const char *, NUM_DWARF_LINE_STATES> value = {
#define DWARF_LINE_STATE(name) #name,
#include "dwarf_defns.inc"
        };
};


// setup some passable default values
#if 1
static constexpr auto K_LNS_OPCODE_BASE     = 13;
static constexpr auto K_LNS_LINE_BASE       = -5;
static constexpr auto K_LNS_LINE_RANGE      = 14;
//static constexpr auto K_LNS_OP_RANGE        = 20;
#else
static constexpr auto K_LNS_OPCODE_BASE     = 13;
static constexpr auto K_LNS_LINE_BASE       = -3;
static constexpr auto K_LNS_LINE_RANGE      = 12;
static constexpr auto K_LNS_OP_RANGE        = 20;
#endif
static constexpr auto K_LNS_OP_RANGE        = (255-K_LNS_OPCODE_BASE)/K_LNS_LINE_RANGE;
static constexpr auto K_LNS_DEFAULT_IS_STMT = 1;
static constexpr auto K_LNS_MIN_INSN_LENGTH = 2;
static constexpr auto K_LNS_MAX_OPS_PER_INSN = 1;


struct DL_STATE
{
    using dl_value_t = uint32_t;
    using dl_addr_t  = uint32_t;
   
    DL_STATE()
    { 
        init();
    }
    
    void init()
    {
        // initial values specified in dwarf4, section 6.2.2
        state = {};
        state[DL_file]    = 1;
        state[DL_line]    = 1;
        state[DL_is_stmt] = K_LNS_DEFAULT_IS_STMT;
        address  = 0;
    }

private:
    // provide translation from "name" to "enum"
    struct dl_lookup_t : x3::symbols<uint8_t>
    {
        dl_lookup_t()
        {
            // addr & line can't be keyword args.
            // conveniently, these are the first two...
            auto n = 2;
            auto p = &dl_state_names::value.data()[2];
            while (n < NUM_DWARF_LINE_STATES)
                add(*p++, n++);
        }

    };
    
    static inline dl_lookup_t dl_lookup;
    
public:
    static auto lookup(const char *name)
    {
        // NB: enum 0 (address) not valid for lookup.
        // use zero as error value
        auto p = dl_lookup.find(name);
        return p ? *p : 0;
    }

    std::array<dl_value_t, NUM_DWARF_LINE_STATES> state {};
    dl_addr_t  address;
};

struct dwarf_dir : core::kas_object<dwarf_dir, std::string>
{
    using base_t::for_each;

	dwarf_dir() = default;
	dwarf_dir(std::string name) : name(name) {}
    
    static auto& get(index_t index)
    {
        auto& d = obstack();
        if (index > d.size())
            d.resize(index);
        return d[index-1];
    }

	std::string name;
};

struct dwarf_file : core::kas_object<dwarf_file, unsigned>
{
    using base_t::for_each;

	dwarf_file() = default;
	dwarf_file(std::string name, unsigned mtime = {}, unsigned size = {})
		: name(name), file_mtime(mtime), file_size(size) {}

    static auto& get(index_t index)
    {
        auto& d = obstack();
        if (index > d.size())
            d.resize(index);
        return d[index-1];
    }

	std::string name;
	unsigned    file_mtime {};
	unsigned    file_size  {};
};

}

#endif
