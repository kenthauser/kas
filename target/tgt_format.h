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

    using emit_base    = core::emit_base;

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

        
        virtual void emit_reloc(emit_base& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const
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
    
    void emit_reloc(unsigned n, emit_base& base, mcode_size_t* op, arg_t& arg, val_t const *val_p) const 
    {
        if (val_p)
            get_impl(n).emit_reloc(base, op, arg, val_p);
    }

    // return "opcode derived type" for given fmt
    virtual opcode_t& get_opc() const = 0;
};

// generic types to support `opc_general`, `opc_list` & `opc_branch` opcodes
template <typename MCODE_T>
struct tgt_fmt_opc_gen : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static tgt_opc_general<MCODE_T> opc; 
        return opc;
    }
};

template <typename MCODE_T>
struct tgt_fmt_opc_list : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static tgt_opc_list<MCODE_T> opc; 
        return opc;
    }
};

template <typename MCODE_T>
struct tgt_fmt_opc_branch : virtual MCODE_T::fmt_t
{
    using opcode_t = typename MCODE_T::opcode_t;
    virtual opcode_t& get_opc() const override 
    {
        static tgt_opc_branch<MCODE_T> opc; 
        return opc;
    }
};


//
// generic `fmt_impl` to extract N bits at OFFSET for WORD (0-baased)
// always paired: insert & extract
//

// Insert/Extract N bits from machine code
// NB: must work correctly for BITS == 0 (ie, do nothing)
template <typename MCODE_T, unsigned SHIFT, unsigned BITS, unsigned WORD = 0>
struct tgt_fmt_generic : MCODE_T::fmt_t::fmt_impl
{
    // extract types from `MCODE_T`
    using arg_t        = typename MCODE_T::arg_t;
    using val_t        = typename MCODE_T::val_t;
    using fmt_t        = typename MCODE_T::fmt_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    
    using emit_base    = core::emit_base;
    
    static constexpr auto MASK = (1 << BITS) - 1;
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        auto code  = op[WORD]; 
             code &= ~(MASK << SHIFT);
             code |= (value & MASK) << SHIFT;
        op[WORD]   = code;

        return !val_p->has_data(arg);
    }

    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = MASK & (op[WORD] >> SHIFT);
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
#define DEFN_ARG(N)                                                                \
template <typename MCODE_T, typename T>                                            \
struct tgt_fmt_arg<MCODE_T, N, T> : virtual MCODE_T::fmt_t                         \
{   using fmt_impl = typename MCODE_T::fmt_t::fmt_impl;                            \
    fmt_impl const& get_arg ## N () const override { constexpr static T impl; return impl; } };


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


