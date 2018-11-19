#ifndef KAS_M68K_M68K_FORMATS_TYPE_H
#define KAS_M68K_M68K_FORMATS_TYPE_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

#include "m68k_types.h"
//#include "m68k_stmt.h"

namespace kas::m68k::opc
{
// forward declare opcode;
struct m68k_stmt_opcode;

struct m68k_opcode_fmt
{
    // NB: info only
    static constexpr auto FMT_MAX_ARGS = 6;

    // "instance" is just a virtual-pointer collection
    // NB: requires `constexpr` ctor to be used as constexpr
    constexpr m68k_opcode_fmt() {};
    
    // Implement `FMT_MAX_ARGS` inserters & extractors
    // default to no-ops for inserters/extractors instead of ABC
    virtual bool insert_arg1(uint16_t* op, m68k_arg_t& arg) const { return false; }
    virtual bool insert_arg2(uint16_t* op, m68k_arg_t& arg) const { return false; }
    virtual bool insert_arg3(uint16_t* op, m68k_arg_t& arg) const { return false; }
    virtual bool insert_arg4(uint16_t* op, m68k_arg_t& arg) const { return false; }
    virtual bool insert_arg5(uint16_t* op, m68k_arg_t& arg) const { return false; }
    virtual bool insert_arg6(uint16_t* op, m68k_arg_t& arg) const { return false; }

    virtual void extract_arg1(uint16_t const* op, m68k_arg_t *arg) const {}
    virtual void extract_arg2(uint16_t const* op, m68k_arg_t *arg) const {}
    virtual void extract_arg3(uint16_t const* op, m68k_arg_t *arg) const {}
    virtual void extract_arg4(uint16_t const* op, m68k_arg_t *arg) const {}
    virtual void extract_arg5(uint16_t const* op, m68k_arg_t *arg) const {}
    virtual void extract_arg6(uint16_t const* op, m68k_arg_t *arg) const {}

    auto insert(unsigned n, uint16_t* op, m68k_arg_t& arg) const
    {
        switch (n)
        {
            case 0: return insert_arg1(op, arg);
            case 1: return insert_arg2(op, arg);
            case 2: return insert_arg3(op, arg);
            case 3: return insert_arg4(op, arg);
            case 4: return insert_arg5(op, arg);
            case 5: return insert_arg6(op, arg);
            default:
                throw std::runtime_error("insert: bad index");
        }
    }

    void extract(int n, uint16_t const* op, m68k_arg_t *arg) const
    {
        switch (n)
        {
            case 0: return extract_arg1(op, arg);
            case 1: return extract_arg2(op, arg);
            case 2: return extract_arg3(op, arg);
            case 3: return extract_arg4(op, arg);
            case 4: return extract_arg5(op, arg);
            case 5: return extract_arg6(op, arg);
            default:
                throw std::runtime_error("extract: bad index");
        }
    }

    // returns "fmt" for "resolved" instructions (ie all constant args)
    static m68k_opcode_fmt const& get_resolved_fmt();
    
    // returns "fmt" for "list" instructions
    static m68k_opcode_fmt const& get_list_fmt();
    
    // return "opcode derived type" for given fmt
    virtual m68k_stmt_opcode& get_opc() const = 0;
};

//
// declare `mix-in` types
//

template <unsigned, typename T> struct fmt_arg;

#define DEFN_ARG(N)                                                             \
template <typename T>                                                           \
struct fmt_arg<N, T> : virtual m68k_opcode_fmt                                  \
{                                                                               \
    bool insert_arg ## N (uint16_t* op, m68k_arg_t& arg) const override         \
        { return T::insert(op, arg);}                                           \
    void extract_arg ## N (uint16_t const* op, m68k_arg_t* arg) const override  \
        { T::extract(op, arg); }                                                \
};

// declare `FMT_MAX_ARGS` times
DEFN_ARG(1); DEFN_ARG(2); DEFN_ARG(3); DEFN_ARG(4); DEFN_ARG(5); DEFN_ARG(6)
#undef DEFN_ARG

}
#endif


