#ifndef KAS_CORE_INSN_DATA_H
#define KAS_CORE_INSN_DATA_H

// Declare information used by opcodes.
// This information is 



#include "expr/expr.h"
#include "core_size.h"


namespace kas::core
{
namespace opc
{
    // an "pseudo-anonymous" union which holds several `core_types`
    template <typename FIXED_T = uint32_t>
    union insn_fixed_t
    {
        using fixed_t = FIXED_T;

        // access as fixed value or array of smaller types
        fixed_t    fixed;
        //uint16_t    fixed_p[2];
        uint8_t     data[sizeof(fixed_t)];

        // access as `core` types
        addr_ref    addr;
        symbol_ref  sym;
        parser::kas_error_t diag;
        parser::kas_loc loc;
        addr_offset_t offset;

        insn_fixed_t(fixed_t fixed = {}) : fixed(fixed) {};

        template <typename T>
        auto begin() const { return static_cast<T *>(data); }
        
        template <typename T>
        auto begin()       { return static_cast<T *>(data); }

        template <typename T>
        constexpr auto size() const  { return sizeof(data)/sizeof(T); }
    };

    // using fixed_t = insn_fixed_t;

    // define a `back_inserter` which adds `last` method to return reference
    // to last stored item (eg: back)
    template <typename Container>
    struct opc_back_insert_iterator : std::back_insert_iterator<Container>
    {
    private:
        Container& c;

    public:
        using value_type = typename Container::value_type;
        using reference  = typename Container::reference;

        opc_back_insert_iterator(Container &c)
            : c(c), std::back_insert_iterator<Container>(c) {}

        auto& last() { return c.back(); }
    };

    template <typename Container>
    auto opc_back_inserter(Container& c)
    {
        return opc_back_insert_iterator<Container>(c);
    }
}


struct insn_data 
{
private:
    static inline std::deque<expr_t> insn_expr_data; 


public:
    // types picked up by `struct opcode`
    using Iter     = typename decltype(insn_expr_data)::iterator;
    using fixed_t  = opc::insn_fixed_t<>;
    using Inserter = opc::opc_back_insert_iterator<decltype(insn_expr_data)>;
  
    
    // this ctor used when generating insn data
    insn_data(parser::kas_loc loc = {}) : loc(loc), fixed(_fixed)
    {
        // remember current status of data queue
        first = insn_expr_data.size();
    }

    // this ctor used when evaluating container data
    insn_data(insn_container_data&);

    // get back-inserter for expression data when generating data
    auto& di() const
    {
        static auto _di = opc::opc_back_inserter(insn_expr_data);
        return _di;
    }

    // get iter to expression data
    static auto begin()
    {
        return insn_expr_data.begin();
    }

    // return object, not reference
    Iter iter() const;
    std::size_t index() const;
    
    // use signed type for opcode size
    // XXX why?
    using op_size_t   = offset_t<int16_t>;

    // fixed is a `local` or `container_data` reference
    fixed_t&         fixed;

    // size always needed & thus calculated
    op_size_t        size;
    parser::kas_loc  loc;

    uint32_t         first;      // index of first expression
    
    // cnt is mutable to allow it to be set when calculating local ITER
    // NB: This only happens when printing INSN before storing in container.
    // NB: Since typically "raw" is run before "fmt", retrieving `iter` before 
    // `cnt` allows `cnt` to be retrieved w/o getter
    mutable uint16_t cnt;        // expressions used by this insn

//private:
    friend insn_container_data;
   
    uint16_t        raw_cnt;
    fixed_t         _fixed;
    
    // for test fixture
    static void clear()
    {
        insn_expr_data.clear();
    }


    // instances used during parsing of insns. 
    insn_container_data *data_p {};

    static inline core::kas_clear _c{clear};
};
}

#endif
