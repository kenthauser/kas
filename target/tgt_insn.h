#ifndef KAS_TARGET_TGT_INSN_H
#define KAS_TARGET_TGT_INSN_H

///////////////////////////////////////////////////////////////////////////////////////
//
// `tgt_insn`: intermediate type between `tgt_insn` & `tgt_mcode`
//
// The `tgt_insn` type holds a list of all mcodes with the same "name".
// The parser looks up `tgt_stmt` types, and the `tgt_insn` allows corresponding
// `tgt_mcode`s to be found. 
//
// The `tgt_insn` data structures are all initialized by `tgt_mcode_adder`,
// which is invoked via `parser/sym_parser_t`.
//
// The `hw_defn` ctor is a hook to allow `sym_parser_t` to initialize the
// `hw_defs` pointer to the global instance. This is the hook that allows
// the generic `target` types to access run-time configuration constants.
//
// Each `mcode` is tested to see if it passes configuration tests before being
// added to `tgt_insn` list. If no `mcodes` pass, the first failed test 
// condition is stored in `tgt_insn::tst` for error message generation.
//
// NB: The `tgt_insn` type is the only target base type which does not use the CRTP
// pattern. The "name" -> "mcode" pattern is always the same, so no customization 
// is required for various processors
//
///////////////////////////////////////////////////////////////////////////////////////

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

    using base_name = KAS_STRING("X_M68K");
    using insn_name = kas::str_cat<base_name, KAS_STRING("_INSN")>;
    using token_t   = parser::token_defn_t<insn_name, tgt_insn_t>;

    using opcode    = core::opcode;
    using kas_error_t = parser::kas_error_t;

    static constexpr auto max_mcodes = MAX_MCODES;
   
// used by "adder"
    using obstack_t = std::deque<tgt_insn_t>;
    static inline const obstack_t *index_base;
    static inline const mcode_t   *list_mcode_p;
    static inline const hw_defs   *hw_cpu_p;  // local pointer to global
    
    // limit of number of MACHINE CODES per instruction
    // ie variants with same "name"
    using bitset_t = std::bitset<max_mcodes>;

    // PSEUDO CTOR: init static global as side effect
    explicit tgt_insn_t(hw_defs const& defs)
    {
        hw_cpu_p = &defs;
    }

    // canonical name & insn_list is all stored in instance
    tgt_insn_t(index_t index, std::string name) : index(index), name(name) {}

    // add `mcode` to list of insns
    void add_mcode(mcode_t *);

    // retrieve error message if no mcodes
    auto err() const
    {
        return (*hw_cpu_p)[tst];
    }

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
    
    // pointers to all `mcode_t` instances with same "name"
    std::vector<mcode_t const *> mcodes;

    std::string name;           // name may be "calculated" from base/sfx/etc
    index_t     index;          // zero-based index
    hw_tst      tst{};          // for error message if no mcodes
};

}


#endif
