#ifndef KAS_TARGET_TGT_INSN_H
#define KAS_TARGET_TGT_INSN_H

// pulls forward declarations for all types: including 
#include "kas_core/core_terminal_types.h"

#include <cstdint>
#include <deque>
#include <vector>
#include <string>

namespace kas::tgt
{
    template <typename OPCODE_T
            , std::size_t _MAX_ARGS
            , std::size_t MAX_OPCODES = 16 
            , typename TST_T          = std::uint16_t>
    
    // NB: complete definition required for `std::reference_wrapper` defn
    struct tgt_insn_t {
        using opcode_t  = OPCODE_T;
        using obstack_t = std::deque<tgt_insn_t>;

        // define maximum number of ARGS
        static constexpr auto MAX_ARGS = _MAX_ARGS;

        // limit of number of OPCODES per instruction
        // ie variants with same "name"
        static constexpr auto max_opcodes = MAX_OPCODES;
        using insn_bitset_t = std::bitset<max_opcodes>;

        // pointers to all `opcode_t` instances with same "name"
        std::vector<opcode_t const *> opcodes;

        // canonical name & insn_list is all stored in instance
        tgt_insn_t(uint16_t index, std::string name) : index(index), insn_name(name) {}

        auto name() const
        {
            return insn_name.c_str();
        }

        // methods are variadic templated to eliminate need for args to be forward declared
        template <typename...Ts>
        core::opc::opcode const *eval(insn_bitset_t&, Ts&&...) const;

        // test if args suitable for INSN (eg: count) & processor flags
        template <typename...Ts>
        parser::tagged_msg validate_args(Ts&&...) const;

        template <typename OS>
        void print(OS& os) const
        {
            os << "[" << name() << "]";
        }

        // retrieve instance from (zero-based) index
        static auto& get(uint16_t idx) { return (*index_base)[idx]; }
        static inline const opcode_t  *list_opcode;

    //private:
        static inline const obstack_t *index_base;
        
        std::string insn_name;
        uint16_t    index;          // zero-based index
        TST_T       hw_tst{};       // error message if no opcodes
    };

}


#endif
