#ifndef KAS_EXPR_OP_TYPES_H
#define KAS_EXPR_OP_TYPES_H

// Declare expression `operators` compile-time defintions
// and run-time objects.
//
// These definitions are created by `sym_parser`

#include "expr_op_eval.h"
#include "precedence.h"

#include <unordered_map>

namespace kas::expression
{

using kas::parser::kas_position_tagged;

namespace detail
{
    using namespace meta;
    // the expression operator definition
    // holds declared values & all derived implementation data

    // forward declare `sym_parser_t` helper function
    struct expr_op_adder;

    struct expr_op_defn
    {
        // declare types used by implementation
        using EVAL      = expr_op_eval;
        using op_map    = std::unordered_map<HASH_T, EVAL>;
        using op_data_t = std::pair<HASH_T, EVAL>;
        
        // for `sym_parser_t`: ALIAS adder
        using ADDER     = expr_op_adder;

        // allow 1 alias
        using NAME_LIST = list<int_<3>, int_<5>>;
        
        // need default ctor because no sfx operators declared.
        expr_op_defn() = default;
        
        template <typename ARITY
                , typename PREC
                , typename IS_DIVIDE
                , typename NAME
                , typename OP
                , typename...SYMs
                , typename...NAME_INDEX
                >
         constexpr expr_op_defn(meta::list<meta::list<NAME_INDEX...>
                                         , meta::list<ARITY,PREC,IS_DIVIDE,NAME,OP,SYMs...>
                                         >)
                : arity      (ARITY::value)
                , prec       (PREC::value)
                , is_divide  (IS_DIVIDE::value)
                , op_p       {expr_op_fns<op_data_t, EVAL, OP, ARITY>::value}
                , op_cnt     {expr_op_fns<op_data_t, EVAL, OP, ARITY>::size }
                , name_index { (NAME_INDEX::value + 1)... }
                {}
      
        // allow max 1 alias per `expr_op`
        static constexpr auto MAX_NAMES = NAME_LIST::size();
        static inline const char * const * op_names;

        // default: return first name (which is, by defintion, canonical)
        const char *name(unsigned n = 0) const noexcept
        {
            if (n < MAX_NAMES)
                if (auto idx = name_index[n])
                    return op_names[idx-1];
            return {};
        }

        // reduce entry to 2 quads.
        op_data_t const *op_p;
        uint8_t          name_index[MAX_NAMES];
        uint8_t          op_cnt;
        uint8_t          arity;
        uint8_t          prec;
        uint8_t          is_divide;
    };


    struct expr_op
    {
        using DEFN    = expr_op_defn;
        using EVAL    = expr_op_eval;
        using op_map  = typename DEFN::op_map;
        using prec_t  = typename precedence::prec_t;
        using prec_fn = prec_t(*)(prec_t);

        // NB: all ARITY limitations are in `expr_op_eval` 
        // NB: `MAX_ARITY` is referenced in `oper_op_visitor`
        static constexpr auto MAX_ARITY = EVAL::MAX_ARITY;

        // construct & initialize
        expr_op(expr_op_defn const& defn)
                : defn_p(&defn)
                , ops(defn.op_p, defn.op_p + defn.op_cnt)
                {};

        // evaluate: forward to visitor
        template <typename...Ts>
        parser::kas_token operator()(kas_position_tagged const& loc, Ts&&...args) const noexcept;

        // evalutate: money function
        template <std::size_t N>
        parser::kas_token eval(kas_position_tagged const& op_loc
                   , parser::kas_token const* const* tokens
                   , HASH_T hash
                   , EVAL::exec_arg_t<N>&& args
                   , std::size_t err        // index of first "kas_diag" arg
                   , std::size_t dem        // index of expr_type of denominator
                   ) const noexcept;

        static auto set_prec_fn(prec_fn fn)
        {
            auto old = pri2prec;
            pri2prec = fn;
            return old;
        }

        prec_t priority() const noexcept
        {
            return pri2prec(defn_p->prec);
        }
        
        auto name() const noexcept
        {
            return defn_p->name();
        }

        auto arity() const noexcept
        {
            return defn_p->arity;
        }
        
    private:
        static inline prec_fn pri2prec = e_precedence<>::type::value;
        op_map              ops;
        expr_op_defn const *defn_p;
    };

    // ADDER
    // 1. allocate & initialize `expr_op` from `expr_op_defn` instance
    // 2. add name & aliases to x3::symbol_parser
    struct expr_op_adder
    {
        using DEFN_T    = expr_op_defn;
        using OBJECT_T  = expr_op;
        using VALUE_T   = OBJECT_T const *;

        // type index of primary name & two aliases
        using NAME_LIST = typename DEFN_T::NAME_LIST;

        template <typename PARSER>
        expr_op_adder(PARSER) : defns(PARSER::sym_defns) 
        {
            DEFN_T::op_names = at_c<typename PARSER::all_types_defns, 0>::value;
        }
        
        // actual `expr_op` instances
        static inline std::deque<OBJECT_T> obstack;

        template <typename X3>
        void operator()(X3& x3, unsigned count)
        {
            defns_count = count;
            for (auto defn_p = defns; count--; ++defn_p)
            {
                auto& s = obstack.emplace_back(*defn_p);
                for (auto i = 0; i < DEFN_T::MAX_NAMES; ++i)
                {
                    if (auto name = defn_p->name(i))
                    {
                        //std::cout << "op_adder: adding " << name << std::endl;
                        x3.add(name, &s);
                    }
                    else
                        break;
                }
            }
        }

        DEFN_T const* const defns;
        unsigned defns_count;
    };
}

// expose type in expression namespace
using detail::expr_op;
}

#endif
