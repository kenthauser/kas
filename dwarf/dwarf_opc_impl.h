#ifndef KAS_DWARF_DWARF_OPC_IMPL_H
#define KAS_DWARF_DWARF_OPC_IMPL_H

#include "dwarf_opc.h"          // has base template declaration
#include "kas_core/core_insn.h"
#include "kas/kas_string.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
#include "kas_core/core_insn.h"
#include "kas_core/core_fixed_inserter.h"
#include "utility/print_type_name.h"

#include <typeinfo>

//
// Generate a stream of instructions given the `insn inserter`
//
// The `insn_inserter` requires instances of "core_insn", but impliciate
// conversion from `opcode` is provided via `core_insn` ctor.
//
// /gen_

namespace kas::dwarf
{


using namespace kas::core::opc;

struct dwarf_emit_fixed
{
    using data_t     = core::opc::opcode::data_t;
    using rt_size_t  = unsigned short;
    
    constexpr dwarf_emit_fixed() {}
    virtual  ~dwarf_emit_fixed() {}
    
    // return pointer to static instance
    virtual dwarf_emit_fixed const *get_p() const = 0;
    
    virtual rt_size_t operator()(void *, e_fixed_t arg) const
    {
        print_type_name{"e_fixed_t"}.name<decltype(arg)>();
        throw std::logic_error {"argument type not supported"};
    }
    virtual rt_size_t operator()(void *, const char *arg) const
    {
        print_type_name{"const char *"}.name<decltype(arg)>();
        throw std::logic_error {"argument type not supported"};
    }
    virtual rt_size_t operator()(void *, expr_t arg) const
    {
        print_type_name{"expr_t&&"}.name<decltype(arg)>();
        throw std::logic_error {"argument type not supported"};
    }

    virtual bool is_same(dwarf_emit_fixed const &o) const
    {
        return o.get_opc_index() == get_opc_index();
    }

    // must define "opcode" type
    virtual uint16_t get_opc_index() const = 0;

    // manage fixed data inserter  
    virtual void *make_inserter(data_t&)  const { return {}; }
    virtual void delete_inserter(void *) const {}
};

// declare argument formats
template <typename NAME, typename T, typename OP>
struct ARG_defn : dwarf_emit_fixed
{
    using value_t = T;
    using op      = OP;
    using data_t  = core::opc::opcode::data_t;

    // type of `back_inserter`
    using di_t = decltype(std::declval<OP>()
                               .gen_inserter(std::declval<data_t&>()));

    static constexpr const char *value = NAME::value;
    static constexpr auto        size  = sizeof(T);

    operator const char *() const { return value; }

    // manage ARG_defn: `get_p`, `is_same`
    dwarf_emit_fixed const *get_p() const override
    {
        static const ARG_defn d;
        static_assert (sizeof(d) == sizeof(void *)
                     , "ARG_defn can't have instance data");
        return &d;
    }

    bool is_same(dwarf_emit_fixed const& o) const override
    {
        return get_opc_index() == o.get_opc_index();
    }

    uint16_t get_opc_index() const override
    {
        return OP().index();
    }
   
    template <typename ARG_T>
    auto proc_one (void *v, ARG_T&& arg, std::true_type) const
     -> decltype(OP::proc_one(std::declval<di_t&>()
               , std::declval<kas_position_tagged>()
               , std::forward<ARG_T>(arg)))
    {
        print_type_name{"proc_one: ARG_T"}.name<ARG_T>();
        auto& di = *static_cast<di_t *>(v);
        kas_position_tagged loc;

        using RT = decltype(OP::proc_one(di, loc, std::forward<ARG_T>(arg)));
        //print_type_name{"proc_one: RT"}.name<RT>();
        if constexpr (!std::is_void_v<RT>)
            return OP::proc_one(*static_cast<di_t *>(v)
                              , loc
                              , std::forward<ARG_T>(arg));
        else
        {
            std::cout << "proc_one: dummy" << std::endl;
            return {};
        }
    }

    template <typename ARG_T, typename...Ts> 
    static core::offset_t<rt_size_t> proc_one(void *, ARG_T const&, Ts&&...)
    {
        throw std::logic_error {"proc_one"};
    }
    auto operator()(void *di_p, e_fixed_t arg) const 
        -> decltype(proc_one(nullptr, std::move(arg), std::true_type())())
        override
    {
        return proc_one(di_p, std::move(arg), std::true_type())();
    }
    rt_size_t operator()(void *di_p, const char *arg) const override
    {
        return proc_one(di_p, arg, std::true_type())();
    }
    rt_size_t
    operator()(void *di_p, expr_t arg) const override
    {
        return proc_one(di_p, std::move(arg), std::true_type())();
    }
    // manage `data_inserter` object
    void *make_inserter(data_t& d) const override
    {
        return new di_t {OP::gen_inserter(d)};
    }
    void delete_inserter(void *di) const override
    {
        delete static_cast<di_t *>(di);
    }

};
}

#endif
