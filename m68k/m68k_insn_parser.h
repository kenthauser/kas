#ifndef KAS_M68K_INSN_PARSER_H
#define KAS_M68K_INSN_PARSER_H

#include <boost/spirit/home/x3/string/symbols.hpp>


namespace kas::m68k::opc
{
namespace detail
{
    using namespace meta;

    template <typename T, typename = void>
    struct get_alias_ctor { using type = void; };

    template <typename T>
    struct get_alias_ctor<T, std::void_t<typename T::alias_ctor>>
    {
        using type = typename T::alias_ctor;
    };

    // XXX consider making return type value dependent on is_copyable()

    // create symbol table parser from sequence of insn traits
    // XXX here T == m68k_op_t: `op_parser` returns T*
    template <typename Encoding
            , typename T
            , typename RT = T*
            , template <typename Char, typename U> class Lookup = x3::tst>
    struct op_parser :
        x3::symbols_parser<
            Encoding, RT, Lookup<typename Encoding::char_type, RT>>
    {
        using op_value_ctor = typename T::value_ctor;
        using op_alias_ctor = meta::_t<get_alias_ctor<T>>;

        static inline unsigned count;

    protected:
        static auto& op_deque()
        {
            static auto _op_deque = new std::deque<T>{};
            return *_op_deque;
        }

    private:

        // adder(name1, name2 = {}, Args...)
        //
        // If `T` not in symbol table, create instance with
        // T(name1, Args...);
        //
        // If `T` is newly instantiated (in deque()), add name1 & name2
        // to symbol table for lookup
        //
        // return std::pair(T*, inserted)

        struct adder
        {
            op_parser const& r;

            template <typename...Ts>
            auto operator()(const char *name1, const char *name2
                                , Ts&&...args) const
            {
                // update statistic
                ++r.count;

                // has this name been seen?
                auto p = r.find(name1);
                if (p)
                    return std::make_pair(*p, false);      // not inserted

                // create T instance for this name
                auto& d = op_deque();
                auto& op = d.emplace_back(std::forward<Ts>(args)...);

                // check if storing instance, or pointer
                auto instance_p = &op;
                if constexpr (std::is_assignable<RT, decltype(op)>::value)
                    p = &op;
                else
                    p = &instance_p;

//#define TRACE_ADD
#ifdef TRACE_ADD
                std::cout << "adding : " << name1;
                if (name2)
                    std::cout << ", " << name2;
                std::cout << std::endl;
#endif
                // now add `op` to parser under name `name1`
                r.add(name1, *p);

                // also add under `name2` if defined
                if (name2)
                    r.add(name2, *p);

                return std::make_pair(*p, true);
            }
        };

        // depth first search for traits
        struct seq_adder
        {
            adder const& adder_obj;

            // add trait to parser, or recurse if meta::list<>
            template <typename Seq>
            void operator()(Seq s)
            {
            #if 0
                if constexpr (is_meta_list<Seq>::value)
                    meta::for_each(s, *this);
                else
            #endif
                    op_value_ctor{}(adder_obj, meta::apply<quote_trait<M68K_INSN>, Seq>());
            }
        };

        // add alias(s) to parser
        struct alias_adder
        {
            op_parser const& r;

            template <typename ORIG>
            struct do_alias
            {
                using orig = ORIG;
                op_parser const& r;

                template <typename List>
                void operator()(List s) const
                {
#if 0
                    if constexpr (is_meta_list<List>::value)
                        meta::for_each(s, *this);
                    else 
    #endif
                        if constexpr (!std::is_void<op_alias_ctor>::value)
                        op_alias_ctor{}(r, orig{}, s);
                    else
                        throw std::logic_error("alias_adder: alias_ctor not defined");
                }
            };

            // add trait to parser, or recurse if meta::list<>
            template <typename Seq>
            void operator()(Seq s) const
            {
                using orig = meta::front<Seq>;
                using rest = meta::pop_front<Seq>;

                meta::for_each(rest{}, do_alias<orig>{r});
            }
        };

    public:
        template <typename Seq, typename Alias = meta::list<>>
        auto& add_seq(Seq s, Alias a = {})
        {
            meta::for_each(s, seq_adder{adder{*this}});
            meta::for_each(a, alias_adder{*this});
            return *this;
        }


    };
}

template <typename T, typename RT = T*>
using op_parser_t = detail::op_parser<
                              boost::spirit::char_encoding::standard
                            , T
                            , RT
                            // , x3::tst_map
                            >;

}

#endif
