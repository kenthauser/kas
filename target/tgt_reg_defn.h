#ifndef KAS_TARGET_TGT_REG_DEFN_H
#define KAS_TARGET_TGT_REG_DEFN_H

////////////////////////////////////////////////////////////////////////////
//
// constexpr definition of target register
//
////////////////////////////////////////////////////////////////////////////

#include "kas_core/kas_object.h"
#include "parser/sym_parser.h"
#include "utility/string_mpl.h"

namespace kas::tgt
{
// declare constexpr definition of register generated at compile-time
template <typename Reg_t>
struct tgt_reg_defn
{
    // pick up definition types from `Reg_t`
    using reg_name_idx_t = typename Reg_t::reg_name_idx_t;
    using reg_class_t    = typename Reg_t::reg_class_t;
    using reg_value_t    = typename Reg_t::reg_value_t;
    using hw_tst         = typename Reg_t::hw_tst;

    static constexpr auto MAX_REG_NAMES = Reg_t::MAX_REG_ALIASES + 1;

    // type definitions for `parser::sym_parser_t`
    // NB: ctor NAMES index list will include aliases
    using NAME_LIST = meta::list<meta::int_<0>>;

    template <typename...NAMES, typename N, typename REG_C, typename REG_V, typename REG_TST>
    constexpr tgt_reg_defn(meta::list<meta::list<NAMES...>
                                     , meta::list<N, REG_C, REG_V, REG_TST>>)
        : names     { (NAMES::value + 1)... }
        , reg_class { REG_C::value          }
        , reg_num   { REG_V::value          }
        , reg_tst   { REG_TST()             }
    {}

    static inline const char *const *names_base;

    // `n` is which alias (0 = canonical, non-zero = alias)
    const char *name(unsigned n = 0, unsigned i = 0) const noexcept
    {
        if (n < MAX_REG_NAMES)
            if (auto idx = names[n])
                return names_base[idx-1] + i;
        return {};
    }

    // each defn is 4 16-bit words (ie 64-bits) iff IDX_T is 8-bits & MAX_NAMES = 2
    reg_name_idx_t  names[MAX_REG_NAMES];
    reg_class_t     reg_class;              
    reg_value_t     reg_num;
    hw_tst          reg_tst;
  
    template <typename OS>
    friend OS& operator<<(OS& os, tgt_reg_defn const& d)
    {
        os << "reg_defn: " << std::hex;
        const char * prefix = "names= ";
        for (auto& name : d.names)
        {
            if (!name)
                break;
            os << prefix << +name;
            prefix = ",";
        }
        os << " class="  << +d.reg_class;
        os << " num="    << +d.reg_num;
        os << " tst="    <<  d.reg_tst;
        return os << std::endl;
    }
};


namespace detail
{
    using namespace meta;

    template <typename ALIASES>
    struct xlate
    {
        // unzip ALISES
        using alias_real_reg = transform<ALIASES, bind_back<quote<at>, int_<0>>>;
        using alias_new_name = transform<ALIASES, bind_back<quote<at>, int_<1>>>;

        template <typename NAMES>
        struct do_xlate
        {
            // when `quoted_trait` is `invoked` to supply NAMES
            using type = do_xlate;

            // do `find_index` for all `alias_to` types
            using xlate_to = concat<list<npos>, transform<alias_new_name
                                                        , bind_front<quote<find_index>
                                                        , front<NAMES>
                                                        >
                                    >>;
            
            template <typename NAME>
            using alias_index = at_c<xlate_to, find_index<alias_real_reg, NAME>::value + 1>;                    
            
            template <typename DEFN>
            using invoke = list<list<find_index<front<NAMES>, front<DEFN>>
                                   , alias_index<front<DEFN>>>
                              , DEFN
                              >;
        };

        template <typename NAMES>
        using invoke = do_xlate<NAMES>;
    };
}

template <typename Reg_t, typename ALIASES = meta::list<>>
struct tgt_reg_adder
{
    // declare member types for `parser::sym_parser_t`
    using OBJECT_T    = Reg_t;
    using VALUE_T     = OBJECT_T *;
    using DEFN_T      = typename Reg_t::defn_t;

    using CTOR        = detail::xlate<ALIASES>;
    //using NAME_LIST   = typename DEFN_T::NAME_LIST;
    using XTRA_NAMES  = typename CTOR::alias_new_name;
    
    using XLATE_LIST = meta::list<meta::list<const char *
                                           , typename DEFN_T::NAME_LIST
                                           , void
                                           , XTRA_NAMES
                                           >
                                >;


    template <typename PARSER>
    tgt_reg_adder(PARSER) : defns(PARSER::sym_defns)
    {
        OBJECT_T::set_insns(PARSER::sym_defns, PARSER::sym_defns_cnt);
        DEFN_T::names_base = meta::front<typename PARSER::all_types_defns>::value;
    }

    static inline std::deque<OBJECT_T> obstack;
    
    template <typename X3>
    void operator()(X3& x3, unsigned count)
    {
        auto p = defns;

        static std::deque<OBJECT_T> obstack;

        for (auto n = 0; n < count; ++n, ++p)
        {
            // for each defn, lookup canonical name in x3 string parser.
            // if not found, allocate an empty `tgt_reg` from obstack & register
            // names from defn with `x3` parser to return `tgt_reg` object.
            // finally, update `tgt_reg` object with defn.

            //std::cout << "reg_adder:" << *p;
            auto canonical = Reg_t::format_name(p->name());
            if (!x3.find(canonical))
            {
                // create instance (numbering instances from 1)
                auto obj_p = &OBJECT_T::add(canonical);

                // add all names
                for (unsigned i = 0; true; ++i)
                {
                    auto name = p->name(i);
                    if (!name)
                        break;
                    
                    //std::cout << "reg_adder: adding " << name << std::endl;
                    x3.add(Reg_t::format_name(name), obj_p);

                    // see if alternate names
                    name = Reg_t::format_name(name, 1); 
                    if (name)
                    {
                        //std::cout << "reg_adder: alternate " << name << std::endl;
                        x3.add(name, obj_p);
                    }
                }
            }
            //std::cout << "reg_adder: adding defn for: " << canonical << std::endl; 
            (*x3.find(canonical))->add_defn(*p, n);
        }
    }

    DEFN_T const * const defns;
};
}


#endif
