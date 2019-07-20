#ifndef KAS_M68K_M68K_SIZE_LWB_H
#define KAS_M68K_M68K_SIZE_LWB_H

#include "m68k_mcode.h"         // need op-size defns
#include "utility/align_as_t.h"

namespace kas::m68k::opc
{
using namespace meta;
using namespace hw;

/////////////////////////////////////////////////////////////////////////
//
//  for insertion of `op_size` into base opcode
//
//  declare size functions (as types)
//
/////////////////////////////////////////////////////////////////////////

template <int WORD, int BIT, int SEQ>
using insn_add_size = meta::list<meta::int_<WORD>, meta::int_<BIT>, meta::int_<SEQ>>;

// sequences (BWL) are surprisingly aribrary
enum { SEQ_BWL_012, SEQ_BWL_132, SEQ_BWL_123, SEQ_WL_01, SEQ_WL_23, SEQ_FLT, SEQ_VOID, NUM_SEQ };

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

using INFO_SIZE_LIST = meta::list<
                              INFO_SIZE_NORM        // default type is first
                            , INFO_SIZE_MOVE 
                            , INFO_SIZE_BWL9 
                            , INFO_SIZE_WL   
                            , INFO_SIZE_WL0  
                            , INFO_SIZE_WL9  
                            , INFO_SIZE_FLT  
                            , INFO_SIZE_NORM1
                            , INFO_SIZE_MAC  
                            , INFO_SIZE_VOID 
                            >;

///////////////////////////////////////////////////////////////////////    
//
// implement the "size_fn" type
//
///////////////////////////////////////////////////////////////////////    

struct m68k_insn_lwb : alignas_t<m68k_insn_lwb, uint8_t>
{
    using base_t::base_t;

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
public:
    uint16_t operator()(uint8_t sz) const
    {
        auto code = get_code(sz_seq, sz);
        if (code < 0)
            throw std::runtime_error{"Invalid insn_lwb code"};
        return code << sz_bit;
    }
   
    uint8_t word() const { return sz_word; }
    
    static_assert(NUM_SEQ <= 8);    // must fit in the allocated 3 bits

private:
    value_t sz_seq  : 3;
    value_t sz_word : 1;
    value_t sz_bit  : 4;
};


}


#endif
