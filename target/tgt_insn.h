#ifndef KAS_TARGET_TGT_INSN_H
#define KAS_TARGET_TGT_INSN_H


#include <cstdint>
#include <deque>
#include <vector>
#include <string>

namespace kas::tgt
{
template <typename MCODE_T
        , typename HW_DEFS
        , unsigned MAX_MCODES = 16 
        , typename INDEX_T    = std::uint16_t
        >
struct tgt_insn_t
{
    using mcode_t   = MCODE_T;
    using hw_defs   = HW_DEFS;
    using hw_tst    = typename hw_defs::hw_tst;
    using index_t   = INDEX_T;

    using opcode    = core::opcode;
    using kas_error_t = parser::kas_error_t;

    static constexpr auto max_mcodes = MAX_MCODES;

// used by "adder"
    using obstack_t = std::deque<tgt_insn_t>;
    static inline const obstack_t *index_base;
    
    // limit of number of MACHINE CODES per instruction
    // ie variants with same "name"
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

    template <typename OS> void print(OS& os) const;
    
    // retrieve instance from (zero-based) index
    static auto& get(index_t idx) { return (*index_base)[idx]; }
    static inline const mcode_t  *list_mcode_p;
    
    // pointers to all `mcode_t` instances with same "name"
    std::vector<mcode_t const *> mcodes;

    std::string name;
    index_t     index;          // zero-based index
    hw_tst      tst{};          // error message if no mcodes
};

}


#endif
