#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "arm_formats.h"            // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode_sizes.h"        // defin ARM insn variants

//#include "arm_info_impl.h"

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::arm::opc
{
using namespace tgt::opc;
using namespace tgt::opc::traits;


// declare opcode groups (ie: include files)
using arm_insn_defn_groups = meta::list<
      struct OP_ARM_GEN
    >;

// boilerplate: incorporate opcode groups
template <typename=void> struct arm_insn_defn_list : meta::list<> {};

// EXAMPLE: define `sz` types for first arg of `defn<>` template.
//          `sz_void` defined by base. All others are per-arch
// Naming scheme: lower case allow flag, upper case require flag

// c: support for `cond` : allow condition field 
// n: support for `no-al`: don't allow `al` case (implies allow condition)
// s: support for `S`: update flags
using a7_u   = arm_sz<SZ_ARCH_ARM>;
using a7_c   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND>;
using a7_cs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND               , SZ_DEFN_S_FLAG>;
using a7_n   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL                >;
using a7_ns  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL, SZ_DEFN_S_FLAG>;

// declare "suffixes" for A7 ld/st instructions
using a7_cb  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_B_FLAG               >;
using a7_cT  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_T                >;
using a7_cHs = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_H, SZ_DEFN_S_FLAG>;
using a7_cM  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_M                >;

// map "suffix" flag `sz` types to `info` manipulation types
using arm_sz_info_map = meta::list<
      meta::pair<a7_c       , struct arm_info_a7_c>
    , meta::pair<a7_cs      , struct arm_info_a7_cs>
    , meta::pair<a7_n       , struct arm_info_a7_c>
    , meta::pair<a7_ns      , struct arm_info_a7_cs>
    >;

// forward declare "list" info_fn
struct arm_info_list;

template <std::size_t OPCODE, typename TST = void, typename INFO_FN = void, std::size_t MASK = 0>
struct OP
{
    // expose template args as member types
    using type    = OP;
    using info_fn = INFO_FN;
    using tst     = TST;

    // wrap integral vaues in `size_t` types
    using code    = std::integral_constant<std::size_t, OPCODE>;
    using mask    = std::integral_constant<std::size_t, MASK>;
};

// general definition: INFO_FLAGS type, NAME type, OP type, optional FMT & VALIDATORS
template <typename INFO_FLAGS, typename NAME, typename OP, typename FMT = void, typename...VALs>
struct defn
{
    // prep the info-fn map values
    using ZIPPED    = meta::zip<arm_sz_info_map>;
    
    // see if INFO_FLAGS is in the info-fn map
    using FLAGS_IDX = meta::find_index<meta::front<ZIPPED>, INFO_FLAGS>;

    // extract INFO_FN if INFO_FLAGS in map
    // if `OP::info_fn` specified, don't override
    // default to `info_fn_t` if not specified
    using DFLT_INFO_FN = meta::if_<std::is_void<typename OP::info_fn>
                                 , typename arm_mcode_t::info_fn_t
                                 , typename OP::info_fn
                                 >;

    // if FN not specified in `OP` and the FLAGS in in `map`, retrieve appropriate `INFO_FN`
    // NB: need to defer evaluation (prevent `at` from possibly being evaluated with `npos`)
    using INFO_FN = std::conditional_t<!std::is_void_v<typename OP::info_fn> ||
                                        std::is_same_v<FLAGS_IDX, meta::npos>
                                      , meta::id<DFLT_INFO_FN>
                                      , meta::lazy::at<meta::back<ZIPPED>, FLAGS_IDX>
                                      >;
   
    // *** CLEAN UP VALUES *** 
    // NB: `meta::if_` is less typing, not better than, `std::conditional_t`

    // map `void` FLAGS to zero
    using DEFN_INFO_FLAGS = meta::if_<std::is_void<INFO_FLAGS>, meta::int_<0>, INFO_FLAGS>;

    // map `void` test to `int_<0>` (always matches)
    using DEFN_TST        = meta::if_<std::is_void<typename OP::tst>, meta::int_<0>, typename OP::tst>;
   
    // six fixed types, plus additional `VALIDATORs`
    using type = meta::list<DEFN_INFO_FLAGS         // rationalized FLAGS
                          , typename INFO_FN::type  // evaluate deferred calculation
                          , NAME                    // opcode base NAME
                          , typename OP::code       // retrieve base binary code
                          , DEFN_TST                // rationalized TST
                          , FMT                     // formatter
                          , VALs...                 // validators
                          >;
};


}


#endif

