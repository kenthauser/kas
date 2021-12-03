#ifndef KAS_CORE_OPCODE_DATA_H
#define KAS_CORE_OPCODE_DATA_H

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
    union opcode_fixed_t
    {
        using fixed_t = FIXED_T;

        // access as fixed value or array of smaller types
        fixed_t     fixed;
        uint8_t     data[sizeof(fixed_t)];

        // access as `core` types
        addr_ref    addr;
        symbol_ref  sym;
        parser::kas_error_t diag;
        parser::kas_loc loc;
        addr_offset_t offset;

        opcode_fixed_t(fixed_t fixed = {}) : fixed(fixed) {};

        template <typename T>
        auto begin()
        { 
            void *v = data;
            return static_cast<T *>(v);
        }
       
        // once more, with const...
        template <typename T>
        auto const begin() const
        { 
            const_cast<opcode_fixed_t&>(*this).begin<T>();
        }

        template <typename T>
        constexpr auto size() const  { return sizeof(data)/sizeof(T); }
    };

    // using fixed_t = opcode_fixed_t;

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

// forward declare `insn_container_data` and inserter
struct insn_container_data;
template <typename DATA> struct insn_inserter;
using insn_inserter_t = insn_inserter<insn_container_data>;


struct opcode_data 
{
//private:
    // allow `insn_container` direct access to `expr_data` queue
    friend insn_container_data;
    static inline std::deque<expr_t> opcode_expr_data; 

public:
    // types picked up by `struct opcode`
    using Iter     = typename decltype(opcode_expr_data)::iterator;
    using fixed_t  = opc::opcode_fixed_t<>;
    using Inserter = opc::opc_back_insert_iterator<decltype(opcode_expr_data)>;
  
    
    // this ctor used when generating insn data
    opcode_data(parser::kas_loc loc = {}) : loc(loc), fixed(_fixed)
    {
        // remember current status of data queue
        first = opcode_expr_data.size();
    }

    // this ctor used when evaluating container data
    opcode_data(insn_container_data&);

    // get back-inserter for expression data when generating data
    auto& di() const
    {
        static auto _di = opc::opc_back_inserter(opcode_expr_data);
        return _di;
    }

    // get iter to expression data
    static auto begin()
    {
        return opcode_expr_data.begin();
    }

    static auto end()
    {
        return opcode_expr_data.size();
    }

    // set insn to `error`
    void set_error(e_diag_t const&);
    void set_error(const char *msg);
    
    // return object, not reference
    Iter iter() const;
    std::size_t index() const;
    
    // use unsigned type for opcode size
    using op_size_t   = offset_t<uint16_t>;

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
   
    uint16_t        raw_cnt{};
    fixed_t         _fixed {};

    // instances used during parsing of insns. 
    insn_container_data *data_p {};

    // for test fixture
    static void clear()
    {
        opcode_expr_data.clear();
    }
    // test fixture routine
    static inline core::kas_clear _c{clear};
};
}

#endif
