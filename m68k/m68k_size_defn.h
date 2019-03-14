#ifndef KAS_M68K_SIZE_DEFN_H
#define KAS_M68K_SIZE_DEFN_H

// Definitions for M68K INSN argument sizes

// The M68K instruction definition specifies a `size` for each instruction.
// For most ALU operations, these sizes are BYTE, WORD, and LONG. For other
// operations, different sizes are specified. These sizes concern the assembler
// because:
//
// 1. The argument size is typically encoded in the opcode
// 2. The argument size is typically specified as a suffix to the opcode
//      eg: move.l, move.w, etc
//
// But with complex instruction sets, nothing is simple. Some instructions
// may have a size but with optional suffix (eg: addq, addq.l are both long).
// Other variations exist (suffix with void size, etc, etc)
//
// To support this, each instruction `size` is passed a MPL tuple containing
// "size, [Suffix]*" (eg a size, and an optional list of suffixes). For example,
// an instruction supporting "floating point extended type", this type is:
// `meta::list<std::integral_constant<m68k_size_t, OP_SIZE_XTND>, IS<'x'>>`
// (where IS is an integer_sequence of INT). Similarly, the type for
// `addq` (OP_SIZE_LONG, with ".l" and no suffix) would be:
// `list<integral_constant<m68k_size_t, OP_SIZE_LONG>, IS<'l'>, kas_string<>>`
//
// It should be noted that the order of suffixes is immaterial to the
// assembler: the first is canonical for the disassembler, but it is otherwise
// unimportant.


// The other item supported by this file is insertion of the
// OP_SIZE_* value into the base opcode. This is performed by the
// `insn_add_size` template which is specialized as `INFO_SIZE_*` types.
// This `type`s operations are constexpr, so all calculations are performed
// at compilation.
//
// This calculation is not as simple as it might seem because the M68K
// instruction set encodes "BYTE/WORD/LONG" as a different tuple for
// different instructions (sigh...). The values are also inserted
// in several different locations in both 16 & 32-bit opcodes.
//
// As a simplification, if only a *single* OP_SIZE_* value is
// supported for a particular instruction (eg: MOVE.W SR, D0),
// the OPCODE value is left unmodified. This makes it easier to code
// the OPCODE tables.

#include <meta/meta.hpp>
#include <type_traits>

namespace kas::m68k::opc
{
    // all m68k instructions have a specified "size"
    // enumerate them (align values with m68881 source register values)
    enum m68k_size_t
    {
          OP_SIZE_LONG      // 0
        , OP_SIZE_SINGLE    // 1
        , OP_SIZE_XTND      // 2
        , OP_SIZE_PACKED    // 3
        , OP_SIZE_WORD      // 4
        , OP_SIZE_DOUBLE    // 5
        , OP_SIZE_BYTE      // 6
        , OP_SIZE_VOID      // 7 NOT ACTUAL OP_SIZE
        , NUM_OP_SIZE
    };

    // declare object code sizes of each size immed arg (bytes): (void -> 0)
    static constexpr uint8_t m68k_size_immed[] = { 4, 4, 12, 12, 2, 8, 2, 0 };

    // instructions for instruction suffix handling:
    // eg: instructions such as `moveq.l` can also be spelled `moveq`
    // SFX_* types say how to handle "blank" suffix
    struct SFX_NORMAL
    {
        static constexpr auto optional  = false;
        static constexpr auto canonical = false;
        static constexpr auto only_none = false;
    };

    struct SFX_OPTIONAL : SFX_NORMAL
    {
        static constexpr auto optional  = true;
    };
    struct SFX_NONE : SFX_NORMAL
    {
        static constexpr auto only_none  = true;
    };
    struct SFX_CANONICAL_NONE : SFX_NORMAL
    {
        static constexpr auto canonical  = true;
    };
    
    //
    // declare "types" for use in instruction definintion
    //

    // only single-sizes have non-standard suffix treatment
    template <int OP_SIZE, typename SFX = SFX_NORMAL>
    using define_sz = meta::list<meta::int_<1 << OP_SIZE>, SFX>;

    // multiple sizes: generate `list` directly`
    using sz_lwb  = meta::list<meta::int_<(1 << OP_SIZE_LONG) | (1 << OP_SIZE_WORD) | (1 << OP_SIZE_BYTE)>>;
    using sz_lw   = meta::list<meta::int_<(1 << OP_SIZE_LONG) | (1 << OP_SIZE_WORD)>>;
    using sz_wb   = meta::list<meta::int_<(1 << OP_SIZE_WORD) | (1 << OP_SIZE_BYTE)>>;
    using sz_all  = meta::list<meta::int_<0x7f>>;

    // single-sizes
    using sz_b    = define_sz<OP_SIZE_BYTE>;
    using sz_w    = define_sz<OP_SIZE_WORD>;
    using sz_l    = define_sz<OP_SIZE_LONG>;
    using sz_s    = define_sz<OP_SIZE_SINGLE>;
    using sz_d    = define_sz<OP_SIZE_DOUBLE>;
    using sz_x    = define_sz<OP_SIZE_XTND>;
    using sz_p    = define_sz<OP_SIZE_PACKED>;

    // void never has suffix (also: always single size)
    using sz_void = define_sz<OP_SIZE_VOID, SFX_NONE>;
    using sz_v    = sz_void;
    
    // only difference between v% & %v: first name is canonical.
    using sz_wv   = define_sz<OP_SIZE_WORD, SFX_OPTIONAL>;
    using sz_vw   = define_sz<OP_SIZE_WORD, SFX_CANONICAL_NONE>;
    using sz_lv   = define_sz<OP_SIZE_LONG, SFX_OPTIONAL>;
    using sz_vl   = define_sz<OP_SIZE_LONG, SFX_CANONICAL_NONE>;
    using sz_bv   = define_sz<OP_SIZE_BYTE, SFX_OPTIONAL>;
    using sz_vb   = define_sz<OP_SIZE_BYTE, SFX_CANONICAL_NONE>;

    // set size field, but no suffix (capital W/L). not common.
    using sz_W    = define_sz<OP_SIZE_WORD, SFX_NONE>;
    using sz_L    = define_sz<OP_SIZE_LONG, SFX_NONE>;


    struct m68k_insn_size
    {
        // list of suffixes by size
        static constexpr char m68k_size_suffixes[] = "lsxpwdbv";
        // what to return for "no suffix"
        static constexpr char suffix_void = m68k_size_suffixes[OP_SIZE_VOID];

        template <typename MASK, typename SFX>
        constexpr m68k_insn_size(meta::list<MASK, SFX>)
            : size_mask           { MASK::value }
            , single_size         { !(MASK::value & (MASK::value - 1)) }
            , opt_no_suffix       { SFX::optional }
            , no_suffix_canonical { SFX::canonical }
            , only_no_suffix      { SFX::only_none }
            {}

        template <typename MASK>
        constexpr m68k_insn_size(meta::list<MASK>)
                : m68k_insn_size(meta::list<MASK, SFX_NORMAL>()) {}

        
        // create an `iterator` to allow range-for to process sizes
        struct iter
        {
            iter(m68k_insn_size const& obj, bool make_begin = {}) : obj(obj)
            {
                if (make_begin)
                {
                    if (obj.size_mask & 1)
                        sz = 0;
                    else
                        sz = next(0);
                }
            }

            // find first size after `prev`
            uint8_t next(uint8_t prev) const
            {
                // if any left, find next
                for (int n = obj.size_mask >> ++prev; n ; ++prev)
                    if (n & 1)
                        return prev;
                    else
                        n >>= 1;
                
                return NUM_OP_SIZE;
            }

            // range operations
            auto& operator++() 
            {
                sz = next(sz);
                return *this;
            }
            auto operator*() const
            { 
                return static_cast<m68k_size_t>(sz);
            }
            auto operator==(iter const& other) const
            { 
                return other.sz == sz;
            }
            auto operator!=(iter const& other) const
            {
                return !(*this == other);
            }
        
        private:
            m68k_insn_size const& obj;
            uint8_t               sz{NUM_OP_SIZE};
        };

        auto begin() const { return iter(*this, true); }
        auto end()   const { return iter(*this);       }

        std::pair<char, char> suffixes(uint8_t sz) const
        {
            // NB: all 4 combinations are represented
            auto sfx = m68k_size_suffixes[sz];

            if (only_no_suffix)
                return { suffix_void, 0 };
            else if (no_suffix_canonical)
                return { suffix_void, sfx };
            else if (opt_no_suffix)
                return { sfx, suffix_void };
            else
                return { sfx, 0 };
        }
        
        uint16_t size_mask           : 8;
        uint16_t single_size         : 1;
        uint16_t opt_no_suffix       : 1;
        uint16_t no_suffix_canonical : 1;
        uint16_t only_no_suffix      : 1;
    };
}

#endif
