#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "arm_formats.h"            // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode_sizes.h"        // defin ARM insn variants

#include "arm_info_impl.h"

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
      meta::pair<void       , meta::int_<1>>     // handle void & simplify meta-code
    , meta::pair<a7_c       , arm_info_a7_c>
    , meta::pair<a7_cs      , arm_info_a7_cs>
    , meta::pair<a7_n       , arm_info_a7_c>
    , meta::pair<a7_ns      , arm_info_a7_cs>
    >;
    

template <std::size_t OPCODE, typename TST = void, typename INFO_FN = void, std::size_t MASK = 0>
struct OP
{
    // expose template args as member types
    using type    = OP;
    //using tst     = TST;
    using info_fn = INFO_FN;

    using code    = std::integral_constant<std::size_t, OPCODE>;
    using mask    = std::integral_constant<std::size_t, MASK>;
    
    // map `void` to `always pass` test
    using tst     = meta::if_<std::is_void<TST>, meta::int_<0>, TST>;
};

// general definition: IF type, NAME type, OP type, optional FMT & VALIDATORS
template <typename SZ, typename NAME, typename OP, typename FMT = void, typename...Ts>
struct defn
{
    // default "info" of `void` maps to zero
    using DEFN_INFO = meta::if_<std::is_same<void, SZ>, meta::int_<0>, SZ>;

    // see if SZ is in the sz->info map
    using ZIPPED   = meta::zip<arm_sz_info_map>;
    using SZ_IDX   = meta::find_index<meta::front<ZIPPED>, SZ>;

    // if not found, map info to index zero (void)
    using INFO_IDX = std::conditional_t<std::is_same_v<SZ_IDX, meta::npos>
                                      , meta::int_<0>, SZ_IDX>;
#if 0
    // if OP_FN specified use that, otherwise use mapped fn (may be void)
    using INFO_FN  = std::conditional_t<std::is_void_v<typename OP::info_fn>
                                      , meta::at<meta::back<ZIPPED>, INFO_IDX>
                                      , typename OP::info_fn
                                      >;
#else
    using INFO_FN = meta::at<meta::back<ZIPPED>, INFO_IDX>;
#endif
#if 0
    // XXX goes into common... 
    using INFO_FN_IDX = std::conditional_t<std::is_void_v<INFO_FN>
                                         , meta::int_<0>
                                         , meta::find_index<arm_info_fns, INFO_FN>
                                         >;
    //using INFO_FN = meta::at<meta::back<ZIPPED>, INFO_IDX>;
#else
    //using INFO_FN_IDX = meta::find_index<arm_info_fns, INFO_FN>;
    //using INFO_FN_IDX = INFO_FN;
    using INFO_FN_IDX  = meta::int_<0>;
    using DEFN_INFO_FN = void;
#endif
    static_assert(!std::is_same_v<INFO_FN_IDX, meta::npos>
                                , "Invalid info function");
    
#if 0

    // default `INFO_FN` index to zero
    using info_fn_idx = meta::if_<std::is_void<INFO_FN>
                            , meta::int_<0>
                            , meta::find_index<INFO_FN_LIST, INFO_FN>
                            >;

    // select IDX_NONE if single type in SZ
    using INFO_IDX = std::conditional_t<MULTIPLE_SZ<SZ>
                                    , typename OP::size_fn_idx
                                    , meta::int_<1>
                                    >;

    
    // M68K_INFO picks up additional value from `OP`
    using M68K_INFO = meta::int_<DEFN_INFO::value | (INFO_IDX::value << 12)>;
    static_assert(!std::is_same_v<info_fn_idx, meta::npos>, "Invalid INFO_FN");

#endif
    // six fixed types, plus additional `VALIDATORs`
    using type = meta::list<DEFN_INFO
                          , DEFN_INFO_FN
                          , NAME
                          , typename OP::code
                          , typename OP::tst
                          , FMT             // formatter
                          , Ts...           // validators
                          >;
};


}


#endif

