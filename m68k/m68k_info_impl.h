#ifndef KAS_M68K_M68K_INFO_IMPL_H
#define KAS_M68K_M68K_INFO_IMPL_H

#include "m68k_mcode.h"             // need op-size defns
#include "target/tgt_opc_base.h"    // extending `tgt_opc_base`
#include "target/tgt_info_fn.h"

namespace kas::m68k::opc
{
using namespace meta;
using namespace hw;

/////////////////////////////////////////////////////////////////////////
//
// declare base_type for coldfire `limit_3w` support
//
/////////////////////////////////////////////////////////////////////////

struct m68k_opc_base : tgt::opc::tgt_opc_base<m68k_mcode_t>
{
    // implementation in `m68k_mcode_impl.h`
    op_size_t& cf_limit_3w(core::opcode_data&) const;
};

// code to insert "info" into "*LIST*" insn
struct m68k_info_list : m68k_mcode_t::info_fn_t
{
    void insert(code_t&     code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        // XXX need so generalized "set code value" method...
        code[0] = stmt_info.value();// << 16;
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        return code_p[0];// >> 16;
    }
};

using INFO_SIZE_LIST  = m68k_info_list;


/////////////////////////////////////////////////////////////////////////
//
//  declare base type to extract `sz` from DEFN code
//
/////////////////////////////////////////////////////////////////////////

struct m68k_info_fn_base : m68k_mcode_t::info_fn_t
{
    bool   single_sz(defn_info_t const& defn_info) const
    {
        auto szs = defn_info.value & 0x7f;      // ignore `void` bit
        return !(szs & (szs - 1));              // true if single bit set
    }
    
    int8_t defn_sz  (defn_info_t const& defn_info) const override
    {
        // extract sz from defn.
        // NB: used by `stmt_info` & `info_t`.
        int8_t sz {};
        
        // NB: code a loop to implement "find_first_set"
        for (auto n = defn_info.value; sz < OP_SIZE_VOID; ++sz)
        {
            if (n & 1)
                break;
            else
                n >>= 1;
        }

        // NB: the SFX_CANON_NONE flag to indicate `void` not applied here.
        // NB: the returned size is for internal arguments, not display.
        return sz;
    }
};

/////////////////////////////////////////////////////////////////////////
//
//  declare suprisingly complicated code for insertion and extraction of
//  `op_size` into base opcode
//
/////////////////////////////////////////////////////////////////////////

// sequences (BWL) are surprisingly aribrary
enum { SEQ_BWL_012
     , SEQ_BWL_132
     , SEQ_BWL_123
     , SEQ_WL_01
     , SEQ_WL_23
     , SEQ_FLT
     , SEQ_VOID
     , NUM_SEQ
     };

// declare methods in base type.
// Instantiate subclasses to minimize code bloat
struct info_add_size_t : m68k_info_fn_base
{
    using value_t = uint8_t;

    constexpr info_add_size_t(value_t WORD, value_t BIT, value_t SEQ)
                : seq {SEQ}, word {WORD}, bit {BIT} {}
    
    
    void insert(code_t& code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
#if 0
        std::cout << "info_add_size_t: insert";
        std::cout << ": SWB = " << +seq
                  << ", "       << +word
                  << ", "       << +bit
                  << std::endl;
        std::cout << "info_add_size_t: " << std::hex
                  << " arg_size = " << +stmt_info.arg_size
                  << " () = " << get_code(seq, stmt_info.arg_size) << std::endl;
#endif   
        auto encoded_size = get_code(seq, stmt_info.arg_size);
        if (encoded_size < 0)
            throw std::logic_error{"info_add_size_t: invalid size code"};
        code[word] |= encoded_size << bit;
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        // if single size in DEFN_INFO, extract from `defn_info`
        if (single_sz(defn_info))
            return defn_sz(defn_info);

        //std::cout << "info_add_size_t: extract" << std::endl;
        auto raw_code = code_p[word] >> bit;
        auto sz = get_sz(seq, raw_code & 3);

        // if sz undetermined from `code_p`, extract from `defn_info`
        if (sz < 0)
            sz = defn_sz(defn_info);
        return sz; 
    }

private:
    static constexpr int get_code(int seq, uint8_t sz)
    {
        switch (seq)
        {
            case SEQ_BWL_012:
                if (sz == OP_SIZE_BYTE) return 0;
                if (sz == OP_SIZE_WORD) return 1;
                if (sz == OP_SIZE_LONG) return 2;
                break;
                
            case SEQ_BWL_132:
                if (sz == OP_SIZE_BYTE) return 1;
                if (sz == OP_SIZE_WORD) return 3;
                if (sz == OP_SIZE_LONG) return 2;
                break;
                
            case SEQ_BWL_123:
                if (sz == OP_SIZE_BYTE) return 1;
                if (sz == OP_SIZE_WORD) return 2;
                if (sz == OP_SIZE_LONG) return 3;
                break;
                
            case SEQ_WL_01:
                if (sz == OP_SIZE_WORD) return 0;
                if (sz == OP_SIZE_LONG) return 1;
                break;
            
            case SEQ_WL_23:
                if (sz == OP_SIZE_WORD) return 2;
                if (sz == OP_SIZE_LONG) return 3;
                break;
            
            case SEQ_FLT:
                return sz;
            
            case SEQ_VOID:
                return 0;

            default:
                break;
        }

        return -1;
    }
    
    static constexpr int get_sz(int seq, uint8_t code)
    {
        switch (seq)
        {
            case SEQ_BWL_012:
                if (code == 0) return OP_SIZE_BYTE;
                if (code == 1) return OP_SIZE_WORD;
                if (code == 2) return OP_SIZE_LONG;
                break;
                
            case SEQ_BWL_132:
                if (code == 1) return OP_SIZE_BYTE;
                if (code == 3) return OP_SIZE_WORD;
                if (code == 2) return OP_SIZE_LONG;
                break;
                
            case SEQ_BWL_123:
                if (code == 1) return OP_SIZE_BYTE;
                if (code == 2) return OP_SIZE_WORD;
                if (code == 3) return OP_SIZE_LONG;
                break;
                
            case SEQ_WL_01:
                code &= 1;      // only single bit
                if (code == 0) return OP_SIZE_WORD;
                if (code == 1) return OP_SIZE_LONG;
                break;
            
            case SEQ_WL_23:
                if (code == 2) return OP_SIZE_WORD;
                if (code == 3) return OP_SIZE_LONG;
                break;
            
            case SEQ_FLT:
                return code;
            
            case SEQ_VOID:
            default:
                break;
        }

        return -1;
    }
private:
    value_t seq  : 3;
    value_t word : 1;
    value_t bit  : 4;
};

// extra indirection to reduce template code bloat
template <int WORD, int BIT, int SEQ>
struct info_add_size : info_add_size_t
{
    using base_t = info_add_size_t;

    constexpr info_add_size() : base_t(WORD, BIT, SEQ) {}
};

// declare types used in INSN definitions
using INFO_SIZE_NORM  = info_add_size<0,  6, SEQ_BWL_012>;
using INFO_SIZE_MOVE  = info_add_size<0, 12, SEQ_BWL_132>;
using INFO_SIZE_BWL9  = info_add_size<0,  9, SEQ_BWL_123>;
using INFO_SIZE_WL    = info_add_size<0,  6, SEQ_WL_01>;
using INFO_SIZE_WL0   = info_add_size<0,  0, SEQ_WL_01>;
using INFO_SIZE_WL9   = info_add_size<0,  9, SEQ_WL_01>;
using INFO_SIZE_FLT   = info_add_size<1, 10, SEQ_FLT>;
using INFO_SIZE_NORM1 = info_add_size<1,  6, SEQ_BWL_012>;
using INFO_SIZE_MAC   = info_add_size<1, 11, SEQ_WL_01>;
using INFO_SIZE_VOID  = info_add_size<0,  0, SEQ_VOID>;

// coordinate this DEFN with FMT_LIST definition:
//using INFO_SIZE_LIST  = info_add_size<0, 12, SEQ_FLT>;  // stash size in 4 MSBs

// declare methods in base type.
// Instantiate subclasses to minimize code bloat
struct info_add_ccode_t: m68k_info_fn_base
{
    using value_t = uint8_t;

    constexpr info_add_ccode_t(value_t WORD, value_t BIT, value_t IS_FP)
                : word {WORD}, bit {BIT}, is_fp(IS_FP)  {}
    
    
    void insert(code_t& code
              , stmt_info_t const& stmt_info
              , defn_info_t const& defn_info) const override
    {
        //std::cout << "info_add_ccode_t: insert";
        code[word] |= (stmt_info.ccode & 0xf) << bit;
    }
    
    stmt_info_t extract(mcode_size_t const *code_p
                      , defn_info_t const& defn_info) const override
    {
        //std::cout << "info_add_ccode_t: extract" << std::endl;
        auto ccode = code_p[word] >> bit;
             ccode &= (is_fp ? 0x1f : 0xf);

        stmt_info_t info{};
        
        // create `info` from `defn` & cc bits 
        info.arg_size  = defn_sz(defn_info);
        info.ccode     = ccode;
        info.has_ccode = true;
        info.fp_ccode  = is_fp;
        info.is_fp     = is_fp;
        
        return info;
    }

private:
    value_t word  : 1;
    value_t bit   : 4;
    value_t is_fp : 1;   // is FP
};

// extra indirection to reduce template code bloat
template <int WORD, int BIT, int IS_FP>
struct info_add_ccode : info_add_ccode_t
{
    using base_t = info_add_ccode_t;

    constexpr info_add_ccode() : base_t(WORD, BIT, IS_FP) {}
};

// declare types used in INSN definitions
using INFO_CCODE_NORM  = info_add_ccode<0,  8, 0>;
using INFO_CCODE_FP    = info_add_ccode<0,  0, 1>;
using INFO_CCODE_FP1   = info_add_ccode<1,  0, 1>;

}
#endif

