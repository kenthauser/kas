#ifndef KAS_Z80_Z80_INSN_ADDER_H
#define KAS_Z80_Z80_INSN_ADDER_H

//////////////////////////////////////////////////////////////////////////////////////////

#include "z80_arg_defn.h"
#include "z80_insn_types.h"
#include "parser/sym_parser.h"

namespace kas::z80::opc
{
using namespace meta;
using namespace hw;

struct z80_insn_adder
{
    using DEFN_T     = z80_insn_defn;
    using VALUE_T    = opc::z80_insn_t *;
    using XLATE_LIST = typename DEFN_T::XLATE_LIST;    

    template <typename PARSER>
    z80_insn_adder(PARSER) : defns { PARSER::sym_defns}
    {
        // get types from sym_parser
        using types_defns = typename PARSER::all_types_defns; 
        DEFN_T::names_base = at_c<types_defns, 0>::value;
        DEFN_T::fmts_base  = at_c<types_defns, 1>::value;

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
        DEFN_T::val_c_base = init_from_list<const z80_validate_args, COMBO>::value;
        
        // val list & names are stored in `combo` 
        using VAL_NAMES = transform<VALS, quote<front>>;
        z80_validate_args::vals_base  = at_c<types_defns, 2>::value;
        z80_validate_args::names_base = init_from_list<const char *, VAL_NAMES>::value;

        //z80_validate_args::set_base(at_c<types_defns, 2>::value);
    };

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
        auto insn_obstack   = new typename z80_insn_t::obstack_t;
        z80_insn_t::index_base = insn_obstack;

        auto opcode_obstack = new typename z80_opcode_t::obstack_t;
        z80_opcode_t::index_base = opcode_obstack;

        // store defns in z80_opcode_t
        z80_opcode_t::defns_base = defns;

        auto p = defns;
        for (int n = 0; n < count; ++p, ++n)
        {
            //std::cout << n << " base: " << p->name() << std::endl;

            // XXX don't worry about validators: allocate all
            z80_opcode_t *op_p {};

            // create the "opcode"
            op_p = &opcode_obstack->emplace_back(opcode_obstack->size(), n, p);
            auto&& name = p->name();

            // test for "list" opcode
            if (name[0] == '*')
            {
                z80_insn_t::list_opcode = op_p;
                continue;
            }
                
            // lookup name. Inserts empty if not found
            auto& insn_p = x3.at(name);

            // if new, allocate insn
            if (!insn_p)
                insn_p = &insn_obstack->emplace_back(insn_obstack->size(), std::move(name));

            // add opcode to insn
            insn_p->opcodes.push_back(op_p);
            if (insn_p->opcodes.size() > insn_p->max_opcodes)
                throw std::logic_error("too many machine codes for " + std::string(name));
        }

        //std::cout << "z80_insn_adder::end" << std::endl;
    }
    
    DEFN_T const *defns;
};
}
#endif

