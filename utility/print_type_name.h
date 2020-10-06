#ifndef UTILITY_PRINT_TYPE_NAME_H
#define UTILITY_PRINT_TYPE_NAME_H

// typename printing functor for development
#include <boost/type_index.hpp>
#include <iostream>
#include <utility>

struct print_type_name
{
    print_type_name() = default;

    template <typename T>
    print_type_name(T&& prefix_str, const char *sep = ": ")
        : prefix{std::forward<T>(prefix_str)}
    {
        prefix += sep;
    }

    template <typename T>
    void operator()(T&, std::ostream& os = std::cout) const
    {
        os << prefix << boost::typeindex::type_id_with_cvr<T>().pretty_name() << std::endl;
    }

    template <typename T>
    void name(std::ostream& os = std::cout) const
    {
        os << prefix << boost::typeindex::type_id_with_cvr<T>().pretty_name() << std::endl;
    }

    std::string prefix;
};

#endif
