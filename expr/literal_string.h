#ifndef KAS_EXPR_LITERAL_STRING_H
#define KAS_EXPR_LITERAL_STRING_H

// declare assembler FLOATING & STRING literal types

// since both strings & floating point types won't fit in a 32-bit int
// (or even a 64-bit int), allocate each on a private deque & reference
// by index. This helps with variant & speeds copying.

// *** STRING CONTAINTER TYPE ***

// c++ delcares type types of parsed strings:
// narrow (default), wide('L'), utc-8/16/32 ('u8'/'u'/'U')
// declare a single type using a std::vector<uchar32_t> to hold characters
// with instance variables to describe the string (8/16/32 unicode/host format)

#include "parser/parser_types.h"
#include "kas_core/ref_loc_t.h"
#include "kas_core/kas_object.h"
#include <boost/range/iterator_range.hpp>
#include <iomanip>

namespace kas::expression
{


namespace detail {

    using kas_string = core::ref_loc_t<struct kas_string_t>;

    struct kas_string_t : core::kas_object<kas_string_t, kas_string>
    {
        using emits_value = std::true_type;
        using ref_t       = kas_string;

        using base_t::index;

        enum ks_coding : uint8_t { KS_STR, KS_WIDE, KS_U8, KS_U16, KS_U32 };

        kas_string_t(kas_position_t loc, ks_codeing coding = KS_STR)
                : kas_string_t(loc.first, loc.last, coding) {}

        template <typename Iter>
        kas_string_t(Iter const& first, Iter const& last, ks_coding coding = KS_STR)
            : s_string{get_string(first, last, coding)}, s_coding(coding) {}

        template <typename Iter>
        void *get_string(Iter const& first, Iter const& last, ks_coding coding);

    private:
        // create utf-8 std::string
        // XXX move to support .cc file
        std::string& get_str() const
        {
            if (c_str_.empty()) {
                std::ostringstream os;
                os << std::hex << "\"";

                for (auto ch : data) {
                    // if control-character: display as hex
                    if (ch <= 0x1f)
                        os << std::setw(2) << ch;
                    else if (ch < 0x7f)
                        os << static_cast<char>(ch);
                    else if (is_unicode() && ch <= 0xffff)
                        os << "U+" << std::setw(4) << ch << " ";
                    else if (is_unicode() && ch_size_ > 1)
                        os << "U+" << std::setw(6) << ch << " ";
                    else
                        os << "\\x" << std::setw(ch_size_ * 2);
                }
                os << "\"";
                c_str_ = os.str();
            }
            return c_str_;
        }

    public:

#if 0
        // string manipulation methods
        auto  begin()      const { return data.begin(); }
        auto  end()        const { return data.end();   }
        auto  size()       const { return data.size();  }
        auto  ch_size()    const { return ch_size_;     }
        bool  is_unicode() const { return unicode_;     }
        void  push_back(uint32_t ch) { data.push_back(ch); }
        auto  c_str() const { return get_str().c_str();   }

        // template <typename OS>
        // friend OS& operator<<(OS& os, kas_string_t const& str)
        //     { return os << str.get_str(); }
        template <typename OS>
        void print(OS& os) const
            { os << get_str(); }
#endif
            
    private:
        void *s_string {};      // basic_string for s_coding
        void *d_string {};      // basic_string for d_coding
        ks_coding s_coding {};
        ks_coding d_coding {};
    };
}

// expose in expression namespace
using kas_string = detail::kas_string;

#endif
