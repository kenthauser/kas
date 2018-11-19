#ifndef KAS_DWARF_DWARF_FSM_H
#define KAS_DWARF_DWARF_FSM_H

#include "kas/kas_string.h"
#include "dl_state.h"
#include "dwarf_emit.h"
#include <meta/meta.hpp>

namespace kas::dwarf
{
using namespace meta;

// declare Operations (with subclass Before & After for "effect" & "side_effect")
struct OP_before {};
struct OP_after  {};

// allow `subclass` to test What for When
template <typename When, typename What>
struct OP_defn : When { using oper = What; };

// declare operations
// primary effects
using S = OP_defn<OP_before, struct OP_SET>;
using X = OP_defn<OP_before, struct OP_INVERT>;
using A = OP_defn<OP_before, struct OP_ADVANCE>;
using U = OP_defn<OP_before, struct OP_SET>;
using P = OP_defn<OP_before, struct OP_ADD_PC>;
using F = OP_defn<OP_before, struct OP_FILE>; 

// side effects
using Z = OP_defn<OP_after,  struct OP_ZERO>;    

// shorten name
#define K           KAS_STRING

// instruction to emit row: "Yes", "Yes & Init", "No"
using Y = K("Y");
using I = K("I");
using _ = K("_");

// define dwarf line instruction type: ie Standard, Extended, or Special
using STD = K("Standard");
using EXT = K("Extended");
using SPL = K("Special");

// due to the `K_STRING` macro 16 character limit, use second macro to prepend "Set "
#define KS(msg) str_cat<KAS_STRING("Set "), KAS_STRING(msg)>

// NB: the rows are arranged in numeric order within each insn type.
// NB: the column order and count must match `dwarf_defns.inc` order
using DW_INSNS = meta::list<
//    type    name               emits  state machine values (in list order) args...
//                                      AD LI FI CO OP ST BB ES PE EB IS DI
  list<STD,  K("Copy"),              Y,  _, _, _, _, _, _, Z, _, Z, Z, _, Z >
, list<STD,  K("Advance PC"),        _,  P, _, _, _, P, _, _, _, _, _, _, _, ULEB>
, list<STD,  K("Advance Line"),      _,  _, A, _, _, _, _, _, _, _, _, _, _, SLEB>
, list<STD,  K("Set File"),          _,  _, _, U, _, _, _, _, _, _, _, _, _, ULEB>
, list<STD,  K("Set Column"),        _,  _, _, _, U, _, _, _, _, _, _, _, _, ULEB>
, list<STD,  K("Set is_stmt"),       _,  _, _, _, _, _, X, _, _, _, _, _, _ >
, list<STD,  K("Set Basic Block"),   _,  _, _, _, _, _, _, S, _, _, _, _, _ > 
, list<STD,  K("Constant Add PC"),   _,  P, _, _, _, P, _, _, _, _, _, _, _ > 
, list<STD,  K("Fixed Advance PC"),  _,  A, _, _, _, Z, _, _, _, _, _, _, _, UHALF>
, list<STD, KS("Prologue End"),      _,  _, _, _, _, _, _, _, _, S, _, _, _ > 
, list<STD, KS("Epilogue Begin"),    _,  _, _, _, _, _, _, _, _, _, S, _, _ >
, list<STD,  K("Set isa"),           _,  _, _, _, _, _, _, _, _, _, _, U, _, ULEB>
, list<EXT,  K("End Sequence"),      I,  _, _, _, _, _, _, _, S, _, _, _, _ > 
, list<EXT,  K("Set Address"),       _,  U, _, _, _, Z, _, _, _, _, _, _, _, ADDR>
, list<EXT,  K("Define File"),       _,  _, _, F, _, _, _, _, _, _, _, _, _, 
                                      // define file args: NAME  NUMBER  MTIME SIZE
                                                           NAME, ULEB,   ULEB, ULEB>
, list<EXT, KS("Discriminator"),     _,  _, _, _, _, _, _, _, _, _, _, _, U, ULEB>
, list<SPL,  K("Special"),           Y,  P, A, _, _, P, _, Z, _, Z, Z, _, Z >
>;

// undefine the local macros
#undef K
#undef KS

// defined state columns. compare against `LINE_STATES`
using DW_INSN_STATE_COLS = std::integral_constant<std::size_t, 12>;
static_assert(DW_INSN_STATE_COLS() == NUM_DWARF_LINE_STATES);

// indexes to retrieve objects from DW_INSN rows
static constexpr uint8_t DW_INSN_TYPE_INDEX = 0;
static constexpr uint8_t DW_INSN_NAME_INDEX = DW_INSN_TYPE_INDEX + 1;
static constexpr uint8_t DW_INSN_EMIT_INDEX = DW_INSN_NAME_INDEX + 1;
static constexpr uint8_t DW_INSN_OP_INDEX   = DW_INSN_EMIT_INDEX + 1;
static constexpr uint8_t DW_INSN_ARG_INDEX  = DW_INSN_OP_INDEX + DW_INSN_STATE_COLS();

template <typename T>
struct insn_is_type
{
    template <typename U>
    using invoke = std::is_same<T, at_c<U, DW_INSN_TYPE_INDEX>>;
};

// get rule from "class, 1"
// eg. RULE_get_c<STD, 2> -> Advance_PC
template <typename T, std::size_t N>
using RULE_get_c = at_c<filter<DW_INSNS, insn_is_type<T>>, N-1>;

using namespace std::literals::string_literals;
template <typename T, typename LIST = DW_INSNS>
auto insn_name(unsigned special = 0)
{
    using INSN_TYPE  = at_c<T, DW_INSN_TYPE_INDEX>;
    using INSN_NAME  = at_c<T, DW_INSN_NAME_INDEX>;
    using INSN_LIST  = filter<LIST, insn_is_type<INSN_TYPE>>;
    using INDEX      = find_index<INSN_LIST, T>;

    auto name = std::string(INSN_TYPE()) + " Opcode ";
    
    if constexpr (std::is_same<INSN_TYPE, SPL>()) 
        name += std::to_string(special) + ':';
    else
        name += std::to_string(INDEX()+1) + ": " + INSN_NAME::value;
    
    return name;
}

template <typename T>
auto insn_name(T, unsigned special = 0)
{
    return insn_name<T>(special);
}

template <typename N>
struct is_update
{
    template <typename T>
    using invoke = std::is_base_of<OP_before, at<T, N>>;
};

}
#endif

