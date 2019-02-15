#ifndef KAS_PARSER_AST_PRINT_H
#define KAS_PARSER_AST_PRINT_H


#include "utility/print_object.h"
#include "utility/is_container.h"


namespace kas::parser::print
{
    
template <typename OS>
struct stmt_print
{
    stmt_print(OS& os) : os(os) {}

    // empty is done...
    void operator()() const {};

    // process args as args...(don't inline)
    template <typename T, typename...Ts>
    void operator()(T&& arg, Ts&&...args) const;

    // specialize print method for container types
    template <typename T, typename U = is_container_t<std::decay_t<T>>>
    void print(T&& t) const
    {
        print(std::forward<T>(t), U());
    }

    // declare print containter
    template <typename T>
    void print(T&& t, std::true_type) const;
    
    // print single element
    template <typename T>
    void print(T const& t, std::false_type) const
    {
        os << t;
    }
    
    OS& os;
};

// don't inline non-trivial methods
template <typename OS>
template <typename T, typename...Ts>
void stmt_print<OS>::operator()(T&& arg, Ts&&...args) const
{
    print(std::forward<T>(arg));
    if constexpr (sizeof...(Ts)) {
        os << ", ";
        (*this)(std::forward<Ts>(args)...);
    }
}

// print containers in braces
template <typename OS>
template <typename T>
void stmt_print<OS>::print(T&& t, std::true_type) const
{
    os << "{";
    const char *delim = "";
    for (auto const& e : t)
    {
        os << delim;
        print(e);
        delim = ", ";
    }
    os << "}";
}

}
#endif
