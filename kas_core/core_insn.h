#ifndef KAS_CORE_CORE_INSN_H
#define KAS_CORE_CORE_INSN_H

///////////////////////////////////////////////////////////////////////////
//
//                          C O R E _ I N S N
//
///////////////////////////////////////////////////////////////////////////
//
// The `core_insn` class holds the result of evaluation of a `parser::parser_stmt`.
//
// The resulting pair {opcode, insn_data} is used to contruct `core_insn`.
// This `core_insn`is then written to the `insn_container`.
//
// When evaluating assembled data, information stored in `insn_container` is 
// reconstitued as `core_insn`. Various `core_insn` methods are used to relax,
// emit, and print associated `insn_container_data`, updating stored values as
// appropriate.
//
// Generally there is only a single `core_insn` instance active at any one time
// as it is totally transient.
//
///////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////

#include "opcode.h"
#include "core_fits.h"

#include <ostream>


namespace kas::core
{

struct core_insn
{
    using op_size_t = opcode::op_size_t;
   
    // init core_insn from `core_stmt` or `insn_container_data` 
    core_insn(opcode const& op, opc::insn_data data) : op(op), data(data) {}
    core_insn(insn_container_data&);

    // construct core_insn from opcode & arg list
    template <typename OPCODE, typename...Ts
        , typename = std::enable_if_t<std::is_base_of_v<opcode, OPCODE>>>
    core_insn(OPCODE const& op, Ts&&...args) : op(op)
    {
        op(data, std::forward<Ts>(args)...);
    }

    auto& size() const
    {
        return data.size;
    }

    auto& get_loc() const
    {
        return data.loc();
    }

    // convenience methods
    bool is_relaxed() const
    {
        return size().is_relaxed();
    }

////////////////////////////////////////////////////////////////////////
//
//  implement virtual `opcode` methods as `core_insn` methods
//
////////////////////////////////////////////////////////////////////////

    auto name() const
    {
        return op.name();
    }

    // format for test fixture interface
    void raw(std::ostream& os) 
    {
    #if 0
        // need this to debug data.fixed reference issue
        auto iter = data.iter();
        os << "fixed = "  << std::hex << data.fixed.fixed;
        os << " _fixed = "  << data._fixed.fixed;
        os << " first = " << data.first;
        os << " cnt = "   << data.cnt;
        os << " loc = "   << data.loc().get();
        os << std::endl;
        os << "raw:  ";
    #endif
        os << op.name() << " " << std::dec << data.size << ": ";
        op.raw(data, os);
    }

    void fmt(std::ostream& os) 
    {
        os << op.name() << " " << std::dec << data.size << ": ";
        op.fmt(data, os);
    }
    
    // used during relax
    auto calc_size(core_fits const& fits) 
    {
        return op.calc_size(data, fits);
    }

    // emit to object file or listing
    void emit(emit_base& base, core_expr_dot const *dot_p)
    {
#define VALIDATE_EMIT
#ifdef  VALIDATE_EMIT
        // extra tests are for validation only
        auto wr_size = data.size();
        auto expect = base.position() + wr_size;
        auto section_p = &base.get_section();
#endif

        // perform `opcode` method...
        op.emit(data, base, dot_p);

        
#ifdef  VALIDATE_EMIT
        if (&base.get_section() != section_p)
            return;     // changed section -- can't test

        if (auto delta = expect - base.position())
        {
            std::cout << "core_insn::emit: expected = 0x" << std::hex << wr_size;
            std::cout << " actual = 0x" << std::hex << (wr_size - delta) << std::endl;
            std::cout << "core_insn::emit: insn: " << fmt() << std::endl;
        }
#endif
#undef VALIDATE_EMIT

    }
    
////////////////////////////////////////////////////////////////////////
//
// declare `ostream<<` methods to stream formatted `core_insn`
//
// use os << insn.raw() &&
//     os << insn.fmt()
//
// to stream instructions
//
////////////////////////////////////////////////////////////////////////

private:
    struct print_t
    {
        core_insn& obj;
        void (core_insn::*fn)(std::ostream&);
    };
    
    // ostream formatted core_insn
    friend std::ostream& operator<<(std::ostream& os, print_t const& arg)
    {
        (arg.obj.*arg.fn)(os);
        return os;
    }

public:
    print_t raw() 
    {
        return { *this, &core_insn::raw };
    }

    print_t fmt() 
    {
        return { *this, &core_insn::fmt };
    }
    
////////////////////////////////////////////////////////////////////////
//
// instance data
//
////////////////////////////////////////////////////////////////////////

    opcode const&  op;
    opc::insn_data data;
    insn_container_data const *data_p{};
};
}

#endif
