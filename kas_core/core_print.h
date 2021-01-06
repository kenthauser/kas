#ifndef KAS_CORE_PRINT_H
#define KAS_CORE_PRINT_H

// #include "core_insn.h"

#include <ostream>


namespace kas { namespace core
{
    namespace detail {
        struct ios_flags_raii {
            using base_t  = std::ios_base;
            using flags_t = base_t::fmtflags;

            ios_flags_raii(base_t& base)
                : base  { base }
                , flags { base.flags() }
                {}

            ~ios_flags_raii() { base.flags(flags); }

            base_t& base;
            flags_t flags;
        };

        using BLK_T = uint16_t;

        template <typename T, typename OS>
        void _put_hex(OS& os, std::size_t n, T v, const char *suffix)
        {
            using BLK = std::conditional_t<
                  std::greater<>{}(sizeof(BLK_T), sizeof(T))
                , T
                , BLK_T>;

            if (n > sizeof(BLK))
                _put_hex<T>(os, n-= sizeof(BLK), v >> (8 * sizeof(BLK)), "_");

            os << std::setw(n * 2);
            // use static cast to mask bits away -- but don't print as `char`
            os << (unsigned)static_cast<BLK>(v) << suffix;
        }

        template <typename T>
        void put_hex(std::ostream& os, T const& v, const char *suffix = " ")
        {
            using UT = std::make_unsigned_t<T>;     // object code is unsigned
            ios_flags_raii{os};
            auto fill = os.fill();
            os << std::hex << std::right << std::setfill('0');
            _put_hex<UT>(os, sizeof(T), v, suffix);
            os.fill(fill);
        }

    }

    // goes in globl hdr.
    // template <typename OS> void emit(OS&);


    template <typename T>
    void emit_old(std::ostream& os, expr_t const& e)
    {
        os << e << ' ';
        // e.apply_visitor(x3::make_lambda_visitor<void>([&](auto& node)
        //      { emit<T>(os, node); }));
    }

    // integral format & value
    template <typename T, typename V, typename = std::enable_if_t<std::is_integral<V>::value>>
    void emit_old(std::ostream& os, V const* p)
    {
        using MIN_T = std::numeric_limits<T>;
        using MAX_T = std::numeric_limits<std::make_unsigned_t<T>>;

        V value = *p;

        bool value_ok = MIN_T::min() <= value && value <= MAX_T::max();

        detail::put_hex<T>(os, value);
    }
}}


#endif
