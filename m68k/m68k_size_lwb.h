#ifndef KAS_M68K_M68K_SIZE_LWB_H
#define KAS_M68K_M68K_SIZE_LWB_H

#include "m68k_mcode.h"             // need op-size defns
#include "target/tgt_opc_base.h"    // extending `tgt_opc_base`

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

/////////////////////////////////////////////////////////////////////////
//
//  declare complicated code for insertion and extraction of `op_size`
//  into base opcode
//
//  declare size functions (as types)
//
/////////////////////////////////////////////////////////////////////////

template <int WORD, int BIT, int SEQ>
using insn_add_size = meta::list<meta::int_<WORD>, meta::int_<BIT>, meta::int_<SEQ>>;

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

// declare types used in INSN definitions
using INFO_SIZE_NORM  = insn_add_size<0,  6, SEQ_BWL_012>;
using INFO_SIZE_MOVE  = insn_add_size<0, 12, SEQ_BWL_132>;
using INFO_SIZE_BWL9  = insn_add_size<0,  9, SEQ_BWL_123>;
using INFO_SIZE_WL    = insn_add_size<0,  6, SEQ_WL_01>;
using INFO_SIZE_WL0   = insn_add_size<0,  0, SEQ_WL_01>;
using INFO_SIZE_WL9   = insn_add_size<0,  9, SEQ_WL_01>;
using INFO_SIZE_FLT   = insn_add_size<1, 10, SEQ_FLT>;
using INFO_SIZE_NORM1 = insn_add_size<1,  6, SEQ_BWL_012>;
using INFO_SIZE_MAC   = insn_add_size<1, 11, SEQ_WL_01>;
using INFO_SIZE_VOID  = insn_add_size<0,  0, SEQ_VOID>;

using INFO_SIZE_LIST  = insn_add_size<0,  12, SEQ_FLT>;     // stash size in MSBs

// declare meta::list with all defined configurations
using LWB_SIZE_LIST = meta::list<
                              INFO_SIZE_NORM        // default type is first
                            , INFO_SIZE_VOID        // VOID type must be second
                            , INFO_SIZE_MOVE 
                            , INFO_SIZE_BWL9 
                            , INFO_SIZE_WL   
                            , INFO_SIZE_WL0  
                            , INFO_SIZE_WL9  
                            , INFO_SIZE_FLT  
                            , INFO_SIZE_NORM1
                            , INFO_SIZE_MAC  
                            , INFO_SIZE_LIST
                            >;

///////////////////////////////////////////////////////////////////////    
//
// implement the "size_fn" type
//
///////////////////////////////////////////////////////////////////////    

struct m68k_insn_lwb 
{
    using value_t   = uint8_t;          // storage for type
    using default_t = INFO_SIZE_NORM;   // default size fn 

    template <typename W, typename B, typename S>
    constexpr m68k_insn_lwb(meta::list<W, B, S>)
        : sz_seq{S::value}, sz_word{W::value}, sz_bit{B::value} {}

    // need default ctor
    constexpr m68k_insn_lwb() : m68k_insn_lwb( default_t() ) {};

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
                return OP_SIZE_VOID;

            default:
                break;
        }

        return -1;
    }
public:
    // used by m68k_mcode_t::code() to generate size code
    uint16_t operator()(uint8_t sz) const
    {
        auto code = get_code(sz_seq, sz);
        if (code < 0)
            throw std::runtime_error{"Invalid insn_lwb code"};
        return code << sz_bit;
    }
   
    // used by m68k_mcode_t::extract_info() to decode embedded size code
    uint8_t extract(uint16_t const *code_p) const
    {
        auto lwb_code = code_p[sz_word] >> sz_bit;
        auto sz = get_sz(sz_seq, lwb_code & 7);

        if (lwb_code < 0)
            throw std::runtime_error{"Invalid insn_lwb code (get)"};
        return sz;
    }

    uint8_t word() const { return sz_word; }
    
private:
    // consexpr "value" is a `uint8_t`
    static_assert(NUM_SEQ <= 8);    // must fit in the allocated 3 bits
    
    value_t sz_seq  : 3;
    value_t sz_word : 1;
    value_t sz_bit  : 4;
};

}
#endif

