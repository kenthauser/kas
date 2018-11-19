#ifndef KAS_M68K_M68K_INSN_ADDER_H
#define KAS_M68K_M68K_INSN_ADDER_H

//////////////////////////////////////////////////////////////////////////////////////////

#include "m68k_arg_defn.h"
#include "m68k_insn_types.h"
#include "mit_moto_names.h"
#include "parser/sym_parser.h"

namespace kas::m68k::opc
{
using namespace meta;
using namespace hw;

struct m68k_insn_adder
{
    using DEFN_T     = m68k_insn_defn;
    using VALUE_T    = opc::m68k_insn_t *;
    using XLATE_LIST = typename DEFN_T::XLATE_LIST;    

    template <typename PARSER>
    m68k_insn_adder(PARSER) : defns { PARSER::sym_defns}
    {
        // get types from sym_parser
        using types_defns = typename PARSER::all_types_defns; 
        DEFN_T::names_base = at_c<types_defns, 0>::value;
        DEFN_T::sizes_base = at_c<types_defns, 1>::value;
        DEFN_T::fmts_base  = at_c<types_defns, 2>::value;

        // xlate VAL_C types as index into VAL array
        using VALS  = at_c<typename PARSER::all_types, 3>;
        using VAL_C = at_c<typename PARSER::all_types, 4>;
        using COMBO = transform<VAL_C
                              , bind_back<quote<transform>
                                        , bind_front<quote<find_index>, VALS>
                                        >
                              >;

        // Also store combo index in DEFN_T
        using kas::parser::detail::init_from_list;
        DEFN_T::val_c_base = init_from_list<const m68k_validate_args, COMBO>::value;
        
        // val list & names are stored in `combo` 
        using VAL_NAMES = transform<VALS, quote<front>>;
        m68k_validate_args::vals_base  = at_c<types_defns, 3>::value;
        m68k_validate_args::names_base = init_from_list<const char *, VAL_NAMES>::value;
    };

    template <typename X3>
    void operator()(X3& x3, unsigned count)
    {
        // initialize runtime objects from definitions
        //
        // For each defintion, several size variants are created.
        // Allocate 1 `m68k_opcode_t` object for each size variant
        //
        // For each `m68k_opcode_t` object, several "name" variants
        // can be created. For each "name", use X3 parser to lookup
        // (& allocate) `m68k_insn_t` instance. Then add `m68k_opcode_t`
        // instance pointer to `m68k_insn_t` instance.

        std::cout << "m68k_insn_adder::add()" << std::endl;

        // allocate run-time objects in deques
        auto insn_obstack   = new typename m68k_insn_t::obstack_t;
        m68k_insn_t::index_base = insn_obstack;

        auto opcode_obstack = new typename m68k_opcode_t::obstack_t;
        m68k_opcode_t::index_base = opcode_obstack;

        // store defns in m68k_opcode_t
        m68k_opcode_t::defns_base = defns;

        auto p = defns;
        for (int n = 0; n < count; ++p, ++n)
        {
            //std::cout << n << " base: " << p->name() << std::endl;

            auto& sz_obj = p->sizes_base[p->sz_index - 1];
            for (auto sz : sz_obj)
            {
                // XXX don't worry about validators: allocate all
                m68k_opcode_t *op_p {};

                op_p = &opcode_obstack->emplace_back(opcode_obstack->size()
                                                   , n, p, sz, sz_obj.single_size);
                
                // now for each "name", add pointer to opcode
                for (auto&& name : mit_moto_names(p->name(), sz_obj.suffixes(sz)))
                {
                    // lookup name. Inserts empty if not found
                    auto& insn_p = x3.at(name);

                    // if new, allocate insn
                    if (!insn_p)
                        insn_p = &insn_obstack->emplace_back(insn_obstack->size(), std::move(name));

                    // add opcode to insn
                    insn_p->opcodes.push_back(op_p);
                }
            }
        }

        std::cout << "m68k_insn_adder::end" << std::endl;
    }
    
    DEFN_T const *defns;
};
}
#endif

