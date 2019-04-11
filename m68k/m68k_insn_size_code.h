#ifndef KAS_M68K_M68K_INSN_SIZE_CODE_H
#define KAS_M68K_M68K_INSN_SIZE_CODE_H

#include "m68k_mcode.h"

namespace kas::m68k::opc
{

///////////////////////////////////////////////////////////////////////    

enum m68k_cpid : int8_t
{
      M68K_CPID_MMU         // 0 = Memory management unit
    , M68K_CPID_FPU         // 1 = Floating point unit
    , M68K_CPID_MMU_040     // 2 = '040 MMU extensions
    , M68K_CPID_MOVE16      // 3 = move16 instructions
    , M68K_CPID_TABLE       // 4 = CPU_32/Coldfire table instructions
    , M68K_CPID_DEBUG = 15  // 15 = Debug instructions
};
    
    
/////////////////////////////////////////////////////////////////////////
//
//  for insertion of `op_size` into base opcode
//
//  declare size functions (as types)
//
/////////////////////////////////////////////////////////////////////////

template <int WORD, int BIT, int SEQ>
struct insn_add_size
{
    using type = meta::list<meta::int_<WORD>, meta::int_<BIT>, meta::int_<SEQ>>;
};

// sequences (BWL) are surprisingly aribrary
enum { SEQ_BWL_012, SEQ_BWL_132, SEQ_BWL_123, SEQ_WL_01, SEQ_WL_23, SEQ_FLT, SEQ_VOID, NUM_SEQ };

// declare types used in INSN definitions
using INFO_SIZE_NORM  = insn_add_size<0,  6, SEQ_BWL_012>;
using INFO_SIZE_MOVE  = insn_add_size<0, 12, SEQ_BWL_132>;
using INFO_SIZE_BWL9  = insn_add_size<0,  9, SEQ_BWL_123>;
using INFO_SIZE_WL    = insn_add_size<0,  6, SEQ_WL_01>;
using INFO_SIZE_WL0   = insn_add_size<0,  0, SEQ_WL_01>;
using INFO_SIZE_WL9   = insn_add_size<0,  9, SEQ_WL_01>;
using INFO_SIZE_FLT   = insn_add_size<1, 10, SEQ_FLT>;          // OP_SIZE is float "code"
using INFO_SIZE_NORM1 = insn_add_size<1,  6, SEQ_BWL_012>;
using INFO_SIZE_MAC   = insn_add_size<1, 11, SEQ_WL_01>;
using INFO_SIZE_VOID  = insn_add_size<0,  0, SEQ_VOID>;


///////////////////////////////////////////////////////////////////////    
//
// implement the "size_fn" type
//
///////////////////////////////////////////////////////////////////////    

struct insn_lwb
{
    using defn_size_t = uint8_t;

    template <typename W, typename B, typename S>
    constexpr insn_lwb(meta::list<W, B, S>)
        : sz_seq{S::value}, sz_word{W::value}, sz_bit{B::value} {}

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
    uint16_t lwb_code(uint8_t sz) const
    {
        auto code = get_code(sz_seq, sz);
        if (code < 0)
            throw std::runtime_error{"Invalid insn_lwb code"};
        return code << sz_bit;
    }
        
    defn_size_t sz_seq  : 3;
    defn_size_t sz_word : 1;
    defn_size_t sz_bit  : 4;
};


}


#endif
