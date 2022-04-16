#ifndef KAS_Z80_Z80_INSN_COMMON_H
#define KAS_Z80_Z80_INSN_COMMON_H

// z80 instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "z80_formats.h"            // actual format types
#include "z80_validate.h"           // actual validate types

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::z80::opc
{

// declare opcode groups (ie: include files)
using z80_insn_defn_groups = meta::list<
      struct OP_Z80_GEN
    >;

// declare "FORMAT" for "*LIST*" insn
template <typename=void> struct z80_insn_defn_list : meta::list<> {};

// EXAMPLE: define `sz` types for first arg of `defn<>` template.
//          `sz_void` required. All others are per-arch
using namespace tgt::opc::traits;
using sz_void   = meta::int_<OP_SIZE_VOID>;
using sz_v      = sz_void;
using sz_b      = meta::int_<OP_SIZE_BYTE>;
using sz_w      = meta::int_<OP_SIZE_WORD>;

// XXX no "sz" fn, so no mapping
using z80_sz_info_map = meta::list<>;

#if 0
template <typename...Ts>
using defn = tgt_insn_defn<z80_mcode_t, z80_sz_info_map, Ts...>;
}
#else
// definition macro: INFO, NAME, OP, plus optional FMT & VALIDATORS
template <typename INFO, typename NAME, typename OP
        , typename FMT = void, typename...Ts>
struct defn
{
#if 0
    // look at `INFO` to pick appropriate `INFO_FN`
    static constexpr bool is_default   = std::is_void_v<typename OP::info_fn>;
    static constexpr bool is_single_sz = SINGLE_SZ<INFO>;
    static constexpr bool is_cond      = INFO::value & SFX_CCODE_ALL::value;
    static constexpr auto fp_types     = ((1 << OP_SIZE_SINGLE)
                                         |(1 << OP_SIZE_XTND  )
                                         |(1 << OP_SIZE_PACKED)
                                         |(1 << OP_SIZE_DOUBLE)
                                         );
    static constexpr bool is_fp        = INFO::value & fp_types;


    // select INFO_FN from `INFO`. respect defined method
    using INFO_FN = std::conditional_t<
                          // if specified, use named `info_fn`
                          !is_default, typename OP::info_fn
                       , std::conditional_t<
                          // if conditional insn, use CCODE_NORM
                          is_cond, INFO_CCODE_NORM
                       , std::conditional_t<
                          // if floating point, use INFO_SIZE_FLT
                          is_fp, INFO_SIZE_FLT
                       , std::conditional_t<
                          // if single width, use SIZE_VOID
                          is_single_sz, INFO_SIZE_VOID
                       ,
                          // default: INFO_SIZE_NORM
                          INFO_SIZE_NORM
                       >>>>;
#else
    using INFO_FN = std::conditional_t<
                        std::is_void_v<typename OP::info_fn>
                      , typename z80_mcode_t::info_fn_t
                      , typename OP::info_fn
                      >;
                    
#endif 
    // forward to `tg_mcode_adder`
    using type = meta::list<INFO
                          , INFO_FN
                          , NAME
                          , typename OP::code
                          , typename OP::tst
                          , FMT             // formatter
                          , Ts...           // validators
                          >;
};
#endif
}
#endif

