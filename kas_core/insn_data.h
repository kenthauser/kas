#ifndef KAS_CORE_INSN_DATA_H
#define KAS_CORE_INSN_DATA_H


namespace kas::core::opc
{
    // an "pseudo-anonymous" union which favors first member type.
    union insn_fixed_t {
        uint32_t    fixed;
        uint16_t    fixed_p[2];
        addr_ref    addr;
        symbol_ref  sym;
        parser::kas_error_t diag;
        parser::kas_loc loc;
        addr_offset_t offset;

        insn_fixed_t(decltype(fixed) fixed = {}) : fixed(fixed) {};
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

    template <typename Container>
    struct insn_data : protected Container
    {
        using value_type  = typename Container::value_type;
        using Iter        = typename Container::iterator;
        using size_type   = typename Container::size_type;
        using Inserter    = opc_back_insert_iterator<Container>;

        using fixed_t     = insn_fixed_t;
        using opc_index_t = uint16_t;

        // use signed type for opcode size
        // XXX why?
        using op_size_t   = offset_t<int16_t>;

        using Container::begin;
        using Container::end;
        using Container::size;
        using Container::operator[];

        using Container::clear;
        
        // create back inserter
        auto back_inserter()
        {
            return Inserter(*this);
        }
    };
}

#endif
