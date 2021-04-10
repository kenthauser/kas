#ifndef KAS_PARSER_ERROR_REPORTING_H
#define KAS_PARSER_ERROR_REPORTING_H

//
// Modify `x3` support file as follows:
//
// - remove `error_handler_tag`
// - Modify `clang-style` error message to support one-line format
// - Change `position-cache` container to std::deque (from std::vector)
// - Remove "err_out" from ctor; make ostream an operator() arg
// - Expose "file" via method
// - Add `position_max` method to expose size of pos_cache

// Insure that x3 headers that directly include x3 header get the
// modified class:
//
// - Define old header guard

/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
// #if !defined(BOOST_SPIRIT_X3_ERROR_REPORTING_MAY_19_2014_00405PM)

#include "kas_loc.h"

#ifndef BOOST_SPIRIT_X3_NO_FILESYSTEM
#include <boost/filesystem/path.hpp>
#endif

#include <boost/locale/encoding_utf.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <ostream>

// Clang-style error handling utilities

namespace kas::parser
{
namespace x3 = boost::spirit::x3;

// works for `T` is a std::basic_string
template <typename T>
auto escaped_str(std::basic_string<T> const& in)
{
    std::basic_string<T> output{"\""};

    for (auto&& c : in)
    {
        switch (c)
        {
            case '\t':
                output.append("[\\t]");
                break;
            case '\n':
                output.append("[\\n]");
                break;
            case '\"':
                output.append("\\\"");
                break;
            default:
                output.push_back(c);
                break;
        }
    }
    output.append("\"");
    return output;
}

template <typename REF>
template <typename OS>
void kas_diag<REF>::print(OS& os) const
{
    auto where = this->loc().where();
    if (!this->loc())
        where = "*** untagged ***";
    os << level_msg() << message << " : " << where;
    os << ": loc = " << std::dec << this->loc().get();
}


template <typename Iterator>
class x3_error_handler
{
public:
    typedef Iterator iterator_type;

    x3_error_handler(
        Iterator first, Iterator last, std::string file = "", int tabs = 4)
      : file(file)
      , tabs(tabs)
      , tab_position(tabs)
      , pos_cache(first, last) {}

    typedef void result_type;

    void operator()(std::ostream& err_out, Iterator err_pos, std::string const& error_message) const;
    void operator()(std::ostream& err_out, Iterator err_first, Iterator err_last, std::string const& error_message, bool) const;
    void operator()(std::ostream& err_out, x3::position_tagged pos, std::string const& message) const
    {
        auto where = pos_cache.position_of(pos);
        (*this)(err_out, where.begin(), where.end(), message);
    }

    template <typename AST>
    void tag(AST& ast, Iterator first, Iterator last)
    {
#ifdef TRACE_ERROR_HANDLER
        std::cout << "x3_error_handler::tag:";
        std::cout << " src = " << escaped_str(std::string({first, last}));
        std::cout << std::endl;
        std::cout << "x3_error_handler::tag:";
        std::cout << " position = " << position_max() << " AST = ";
        std::cout << boost::typeindex::type_id_with_cvr<AST>().pretty_name();
        std::cout << std::endl;
#endif
        return pos_cache.annotate(ast, first, last);
    }

    boost::iterator_range<Iterator> position_of(x3::position_tagged pos) const
    {
        return pos_cache.position_of(pos);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    // KAS additions
    //
    // four added methods:
    //      get_last_loc() & set_next_loc(next) to manage `kas_loc` segments
    //
    //      get_loc(ast) to generate "loc" from "annotation"
    //
    //      get_src(index) : get boost::iter_range from `position_cache`,
    //                       using `index` into cache (ie no offset)


    using loc_index_t = typename kas_loc::index_t;
    loc_index_t get_last_loc() const
    {
        return (position_max() >> 1) + offset;
    }

    // returns new offset
    loc_index_t set_next_loc(loc_index_t next)
    {
        offset = next - (position_max() >> 1);
        return offset;
    }

    // generatate "loc" from "annotation"
    auto get_loc(kas_position_tagged const& ast)
    {
        // generate an entry in position cache
        x3::position_tagged x3_loc;
        tag(x3_loc, ast.begin(), ast.end());

        // x3_handler allocates two entries per `position_tagged`
        loc_index_t loc = offset + (x3_loc.id_first >> 1);
#ifdef TRACE_ERROR_HANDLER
        std::cout << "x3_error_handler::get_loc: kas_loc = " << loc;
        std::cout << ", id_first = " << x3_loc.id_first;
        std::cout << ", src = " << escaped_str(std::string(ast));
        std::cout << std::endl;
#endif
        return loc;
    }

    auto get_src(loc_index_t idx) const
    {
        // two entries per index. convert to `pos_cache format`
        idx *= 2;
        x3::position_tagged pos { (int)idx, (int)idx+1 };
        return pos_cache.position_of(pos);    
    }

    // get beginning/end of file for listing
    auto first() const
    {
        return pos_cache.first();
    }
    auto last() const
    {
        return pos_cache.last();
    }

    auto get_file() const
    {
        return file;
    }
    
private:
    auto position_max() const
    {
        return pos_cache.get_positions().size();
    }

    loc_index_t offset{};

    //
    //
    ////////////////////////////////////////////////////////////////////////////


private:

    void print_err_msg(std::ostream& err_out, std::size_t line) const;
    void print_line(std::ostream& err_out, Iterator line_start, Iterator last) const;
    void print_indicator(std::ostream& err_out, Iterator& line_start, Iterator last
                        , char ind, char last_ind = {}) const;
    void skip_whitespace(Iterator& err_pos, Iterator last) const;
    void skip_non_whitespace(Iterator& err_pos, Iterator last) const;
    Iterator get_line_start(Iterator first, Iterator pos) const;
    std::size_t position(Iterator i) const;

    std::string file;
    int tabs;           // tab width
    mutable int tab_position;   // position within tab
    x3::position_cache<std::deque<Iterator>> pos_cache;
};

template <typename Iterator>
void x3_error_handler<Iterator>::print_err_msg(std::ostream& err_out, std::size_t line) const
{
    namespace fs = boost::filesystem;

    err_out << fs::path(file).generic_string() << ":";
    err_out << std::dec << line << ':';
}

template <typename Iterator>
void x3_error_handler<Iterator>::print_line(std::ostream& err_out, Iterator start, Iterator last) const
{
    auto end = start;
    while (end != last)
    {
        auto c = *end;
        if (c == '\r' || c == '\n')
            break;
        else
            ++end;
    }
    typedef typename std::iterator_traits<Iterator>::value_type char_type;
    std::basic_string<char_type> line{start, end};
    err_out << boost::locale::conv::utf_to_utf<char>(line);
}

template <typename Iterator>
void x3_error_handler<Iterator>::print_indicator(std::ostream& err_out
            , Iterator& start, Iterator last
            , char ind, char last_ind) const
{
    for (; start != last; )
    {
        auto c = *start++;
        if (c == '\r' || c == '\n')
            break;

        if (c == '\t')
        {
            if (tab_position)
                err_out << std::string(tab_position, ind);
            tab_position = tabs;
        }
        else if (start == last && last_ind)
        {
            err_out << last_ind;
        }
        else
        {
            err_out << ind;
            if (!--tab_position)
                tab_position = tabs;
        }
    }
}

template <typename Iterator>
void x3_error_handler<Iterator>::skip_whitespace(Iterator& err_pos, Iterator last) const
{
    // make sure err_pos does not point to white space
    while (err_pos != last)
    {
        char c = *err_pos;
        if (std::isspace(c))
            ++err_pos;
        else
            break;
    }
}

template <typename Iterator>
void x3_error_handler<Iterator>::skip_non_whitespace(Iterator& err_pos, Iterator last) const
{
    // make sure err_pos does not point to white space
    while (err_pos != last)
    {
        char c = *err_pos;
        if (std::isspace(c))
            break;
        else
            ++err_pos;
    }
}

template <class Iterator>
inline Iterator x3_error_handler<Iterator>::get_line_start(Iterator first, Iterator pos) const
{
    Iterator latest = first;
    for (Iterator i = first; i != pos; ++i)
        if (*i == '\r' || *i == '\n')
            latest = i;
    return latest;
}

template <typename Iterator>
std::size_t x3_error_handler<Iterator>::position(Iterator i) const
{
    std::size_t line { 1 };
    typename std::iterator_traits<Iterator>::value_type prev { 0 };

    for (Iterator pos = pos_cache.first(); pos != i; ++pos) {
        auto c = *pos;
        switch (c) {
        case '\n':
            if (prev != '\r') ++line;
            break;
        case '\r':
            if (prev != '\n') ++line;
            break;
        default:
            break;
        }
        prev = c;
    }

    return line;
}

template <typename Iterator>
void x3_error_handler<Iterator>::operator()(std::ostream& err_out,
    Iterator err_pos, std::string const& error_message) const
{
    Iterator first = pos_cache.first();
    Iterator last = pos_cache.last();

    // make sure err_pos does not point to white space
    skip_whitespace(err_pos, last);

    print_err_msg(err_out, position(err_pos));
    err_out << error_message << std::endl;

    Iterator start = get_line_start(first, err_pos);
    if (start != first)
        ++start;
    print_line(err_out, start, last);
    print_indicator(err_out, start, err_pos, '_');
    err_out << "^_" << std::endl;
}

template <typename Iterator>
void x3_error_handler<Iterator>::operator()(std::ostream& err_out,
    Iterator err_first, Iterator err_last, std::string const& error_message, bool show_line) const
{
    Iterator first = pos_cache.first();
    Iterator last  = pos_cache.last();

    // make sure err_pos does not point to white space
    skip_whitespace(err_first, last);

    Iterator start = get_line_start(first, err_first);
    if (start != first)
        ++start;
    if (show_line)
        print_line(err_out, start, last);

    char last_char{};
    if (err_first == err_last)
        last_char = '^';

    print_indicator(err_out, start, err_first, ' ', last_char);
    print_indicator(err_out, start, err_last, '~');
    err_out << " <<-- Here" << std::endl;

    print_err_msg(err_out, position(err_first));
    err_out << error_message << std::endl;

}

// create permanent `loc` in handler's table
template <typename Iter>
kas_position_tagged_t<Iter>::operator kas_loc&() const
{
    if (!loc && handler)
        loc = handler->get_loc(*this);
    return loc;
}

// lookup handler/first/last given `loc`
template <typename Iter>
void kas_position_tagged_t<Iter>::
    init() const
{
    auto w = error_handler<Iter>::raw_where(loc);
    //handler = &error_handler<Iter>::get_handler(w.first);
    first   = w.second.begin();
    last    = w.second.end();
}

template <typename Iter>
auto kas_position_tagged_t<Iter>::
    where() const -> std::basic_string<value_type>
    {
        // if handler not set, use `loc`
        if (!handler)
            return loc.where();
        //return { first, last };
        return escaped_str<value_type>({first, last});
    }
}

#endif
