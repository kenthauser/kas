#ifndef KAS_M68K_M68K_REG_ADDER_H
#define KAS_M68K_M68K_REG_ADDER_H

#include "m68k_reg_defns.h"
#include "m68k_options.h"
#include "parser/sym_parser.h"

namespace kas::m68k::defn
{
using namespace meta;

using ALIASES    = defn::m68k_reg_aliases_l;
using reg_names  = transform<defn::m68k_all_reg_l, bind_back<quote<at>, int_<0>>>;

template <typename ALIASES>
struct xlate
{
    // unzip ALISES
    using alias_from = transform<ALIASES, bind_back<quote<at>, int_<0>>>;
    using alias_to   = transform<ALIASES, bind_back<quote<at>, int_<1>>>;

    template <typename NAMES>
    struct do_xlate
    {
        // when `quoted_trait` is `invoked` to supply NAMES
        using type = do_xlate;

        // do `find_index` for all `alias_to` types
        using xlate_to = concat<list<npos>, transform<alias_to, bind_front<quote<find_index>, front<NAMES>>>>;
        
        template <typename NAME>
        using alias_index = at_c<xlate_to, find_index<alias_from, NAME>::value + 1>;                    
        
        template <typename DEFN>
        using invoke = list<list<find_index<front<NAMES>, front<DEFN>>, alias_index<front<DEFN>>>
                          , DEFN
                          >;

    };

    template <typename NAMES>
    using invoke = do_xlate<NAMES>;
    
};

template <typename ALIASES>
struct reg_adder
{
    // declare member types for `parser::sym_parser_t`
    using DEFN_T      = m68k_reg_defn;
    using OBJECT_T    = m68k_reg;
    using VALUE_T     = OBJECT_T *;

    using CTOR        = xlate<ALIASES>;
    //using NAME_LIST   = typename DEFN_T::NAME_LIST;
    using XTRA_NAMES  = typename CTOR::alias_to;
    
    using XLATE_LIST = list<list<const char *, typename DEFN_T::NAME_LIST, void, XTRA_NAMES>>;


    template <typename PARSER>
    reg_adder(PARSER) : defns(PARSER::sym_defns)
    {
        using kas::parser::detail::init_from_list;
        OBJECT_T::set_insns(PARSER::sym_defns, PARSER::sym_defns_cnt);
        DEFN_T::names_base = front<typename PARSER::all_types_defns>::value;
#if 0
        // XLATE_LIST meta-fn doesn't work properly
        auto p = DEFN_T::names_base;
        auto n = front<typename PARSER::all_types_defns>::size;
        std::cout << "reg_names: ";
        while (n--)
            std::cout << *p++ << ", ";
        std::cout << std::endl;

        print_type_name{"reg: XTRA"}.name<XTRA_NAMES>();
        print_type_name{"reg: XLATE_LIST"}.name<XLATE_LIST>();
#endif
    }

    enum reg_prefix { PFX_NONE, PFX_ALLOW, PFX_REQUIRE };
    static inline std::deque<OBJECT_T> obstack;
    
    template <typename X3>
    void operator()(X3& x3, unsigned count)
    {
        auto p = defns;

        static std::deque<OBJECT_T> obstack;

        for (auto n = 0; n < count; ++n, ++p)
        {
            //std::cout << "reg_adder:" << *p;
            auto canonical = p->name();
            if (!x3.find(canonical))
            {
                // create instance
                auto obj_p = &obstack.emplace_back();

                // add all names
                for (unsigned i = 0; true; ++i)
                {
                    auto name = p->name(i);
                    if (!name)
                        break;
                    
                    //std::cout << "reg_adder: adding " << name << std::endl;
                    x3.add(name, obj_p);

                    // see if alternate name
                    name = p->name(i, true); 
                    if (name)
                    {
                        //std::cout << "reg_adder: alternate " << name << std::endl;
                        x3.add(name, obj_p);
                    }
                }
            }
            x3.at(canonical)->add(*p, n);
        }
    }

    m68k_reg_defn const * const defns;
};

static const auto reg_parser = kas::parser::sym_parser_t<m68k_reg_defn
                                                  , defn::m68k_all_reg_l
                                                  , reg_adder<defn::m68k_reg_aliases_l>
                                                  >();

}


#endif
