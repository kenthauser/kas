#ifndef KAS_CORE_OPCODE_H
#define KAS_CORE_OPCODE_H

///////////////////////////////////////////////////////////////////////////
//
//                          o p c o d e
//
///////////////////////////////////////////////////////////////////////////
//
// The `opcode` class provides one of the two parts of the `core_insn`.
// The second class is `insn_data`.
//
// The `opcode` provides the "methods" to operate on an associated instance
// of `insn_data`. In this way, `opcode` is much like an un-bound python object.
// 
///////////////////////////////////////////////////////////////////////////
//
// The `opcode` class is implemented as a virtual base class.
//
// XXX Describe OPC_INDEX()
//
///////////////////////////////////////////////////////////////////////////

#include "core_size.h"
#include "core_emit.h"
#include "opcode_data.h"
#include "core_fits.h"

#include "kas_object.h"
#include "utility/string_mpl.h"

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <deque>
#include <typeinfo>
#include <typeindex>



namespace kas::core::opc
{

using parser::kas_error_t;
using parser::kas_diag_t;

#define OPC_INDEX()   \
    uint16_t& opc_index() const override            \
    {  static uint16_t index_; return index_; }     \
    uint16_t opc_size() const override { return sizeof(*this); }

struct opcode
{
#if 0
    // all opcode configurable types are bundled into `DATA`
    using data_t     = DATA;
    using value_type = typename DATA::value_type;
    using Iter       = typename DATA::Iter;
    using Inserter   = typename DATA::Inserter;

    using fixed_t    = typename DATA::fixed_t;
    using op_size_t  = typename DATA::op_size_t;
#endif

    // pick up some types from `opcode_data`
    using data_t     = opcode_data;
    // XXX wrong place
    using op_size_t  = typename data_t::op_size_t; 
    using Iter       = typename data_t::Iter;

    virtual ~opcode() = default;

    // default call operator is no-op
    void operator()(data_t&) const
    {
    }
    
    // declare virtual functions for evaluating opcodes
    virtual const char *name() const
    {
        return typeid(*this).name();
    }

    // NB: the `Iter` args are passed by value to generate a local copy
    virtual op_size_t calc_size(opcode_data& data, core_fits const& fits) const
    {
        return data.size;
    }
    virtual void fmt(opcode_data const& data, std::ostream& out) const
    {}
    virtual void emit(opcode_data const& data, emit_base& base, core_expr_dot const *dot_p) const
    {}

private:
    // XXX Predates unbound opcode
    virtual uint16_t& opc_index() const = 0;
    virtual uint16_t  opc_size()  const = 0;

    static constexpr uint16_t V_OPCODE_SZ = 64;
    struct V_OPCODE
    {
        V_OPCODE(opcode const *p)
        {
            auto n = p->opc_size();
            if (n > V_OPCODE_SZ)
                throw std::length_error(p->name() +
                    std::string(" = ") + std::to_string(n));
            std::memcpy(data, (void *)p, n);
        }
        
        operator opcode *()
        {
            return reinterpret_cast<opcode *>(data);
        }

        uint8_t data[V_OPCODE_SZ];
    };
    
    static auto& opc_indexes ()
    {
        static auto _indexes = new std::deque<V_OPCODE>;
        return *_indexes;
    }

    static uint16_t add_index(opcode const *p)
    {
        auto& indexes = opc_indexes();
        indexes.emplace_back(p);
        return indexes.size();
    }
    // XXX
public:
    uint16_t index() const
    {
        auto& idx = opc_index();
        if (idx == 0)
            idx = add_index(this);
        return idx;
    }
    
    static auto& get(uint16_t n)
    {
        return *opc_indexes()[n - 1];
    }

    // if no arguments -- presumably result stored in fixed.
    void proc_args(opcode_data&) {}

    // routine for test runner
    void raw(opcode_data const& data, std::ostream& out) const
    {
        out << std::hex << data.fixed.fixed;
        auto iter = data.iter();        // NB: `iter` must be called before...
        auto cnt  = data.cnt;           // ...accessing `cnt`: see `insn_defn.h`
        while(cnt--)
            out << ' ' << *iter++;
    }

    // declare methods for creating opcodes
    template <typename C> 
    kas_error_t validate(C&) { return {}; }

    template <typename C>
    static kas_error_t validate_min_max(C&, uint16_t = 0, uint16_t = ~0);

    void make_error(opcode_data& data, parser::kas_error_t err)
    {
        data.fixed.diag = err;
        data.size.set_error();
    }

    // convenience method: pass msg & `loc`
    void make_error(opcode_data& data, std::string&& msg, kas::parser::kas_loc const& loc)
    {
        return make_error(data, kas_diag_t::error(std::move(msg), loc));
    }

    template <typename REF, typename = std::enable_if_t<std::is_base_of_v<ref_loc_tag, REF>>>
    void make_error(opcode_data& data, const char *msg, REF const& ref)
    {
        return make_error(data, msg, *ref.template get_p<kas::parser::kas_loc>());
    }

public:
    static inline std::ostream *trace;
};

template <typename C>
kas_error_t opcode::validate_min_max(C& c, uint16_t min, uint16_t max)
{
    // single arg can be "missing" or real arg.
    // clear container if "missing"
    if (c.size() == 1)
        if (c.front().is_missing())
        {
            if (min == 1)
                return kas_diag_t::error("Requires argument").ref(c.front());
            else if (!min) {
                c.clear();
                return {};
            }
        }

    if (c.size() < min)
        return kas_diag_t::error("Too few arguments").ref(*std::prev(c.end()));

    if (c.size() > max) {
        auto last = c.begin();
        std::advance(last, max);
        return kas_diag_t::error("Too many arguments").ref(*last);
    }

    return {};
}

template <typename NAME = KAS_STRING("NOP")>
struct opc_nop : opcode
{
    OPC_INDEX();
    const char *name() const override
    {
        return NAME::value;
    }
};

struct opc_error : opcode
{
    OPC_INDEX();
    const char *name() const override
    {
        return "ERROR";
    }

    void proc_args(opcode_data& data, kas::parser::kas_error_t diag)
    {
        data.fixed.diag = diag;
    }

    void fmt(opcode_data const& data, std::ostream& out) const override
    {
        auto& fixed = data.fixed;

        out << name() << ": ";
        out << (fixed.diag ? fixed.diag.get().message : "[[ Zero Errno ]]");
    }

    void emit(opcode_data const& data, emit_base& emit, core_expr_dot const *dot_p) const override
    {
        auto& fixed = data.fixed;
        
        if (fixed.diag)
            emit <<  set_size(data.size()) << fixed.diag.get();
    }
};
}

// copy "opcode" into kas::core

namespace kas::core
{
    using opc::opcode;
}

#endif
