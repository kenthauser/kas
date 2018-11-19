#ifndef KAS_UTILITY_PRINT_OBJECT_H
#define KAS_UTILITY_PRINT_OBJECT_H

#include <boost/type_index.hpp>
#include <type_traits>

// print object operates as follows:
//      1) if member function `print(OS&)` exists, use it
//      2) if type is output-streamable, stream it
//      3) print typename (using boost::type_index)


namespace kas
{
    namespace print_object_detail
    {

    template <typename OS>
    struct print_object
    {
        template <typename, typename = void>
        struct has_print_member : std::false_type {};

        template <typename T>
        struct has_print_member<
                  T
                , std::void_t<decltype(
                        std::declval<T>().print(std::declval<OS&>())
                        )>
                > : std::true_type {};

        template <typename, typename = void>
        struct can_ostream : std::false_type {};

        template <typename T>
        struct can_ostream<
                  T
                , std::void_t<decltype(
                        std::declval<OS&>().operator<<(std::declval<T>())
                        )>
                > : std::true_type {};

        // prefer print member function
        template <typename T>
        std::enable_if_t<
            has_print_member<T>::value
            >
        operator()(T&& obj)
        {
            obj.print(os);
        }

        // if no member function, ostream if possible
        template <typename T>
        std::enable_if_t<
            !has_print_member<T>::value &&
            can_ostream<T>::value
            >
        operator()(T&& obj)
        {
            os << obj;
        }

        // use type name as fallback
        template <typename T>
        std::enable_if_t<
            !has_print_member<T>::value &&
            !can_ostream<T>::value
            >
        operator()(T&& obj)
        {
            os << boost::typeindex::type_id_with_cvr<T>().pretty_name();
        }

        OS& os;
    };
    }

template <typename OS, typename T>
void print_object(OS& os, T&& obj)
{
     print_object_detail::print_object<OS>{os}(std::forward<T>(obj));
}
}

#endif
