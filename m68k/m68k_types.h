#ifndef KAS_M68K_TYPES_H
#define KAS_M68K_TYPES_H

// Define the m68k terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "m68k_hw_defns.h"
#include "m68k_reg_types.h"

// define error_message type
#include "m68k_error_messages.h"

#include "tgt_insn.h"

namespace kas::m68k
{
// M68K is baased on 16-bit instructions & 32-bit addresses
using m68k_base_t = std::uint16_t;
using m68k_addr_t = std::uint32_t;

namespace opc
{
    // declare opcode groups (ie: include files)
    using m68k_insn_defn_groups = meta::list<
          struct OP_M68K_GEN
        , struct OP_M68K_020
        , struct OP_M68K_040
        , struct OP_M68K_060
        , struct OP_M68K_CPU32
        , struct OP_M68K_68881
        , struct OP_M68K_68551
        , struct OP_COLDFIRE
        >;

    template <typename=void> struct m68k_insn_defn_list : meta::list<> {};
#if 0
    // Declare type returned by instruction parser
    // This type holds list of all `opcodes` which have the same name.

    // NB: complete definition required for `std::reference_wrapper` defn
    struct m68k_insn_t {
        using opcode_t = struct m68k_opcode_t;
        using obstack_t = std::deque<m68k_insn_t>;

        // define maximum number of ARGS
        // coldfire emac has lots of args
        static constexpr auto MAX_ARGS = 6;
        static constexpr auto max_args = MAX_ARGS;

        // limit of number of OPCODES per instruction
        // ie variants with same "name"
        static constexpr auto MAX_OPCODES = 32;
        using insn_bitset_t = std::bitset<MAX_OPCODES>;

        // pointers to all `m68k_opcode_t` instances with same "name"
        std::vector<opcode_t const *> opcodes;

        // canonical name & insn_list is all stored in instance
        m68k_insn_t(uint16_t index, std::string name) : index(index), insn_name(name) {}

        auto name() const
        {
            return insn_name.c_str();
        }

        // methods are variadic templated to eliminate need for args to be forward declared

        template <typename...Ts>
        opcode_t const *eval(insn_bitset_t&, Ts&&...) const;

        // test if args suitable for INSN (eg: count) & processor flags
        template <typename...Ts>
        parser::tagged_msg validate_args(Ts&&...) const;

        template <typename OS>
        void print(OS& os) const
        {
            os << "[" << name() << "]";
        }

        // retrieve instance from (zero-based) index
        static inline const obstack_t *index_base;
        static auto& get(uint16_t idx) { return (*index_base)[idx]; }

    //private:
        std::string insn_name;
        uint16_t    index;          // zero-based index
        uint16_t    hw_tst{};       // error message if no opcodes
    };
#else
    using m68k_insn_t   = tgt::tgt_insn_t<struct m68k_opcode_t>;
#endif
}

using opc::m68k_insn_t;
using m68k_insn_ref = std::reference_wrapper<m68k_insn_t const>;

// declare parsed argument
struct m68k_arg_t;
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              m68k::m68k_reg        // m68k register
            , m68k::m68k_rs_ref     // m68k register-set reference
            , m68k::m68k_insn_ref   // m68k insn reference
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              m68k::parser::X_m68k_reg_parser_p
            > {};
}


#endif
