#ifndef KAS_PARSER_SYM_PARSER_DETAIL_H
#define KAS_PARSER_SYM_PARSER_DETAIL_H

//#include "sym_parser_xlate.h"

namespace kas::parser::detail
{
    using namespace meta;
    using namespace meta::placeholders;
    
    // ADDER adds instances to string parser
    // default: simple adder: just add insn* using name member variable
    template <typename T, typename = void>
    struct adder
    {
        // parser return type
        using VALUE_T   = T const *;

        template <typename PARSER>
        adder(PARSER) : insns(PARSER::sym_defns) {}

        template <typename X3>
        void operator()(X3& x3, unsigned count) const
        {
            auto& add = x3.add;
            for (auto p = insns; count--; ++p)
                add(p->name, p);
        }

        const T *insns;
    };

    // if T has ADDER member type, use as ADDER
    template <typename T>
    struct adder<T, std::void_t<typename T::ADDER>> : T::ADDER {};
    
#if 1
    template <typename T, typename = void>
    struct name_list_impl
    {
        // default: no xlate
        using type = list<>;
    };

    // if `T` has NAME_LIST member type, use it
    template <typename T>
    struct name_list_impl<T, std::void_t<typename T::NAME_LIST>>
    {
        using type = list<const char *, typename T::NAME_LIST>;
    };

    template <typename T>
    using name_list = _t<name_list_impl<T>>;
#else
    // XXX why doesn't this work???
    // default NAME_LIST to an empty list
    template <typename T, typename = void>
    struct name_list : list<> {};

    // if `T` has NAME_LIST member type, use it
    template <typename T>
    struct name_list<T, std::void_t<typename T::NAME_LIST>>
                : list<const char *, typename T::NAME_LIST> {};
#endif

// extract or generate `xlate_list`
//
// 1) ADDER::XLATE_LIST exists, use this
//
// 2) if T::NAME_LIST exists and is not empty, use this to 
//    generate XLATE_LIST with no special features, only NAMES to xlate.
//    The generated list is format `meta::list<NAME_LIST>>`
//
// 3) default to `list<>`

#if 1
    // default to no xlate
    template <typename ADDER, typename = void, typename = void>
    struct xlate_list_impl : id<list<>> {};
   
    // prefer `XLATE_LIST` member type if present
    template <typename ADDER, typename T>
    struct xlate_list_impl<ADDER, T, std::void_t<typename ADDER::XLATE_LIST>>
    {
        using type = typename ADDER::XLATE_LIST;
    };

    // otherwise use `name_list<T>` if not empty
    template <typename ADDER>
    struct xlate_list_impl<ADDER, std::void_t<at_c<name_list<ADDER>, 0>>, void>
    {
        using type = list<name_list<ADDER>>;
    };

    template <typename ADDER>
    using xlate_list = _t<xlate_list_impl<ADDER>>;
#else
    // default to no xlate
    template <typename T, typename = void, typename = void>
    struct xlate_list_impl
    {
        using type = list<>;
    };
   
    // prefer `XLATE_LIST` member type if present
    template <typename T, typename U>
    struct xlate_list_impl<T, U, std::void_t<typename T::XLATE_LIST>>
    {
        using type = typename T::XLATE_LIST;
    };

    // otherwise use `name_list<T>` if not empty
    template <typename T>
    struct xlate_list_impl<T, std::void_t<at_c<name_list<T>, 0>>, void>
    {
        using type = list<name_list<T>>;
    };

    template <typename T>
    using xlate_list = _t<xlate_list_impl<T>>;
#endif
}

#endif

