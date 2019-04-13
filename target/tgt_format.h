#ifndef KAS_TARGET_TGT_FORMAT_H
#define KAS_TARGET_TGT_FORMAT_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

//#include "z80_types.h"
//#include "z80_stmt.h"

#include "kas_core/opcode.h"
#include "tgt_opc_general.h"
#include "tgt_opc_list.h"

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

    static_assert(FMT_MAX_ARGS >= MCODE_T::MAX_ARGS);

    // "instance" is just a virtual-pointer collection
    // NB: requires `constexpr` ctor to be used as constexpr
    constexpr tgt_format() {};
    
    // Implement `FMT_MAX_ARGS` inserters & extractors
    // default to no-ops for inserters/extractors instead of ABC
    // NB: missing validators occur during serialize/deserialize
    
    // format first, compress rest
    virtual bool insert_arg1(mcode_size_t* op, arg_t& arg, val_t const *val_p) const 
    { 
        // prototype `arg1`: not completely saved
        return false;
    }
    virtual bool insert_arg2(mcode_size_t* op, arg_t& arg, val_t const *val_p) const
        { return false; }
    virtual bool insert_arg3(mcode_size_t* op, arg_t& arg, val_t const *val_p) const
        { return false; }
    virtual bool insert_arg4(mcode_size_t* op, arg_t& arg, val_t const *val_p) const
        { return false; }
    virtual bool insert_arg5(mcode_size_t* op, arg_t& arg, val_t const *val_p) const
        { return false; }
    virtual bool insert_arg6(mcode_size_t* op, arg_t& arg, val_t const *val_p) const
        { return false; }

    // format first, compress rest
    virtual void extract_arg1(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
    {
        // prototype `arg1`
        if (val_p)
            val_p->set_arg(*arg, 0);
    }
    virtual void extract_arg2(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
        { if (val_p) val_p->set_arg(*arg, 0); }
    virtual void extract_arg3(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
        { if (val_p) val_p->set_arg(*arg, 0); }
    virtual void extract_arg4(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
        { if (val_p) val_p->set_arg(*arg, 0); }
    virtual void extract_arg5(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
        { if (val_p) val_p->set_arg(*arg, 0); }
    virtual void extract_arg6(mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
        { if (val_p) val_p->set_arg(*arg, 0); }

    auto insert(unsigned n, mcode_size_t* op, arg_t& arg, val_t const *val_p) const
    {
        switch (n)
        {
            case 0: return insert_arg1(op, arg, val_p);
            case 1: return insert_arg2(op, arg, val_p);
            case 2: return insert_arg3(op, arg, val_p);
            case 3: return insert_arg4(op, arg, val_p);
            case 4: return insert_arg5(op, arg, val_p);
            case 5: return insert_arg6(op, arg, val_p);
            default:
                throw std::runtime_error("insert: bad index");
        }
    }

    void extract(int n, mcode_size_t const* op, arg_t *arg, val_t const *val_p) const
    {
        switch (n)
        {
            case 0: return extract_arg1(op, arg, val_p);
            case 1: return extract_arg2(op, arg, val_p);
            case 2: return extract_arg3(op, arg, val_p);
            case 3: return extract_arg4(op, arg, val_p);
            case 4: return extract_arg5(op, arg, val_p);
            case 5: return extract_arg6(op, arg, val_p);
            default:
                throw std::runtime_error("extract: bad index");
        }
    }
    
    // return "opcode derived type" for given fmt
    virtual opcode_t& get_opc() const = 0;
};

// generic types to support `opc_general & `opc_list` opcodes
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

//
// generic type to extract N bits OFFSET for WORD (0-baased)
// always paired: insert & extract
//

// Insert/Extract N bits from machine code
template <typename MCODE_T, unsigned SHIFT, unsigned BITS, unsigned WORD = 0>
struct tgt_fmt_generic
{
    // extract types from `MCODE_T`
    using arg_t        = typename MCODE_T::arg_t;
    using val_t        = typename MCODE_T::val_t;
    using fmt_t        = typename MCODE_T::fmt_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    
    static constexpr auto MASK = (1 << BITS) - 1;
    static bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p)
    {
        kas::expression::expr_fits fits;
        
        // if no validator, not  completely stored
        if (!val_p) return false;

        auto value = val_p->get_value(arg);
        auto code  = op[WORD]; 
             code &= ~(MASK << SHIFT);
             code |= (value & MASK) << SHIFT;
        op[WORD]   = code;
        return val_p->all_saved(arg);
    }

    static void extract(mcode_size_t const* op, arg_t* arg, val_t const *val_p)
    {
        auto value = MASK & (op[WORD] >> SHIFT);
        val_p->set_arg(*arg, value);
    }
};

//
// declare `mix-in` types for arguments
//

// declare template for arg inserter
template <typename MCODE_T, unsigned, typename T>
struct tgt_fmt_arg;

// specialize arg inserter for each arg
#define DEFN_ARG(N)                                                                                 \
template <typename MCODE_T, typename T>                                                             \
struct tgt_fmt_arg<MCODE_T, N, T> : virtual MCODE_T::fmt_t                                          \
{                                                                                                   \
    using mcode_size_t = typename MCODE_T::mcode_size_t;                                            \
    using arg_t        = typename MCODE_T::arg_t;                                                   \
    using val_t        = typename MCODE_T::val_t;                                                   \
    bool insert_arg ## N (mcode_size_t* op, arg_t& arg, val_t const * val_p) const override         \
        { return T::insert(op, arg, val_p);}                                                        \
    void extract_arg ## N (mcode_size_t const* op, arg_t* arg, val_t const * val_p) const override  \
        { T::extract(op, arg, val_p); }                                                             \
};

// declare `FMT_MAX_ARGS` times
DEFN_ARG(1)
DEFN_ARG(2)
DEFN_ARG(3)
DEFN_ARG(4)
DEFN_ARG(5)
DEFN_ARG(6)
#undef DEFN_ARG

}
#endif


