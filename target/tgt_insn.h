#ifndef KAS_TARGET_TGT_INSN_H
#define KAS_TARGET_TGT_INSN_H

// pulls forward declarations for all kas_core types
//#include "kas_core/opcode.h"

#include <cstdint>
#include <deque>
#include <vector>
#include <string>

namespace kas::tgt
{
template <typename MCODE_T
        , typename TST_T      = std::uint16_t
        , unsigned MAX_MCODES = 16 
        , typename INDEX_T    = std::uint16_t>

struct tgt_insn_t
{
    using mcode_t   = MCODE_T;
    using tst_t     = TST_T;
    using index_t   = INDEX_T;

    using opcode    = core::opcode;

    // used by "adder"
    using obstack_t = std::deque<tgt_insn_t>;
    static inline const obstack_t *index_base;
    
    // limit of number of MACHINE CODES per instruction
    // ie variants with same "name"
    static constexpr auto max_mcodes = MAX_MCODES;
    using bitset_t = std::bitset<max_mcodes>;

    // canonical name & insn_list is all stored in instance
    tgt_insn_t(index_t index, std::string name) : index(index), name(name) {}

    // add `mcode` to list of insns
    void add_mcode(mcode_t *);

    // stmt interface: NB: defer naming types
    //template <typename...Ts> core::opcode& gen_insn(Ts&&...) const;
    template <typename ARGS_T>
    core::opcode *gen_insn(core::opcode::data_t&, ARGS_T&&) const;

    // methods are variadic templated to eliminate need for args to be forward declared
    template <typename...Ts>
    mcode_t const *eval(bitset_t&, Ts&&...) const;

    // test if args suitable for INSN (eg: count) & processor flags
    template <typename...Ts>
    parser::tagged_msg validate_args(Ts&&...) const;

    // XXX move to impl
    template <typename OS>
    void print(OS& os) const
    {
        os << "[" << name << "]";
    }

    // retrieve instance from (zero-based) index
    static auto& get(index_t idx) { return (*index_base)[idx]; }
    static inline const mcode_t  *list_mcode_p;
    
    // pointers to all `mcode_t` instances with same "name"
    std::vector<mcode_t const *> mcodes;

    std::string name;
    index_t     index;          // zero-based index
    tst_t       hw_tst{};       // error message if no mcodes
};

}


#endif
