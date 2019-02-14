#ifndef KAS_Z80_Z80_INSN_ADDER_H
#define KAS_Z80_Z80_INSN_ADDER_H

//////////////////////////////////////////////////////////////////////////////////////////

#include "tgt_insn_defn.h"
#include "parser/sym_parser.h"

namespace kas::tgt::opc
{
using namespace meta;
//using namespace hw;

template <typename MCODE_T>
struct tgt_insn_adder
{
    using mcode_t    = MCODE_T;
    
    // types used by `tgt_insn_adder`
    using defn_t     = typename mcode_t::defn_t;
    using insn_t     = typename mcode_t::insn_t;
    using val_c_t    = typename mcode_t::val_c_t;
    
    // types interpreted by `sym_parser`
    using VALUE_T    = mcode_t const *;
    using XLATE_LIST = typename defn_t::XLATE_LIST;



    template <typename PARSER>
    tgt_insn_adder(PARSER) : defns {PARSER::sym_defns}
    {
        // get types from sym_parser
        using types_defns = typename PARSER::all_types_defns; 
        defn_t::names_base = at_c<types_defns, 0>::value;
        defn_t::fmts_base  = at_c<types_defns, 1>::value;

        // xlate VAL_C types as index into VAL array
        using VALS  = at_c<typename PARSER::all_types, 2>;
        using VAL_C = at_c<typename PARSER::all_types, 3>;
        using COMBO = transform<VAL_C
                              , bind_back<quote<transform>
                                        , bind_front<quote<find_index>, VALS>
                                        >
                              >;

        // Also store combo index in DEFN_T
        using kas::parser::detail::init_from_list;
        defn_t::val_c_base = init_from_list<const val_c_t, COMBO>::value;
        
        // val list & names are stored in `combo` 
        using VAL_NAMES = transform<VALS, quote<front>>;
        val_c_t::vals_base  = at_c<types_defns, 2>::value;
        val_c_t::names_base = init_from_list<const char *, VAL_NAMES>::value;

        //z80_validate_args::set_base(at_c<types_defns, 2>::value);
    }

    template <typename X3>
    void operator()(X3& x3, unsigned count)
    {
        // initialize runtime objects from definitions
        //
        // For each defintion, several size variants are created.
        // Allocate 1 `z80_opcode_t` object for each size variant
        //
        // For each `z80_opcode_t` object, several "name" variants
        // can be created. For each "name", use X3 parser to lookup
        // (& allocate) `z80_insn_t` instance. Then add `z80_opcode_t`
        // instance pointer to `z80_insn_t` instance.

        //std::cout << "z80_insn_adder::add()" << std::endl;

        // allocate run-time objects in deques
        auto insn_obstack   = new typename insn_t::obstack_t;
        insn_t::index_base  = insn_obstack;

        auto mcode_obstack  = new typename mcode_t::obstack_t;
        mcode_t::index_base = mcode_obstack;

        // store defns base in `mcode`
        mcode_t::defns_base = defns;

        auto p = defns;
        for (int n = 0; n < count; ++p, ++n)
        {
            //std::cout << n << " base: " << p->name() << std::endl;

            // XXX don't worry about hw validators: allocate all
            mcode_t *mcode_p {};

            // create the "mcode instance" 
            // NB: use "size()" for instance index
            mcode_p = &mcode_obstack->emplace_back(mcode_obstack->size(), n, p);
            auto&& name = p->name();

            // test for "list" opcode
            if (name[0] == '*')
            {
                insn_t::list_mcode_p = mcode_p;
                continue;
            }
                
            // lookup name. Inserts empty if not found
            auto& insn_p = x3.at(name);

            // if new, allocate insn
            if (!insn_p)
                insn_p = &insn_obstack->emplace_back(insn_obstack->size(), std::move(name));

            // add opcode to insn
            insn_p->opcodes.push_back(mcode_p);
            if (insn_p->opcodes.size() > insn_p->max_opcodes)
                throw std::logic_error("too many machine codes for " + std::string(name));
        }

        //std::cout << "z80_insn_adder::end" << std::endl;
    }

    defn_t const *defns;
};
}
#endif

