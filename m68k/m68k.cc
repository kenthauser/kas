// instantiate m68k instruction parser.
#if 0
#include "m68k.h"
#include "m68k_options.h"
//#include "m68k_arg_size.h"
#include "mit_moto_parser_def.h"

#include "mit_moto_names.h"
#include "mit_arg_ostream.h"
#include "moto_arg_ostream.h"

#include "m68k_insn_impl.h"
#include "m68k_insn_validate.h"
#include "m68k_insn_types.h"

#include "m68k_arg_impl.h"

#include "m68k_stmt_opcode.h"
//#include "m68k_stmt_impl.h"
#include "target/tgt_stmt_impl.h"
#include "target/tgt_insn_impl.h"
#include "target/tgt_insn_eval.h"
#endif

#if 0
namespace kas::m68k::hw
{
    cpu_defs_t cpu_defs;
}
#endif

namespace kas::m68k
{
#if 0
    namespace parser
    {
        using kas::parser::iterator_type;
        using kas::parser::context_type;

        BOOST_SPIRIT_INSTANTIATE(m68k_stmt_x3, iterator_type, context_type)
    }
    
    // print m68k_arg_t in canonical format (MIT or MOTOROLA)
    std::ostream& operator<<(std::ostream& os, m68k_arg_t const& arg)
    {
        static bool is_mit_canonical = m68k_options.mit_canonical &&
                                       m68k_options.gen_mit;

        if (is_mit_canonical)
            return mit_arg_ostream(os, arg);
        return moto_arg_ostream(os, arg);
    }
#endif
}


#if 0
namespace kas::m68k::opc
{
    // static methods declared inside incomplete types
    m68k_insn_t const& m68k_insn_t::get(uint16_t idx)
    {
        return (*index_base)[idx];
    }

    // static methods declared inside incomplete types
    m68k_opcode_t const& m68k_opcode_t::get(uint16_t idx)
    {
        return (*index_base)[idx];
    }

}
#endif

namespace kas::m68k::opc::dev
{
#if 0

// extend `meta::at`: if out-of-range, return void
template <typename DEFN>
struct get_one_type_index
{
    template <typename INDEX>
    using invoke = _t<if_c<INDEX::value < DEFN::size()
                         , lazy::at<DEFN, INDEX>
                         , id<void>
                         >>;
};

template <typename TYPE_LIST, typename DEFN>
using get_type_list = filter<transform<TYPE_LIST, get_one_type_index<DEFN>>
                           , not_fn<quote<std::is_void>>
                           >;

template <typename TYPE_LIST, typename FN>
struct get_one_insn_types
{
    template <typename DEFN>
    using invoke = _t<invoke<FN, get_type_list<TYPE_LIST, DEFN>>>;
};


template <typename DEFNS, typename TYPE_LIST, typename FN>
using get_all_insn_types = invoke<quote<join>, transform<DEFNS, get_one_insn_types<TYPE_LIST, FN>>>;

// callable to extract type list for one item
template <typename DEFNS>
struct get_types_one_item
{

    template <typename RT, typename TYPE_LIST, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = unique<concat<get_all_insn_types<DEFNS, TYPE_LIST, FN>, XTRA>>;
};


template <typename XLATE_LIST, typename DEFNS>
using get_all_types = transform<XLATE_LIST, uncurry<get_types_one_item<DEFNS>>>;

template <typename DEFNS, typename TYPES>
struct xlate_one_rule_impl
{
    template <typename TYPE_LIST, typename FN>
    struct xlate_one_defn
    {
        using xlate_types = bind_front<quote<find_index>, TYPES>;

        template <typename DEFN>
        using source_types = invoke<get_one_insn_types<TYPE_LIST, FN>, DEFN>;

        template <typename DEFN>
        using invoke = transform<source_types<DEFN>, xlate_types>;
    };

    template <typename RT, typename TYPE_LIST, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = transform<DEFNS, xlate_one_defn<TYPE_LIST, FN>>;
};


template <typename DEFNS>
struct xlate_one_rule:
{
    template <typename RULE, typename TYPES>
    using invoke = apply<xlate_one_rule_impl<DEFNS, TYPES>, RULE>;
};

/////////////////////////////////////


template <typename DEFN, typename TYPE>
struct X_xlate_one_rule
{
    template <typename RT, typename TYPE_LIST, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = transform<invoke<get_one_insn_types<TYPE_LIST, FN>, DEFN>
                            , bind_front<quote<find_index>, TYPE>>;
};

template <typename DEFN>
struct X_xlate_one_defn_impl
{
    template <typename RULE, typename TYPE>
    using invoke = apply<X_xlate_one_rule<DEFN, TYPE>, RULE>;
};


template <typename X_LIST, typename X_TYPES>
struct X_xlate_one_defn
{
    template <typename DEFN>
    using invoke = transform<X_LIST, X_TYPES, X_xlate_one_defn_impl<DEFN>>;
};
#endif
}

