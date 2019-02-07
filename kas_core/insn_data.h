#ifndef KAS_CORE_INSN_DATA_H
#define KAS_CORE_INSN_DATA_H

// Declare information used by opcodes.
// This information is 



#include "expr/expr.h"
#include "core_size.h"


namespace kas::core::opc
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

    struct insn_data 
    {
    private:
        static inline std::deque<expr_t> insn_expr_data; 

    
    public:
        // picked up in `struct opcode`
        using iter     = typename decltype(insn_expr_data)::iterator;
        using fixed_t  = insn_fixed_t<>;
        
        // XXX don't like this defn
        using Inserter = opc_back_insert_iterator<decltype(insn_expr_data)>;
        
        // this ctor used when generating insn data
        insn_data(parser::kas_loc loc) : 
                _opc_loc(loc)       // record location of insn
              , fixed(_opc_fixed)   // reference local instances
              , size(_opc_size)
        {
            // remember current status of data queue
            first = insn_expr_data.size();
        }

        // this ctor used when evaluating insn data
        insn_data(insn_container_data&);

        // get back-inserter for expression data when generating data
        auto& di() const
        {
            static auto _di = opc_back_inserter(insn_expr_data);

            if (data)
                throw std::logic_error{"back_inserter referenced during evaluation"};

            return _di;
        }

        // use signed type for opcode size
        // XXX why?
        using op_size_t   = offset_t<int16_t>;

        // get iter to expression data
        static auto begin()
        {
            return insn_expr_data.begin();
        }


        // XXX
        static void clear()
        {
            insn_expr_data.clear();
        }
#if 0
        // create back inserter
        auto back_inserter()
        {
            return Inserter(*this);
        }
#endif

        // XXX define after `insn_container_data` declared
        parser::kas_loc loc() const
        {
            return _opc_loc;
        }

        fixed_t&        fixed;
        op_size_t&      size;

        uint32_t        first;      // index of first expression
        uint16_t        cnt;        // expressions used by this insn
    private:
        // instances used during parsing of insns. 
        parser::kas_loc _opc_loc;
        fixed_t         _opc_fixed;
        op_size_t       _opc_size;

        insn_container_data *data {};

    };
}

#endif
