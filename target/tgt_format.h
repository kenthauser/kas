#ifndef KAS_TARGET_TGT_FORMAT_H
#define KAS_TARGET_TGT_FORMAT_H

// type interface: two static methods
//
// bool insert (mcode_size_t const *op, arg_t& arg, val_t const *val_p)
// void extract(mcode_size_t const *op, arg_t *arg, val_t *constval_p)
//
// in the above:
//  *op     points to first word of "machine code"
//  arg     is current argument being processed. 
//  *val_p  points to current argument validator
//
// insert returns `true` if argument completely stored in machine code

#include "kas_core/opcode.h"
#include "tgt_opc_list.h"
#include "tgt_opc_general.h"
#include "tgt_opc_branch.h"

#include <limits>

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_format
{
    // NB: info only
    static constexpr auto FMT_MAX_ARGS = 6;

    // extract supporting types
    using mcode_t      = MCODE_T;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using arg_t        = typename MCODE_T::arg_t;
    using val_t        = typename MCODE_T::val_t;
    using opcode_t     = typename MCODE_T::opcode_t;

    using core_emit    = core::core_emit;

    static_assert(FMT_MAX_ARGS >= MCODE_T::MAX_ARGS);

    // formatter insertion actions
    // virtual methods in this type overridden to create formatters.
    // see example `fmt_generic` which follows method definition.
    struct fmt_impl
    {
        virtual bool insert(mcode_size_t* op, arg_t& arg, val_t const * val_p) const
        {
            return false;        // no-validator: use `data` to insert arg
        }

        virtual void extract(mcode_size_t const* op, arg_t& arg, val_t const * val_p) const 
        {
            // default: invoke `set_arg` with zero value
            if (val_p) val_p->set_arg(arg, 0);
        }

        
        virtual void emit_reloc(core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const
            {}
    };
    
private: 
    // NB: using a `static inline` instead of method makes CLANG drop core 2019/08/21 KBH
    auto& default_impl() const
    {
        constexpr static fmt_impl dummy;
        return dummy;
    }

    // allow override of formatter implementation for each arg
    virtual fmt_impl const& get_arg1() const { return default_impl(); }
    virtual fmt_impl const& get_arg2() const { return default_impl(); }
    virtual fmt_impl const& get_arg3() const { return default_impl(); }
    virtual fmt_impl const& get_arg4() const { return default_impl(); }
    virtual fmt_impl const& get_arg5() const { return default_impl(); }
    virtual fmt_impl const& get_arg6() const { return default_impl(); }

    auto& get_impl(uint8_t n) const
    {
        switch (n)
        {
            case 0: return get_arg1();
            case 1: return get_arg2();
            case 2: return get_arg3();
            case 3: return get_arg4();
            case 4: return get_arg5();
            case 5: return get_arg6();
            default:
                throw std::runtime_error("tgt_format::get_impl: bad index");
        }
    }
    
public:
    // access inserters/extractors via public interface
    auto insert(unsigned n, mcode_size_t* op, arg_t& arg, val_t const *val_p) const 
    {
        if (val_p)
            return get_impl(n).insert(op, arg, val_p);
        return false;       // "not completely saved"
    }

    void extract(int n, mcode_size_t const* op, arg_t& arg, val_t const *val_p) const
    {
        if (val_p)
            get_impl(n).extract(op, arg, val_p);
    }
    
    void emit_reloc(unsigned n, core_emit& base, mcode_size_t* op, arg_t& arg, val_t const *val_p) const 
    {
        if (val_p)
            get_impl(n).emit_reloc(base, op, arg, val_p);
    }

    // return "opcode derived type" for given fmt
    virtual opcode_t& get_opc() const = 0;
};

// generic types to support `opc_general`, `opc_list` & `opc_branch` opcodes
template <typename MCODE_T, typename OPC = tgt_opc_general<MCODE_T>>
struct tgt_fmt_opc_gen : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static OPC opc;
        return opc;
    }
};

template <typename MCODE_T, typename OPC = tgt_opc_list<MCODE_T>>
struct tgt_fmt_opc_list : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static OPC opc;
        return opc;
    }
};

template <typename MCODE_T, typename OPC = tgt_opc_branch<MCODE_T>>
struct tgt_fmt_opc_branch : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static OPC opc; 
        return opc;
    }
};


//
// generic `fmt_impl` to extract N bits at OFFSET for WORD (0-baased)
// always paired: insert & extract
//
// SHIFT, BITS, and WORD are based on `INSN_T` sized word value.
// 
// Normally, `mcode_size_t` and `INSN_T` will be same size. However for architectures
// which have multiple insn sizes (looking at you ARM + THUMB), `mcode_size_t` will match
// smallest insn size & `INSN_T` will match current insn size. 
//
// use constexpr calculations to make it just work.
//

// Insert/Extract N bits from machine code
// NB: must work correctly for BITS == 0 (ie, do nothing)
template <typename MCODE_T, typename INSN_T, unsigned SHIFT, unsigned BITS, unsigned WORD = 0>
struct tgt_fmt_generic : MCODE_T::fmt_t::fmt_impl
{
    using core_emit    = core::core_emit;
    
    // extract types from `MCODE_T`
    using arg_t        = typename MCODE_T::arg_t;
    using val_t        = typename MCODE_T::val_t;
    using fmt_t        = typename MCODE_T::fmt_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;

    // NB: require INSN size to be multiple of `mcode_size_t`
    static constexpr auto insn_per_mcode  = sizeof(INSN_T) / sizeof(mcode_size_t);
    static constexpr auto insn_size_bits  = std::numeric_limits<INSN_T>::digits;
    static constexpr auto mcode_size_bits = std::numeric_limits<mcode_size_t>::digits;

    static_assert(mcode_size_bits * insn_per_mcode == insn_size_bits);
   
    // allow values to span max of two `mcode_size_t` code-array values
    static constexpr auto M_FRAG0  = (SHIFT + 0   ) / mcode_size_bits;
    static constexpr auto M_FRAG1  = (SHIFT + BITS) / mcode_size_bits;
    static constexpr auto M_WORD0  = insn_per_mcode - M_FRAG0 - 1 + (WORD * insn_per_mcode);
    static constexpr auto M_WORD1  = insn_per_mcode - M_FRAG1 - 1 + (WORD * insn_per_mcode);
    static constexpr auto M_SHIFT0 = (SHIFT + 0       ) % mcode_size_bits;
    static constexpr auto M_SHIFT1 = (SHIFT + BITS - 1) % mcode_size_bits;
    
    //static constexpr auto M_BITS0  = BITS - ((SHIFT + BITS) % 16);
    static constexpr auto M_BITS0 = BITS;
    static constexpr auto M_BITS1  = BITS - M_BITS0;

  //  static_assert((M_WORD1 - M_WORD0) < 2); // max two writes

    //static constexpr auto MASK0 = (1 << M_BITS0) - 1;
    static constexpr auto MASK0 = (1 << M_BITS0) - 1;
    static constexpr auto MASK1 = (1 << M_BITS1) - 1;

    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        if constexpr (M_BITS0 != 0)
        {
            std::cout << std::hex;
            std::cout << "INSERT: " << +WORD << "/" << +SHIFT << "/" << +BITS << std::endl; 
            std::cout << "insert: " << +M_WORD0 << "/" << +M_SHIFT0 << "/" << +M_BITS0;
            std::cout << "/" << MASK0; 
            std::cout << ", value = " << +value << std::endl;
            // lower word
            auto code  = op[M_WORD0]; 
                 code &= ~(MASK0 << M_SHIFT0);
                 code |= (value & MASK0) << M_SHIFT0;
            op[M_WORD0]   = code;
        }
        if constexpr (M_BITS1 != 0)
        {
            std::cout << "insert: " << +M_WORD1 << "/" << +M_SHIFT1 << "/" << +M_BITS1;
            std::cout << "/" << MASK1; 
            std::cout << ", value = " << (value >> M_BITS0) << std::endl;
            // upper word
            auto code  = op[M_WORD1]; 
                 code &= ~MASK1;        // WORD1 always continues from bit0
                 code |= (value >> M_BITS0) & MASK1;
            op[M_WORD1]   = code;
        }
        return !val_p->has_data(arg);
    }

    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        INSN_T value{};
        if constexpr (M_BITS0 != 0)
        {
            value = MASK0 & (op[M_WORD0] >> M_SHIFT0);
        }
        if constexpr (M_BITS1 != 0)
        {
            value |= (MASK1 & op[M_WORD1]) << M_BITS0;
        }
        val_p->set_arg(arg, value);
    }
    
    // don't need to override `emit_reloc` for generic type
};

//
// declare specialized templates to override the argument formatters
//

// declare template for arg formatter 
template <typename MCODE_T, unsigned, typename T>
struct tgt_fmt_arg;

// specialize arg formatter for each arg (as a virtual base class)
// (Override `get_argN` and instantiate formatter `T`)
#define DEFN_ARG(N)                                                             \
template <typename MCODE_T, typename T>                                         \
struct tgt_fmt_arg<MCODE_T, N, T> : virtual MCODE_T::fmt_t                      \
{   using fmt_impl = typename MCODE_T::fmt_t::fmt_impl;                         \
    fmt_impl const& get_arg ## N () const override                              \
        { constexpr static T impl; return impl; } };


// specialize for each of the `FMT_MAX_ARGS` arguments
DEFN_ARG(1)
DEFN_ARG(2)
DEFN_ARG(3)
DEFN_ARG(4)
DEFN_ARG(5)
DEFN_ARG(6)

#undef DEFN_ARG

}
#endif


