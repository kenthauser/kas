#ifndef KAS_CORE_EMIT_LISTING_H
#define KAS_CORE_EMIT_LISTING_H

#if 0

listing algorithm:

1. Accumulate address, object code, source in seperate strings.

2. Source code is a iter-pair and includes trailing newline (or separator).

3. Sequence of inputs for insn must *end* with src.

4. Hold a running "source location" (end of last).

5. When source emitted, output from previous end to current beginning (eg. comments)

6. After "source prefix", set "emit obj" flag. Save "addr/obj" details.

7. Copy src to "output buffer". If newline emit with addr/obj data.

8.

XXXX

Development plan:

1. Output Addr, Obj, Comment, Src for each insn. Emit on "src".


#endif


#include "emit_string.h"
#include "core_options.h"
#include <regex>

namespace kas::core
{
template <typename Iter> struct listing_line;

template <typename Iter>
struct emit_listing : emit_base
{
    // save ostream. init `emit_base` with `put` & `put_diag`
    emit_listing(std::ostream& out) : out(out), emit_base(fmt) {}

    // public interface: push listing after insn
    void gen_listing(core_expr_dot const& dot, parser::kas_loc loc);

    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always specified for listing
        insn.emit(*this, dot_p);
        gen_listing(*dot_p, insn.loc());
    }

private:
    auto prev_it(size_t num)
    {
        auto it = current_pos.find(num);
        if (it != current_pos.end())
            return it;

        auto initial = parser::error_handler<Iter>::initial(num);
        auto result = current_pos.emplace(num, initial);
        return result.first;
    }

    auto splice(std::vector<std::string>& v) const
    {
        std::string out;
        for (auto const& s : v)
            out += s + ' ';
        return out;
    }

    decltype(emit_formatted::put) put()
    {
        return [&](e_chan_num num, std::string const& s)
        {
            buffers[num].push_back(std::move(s));
        };
    }

    decltype(emit_formatted::emit_diag) put_diag()
    {
        return [&](e_chan_num num, std::size_t width, parser::kas_diag_t const& diag)
        {
            if (num == EMIT_DATA) {
                std::string xxx(width * 2, 'x');
                buffers[EMIT_DATA].push_back(std::move(xxx));
            }
            diagnostics.push_back(diag.index());
        };
    }

    decltype(emit_formatted::emit_reloc) put_reloc()
    {
        return [&](e_chan_num num, std::size_t width, std::string const& msg)
        {
            relocs.push_back(std::move(msg));
        };
    }

    friend listing_line<Iter>;
    emit_formatted fmt{ put(), put_diag(), put_reloc() };
    std::array<std::vector<std::string>, NUM_EMIT_FMT> buffers;
    std::list<parser::kas_diag_t::index_t> diagnostics;
    std::list<std::string> relocs;
    std::map<size_t, Iter> current_pos;
    parser::kas_loc::index_t prev_loc {};
    std::ostream& out;
};

template <typename Iter>
void emit_listing<Iter>::gen_listing(core_expr_dot const& dot, parser::kas_loc loc)
{
    // accumulate listing into `line`
    static listing_line<Iter> line(out, *this);
    
    //std::cout << "emit_listing: loc = " << loc.get() << std::endl;

    // don't generate listing for internally generated insns
    if (!loc)
        return;

    if (loc.get() < prev_loc)
        throw std::logic_error{"Backwards listing: src = \"" + std::string(loc.where()) + "\""};
    prev_loc = loc.get();

    // unpack location into file_num/first/last
    auto where = parser::error_handler<Iter>::raw_where(loc);
    auto idx   = where.first;
    auto first = where.second.begin();
    auto last  = where.second.end();

    //auto src = std::string(first,last);
    //std::cout << "emit_listing: idx = " << idx << " src = [" << parser::parser_src::escaped_str(src) << "]" << std::endl;
    
    
    // get iter to where in the source file we left off
    auto& prev = prev_it(idx)->second;

    //auto prev_src = std::string(prev, first);
    //std:: cout << "emit_listing: prev = [" << parser::parser_src::escaped_str(prev_src) << "]" << std::endl;
    // emit all before this insn
    prev = line.emit_line(prev, first);

    // collect object code for this insn
    line.gen_addr(dot);
    line.gen_data(buffers[EMIT_DATA]);
    line.gen_equ (buffers[EMIT_EXPR]);
    line.set_force_addr(buffers[EMIT_ADDR]);
    line.append_reloc(relocs);
    line.append_diag(diagnostics);
    buffers = {};

    // emit this insn
    prev = line.emit_line(prev, last);
}

template <typename Iter>
struct listing_line
{
    using data_type  = typename decltype(emit_listing<Iter>::buffers)::value_type;
    using diag_type  = decltype(emit_listing<Iter>::diagnostics);
    using reloc_type = decltype(emit_listing<Iter>::relocs);

    static constexpr size_t addr_size  = 11;
    static constexpr size_t data_size  = 22;
    static constexpr size_t tab_space  = 4;
    static constexpr size_t cont_lines = 2;

    listing_line(std::ostream& out, emit_listing<Iter> &e) : e(e), out(out) {}

    void emit(Iter first, Iter last);

    void gen_addr(core_expr_dot const& dot);
    void gen_data(data_type const&, bool last = false);
    void gen_equ(data_type  const&);
    void set_force_addr(data_type const&);
    void append_diag(diag_type&);
    void append_reloc(reloc_type&);
    Iter emit_line(Iter first, Iter last);
private:
    void do_emit(Iter first, Iter last);

    std::string addr_field;
    std::string data_field;
    std::string equ_field;
    const std::string ellipse{"..."};

    std::vector<std::string> data_overflow;
    bool force_addr {false};
    short continuation_lines{0};

    emit_listing<Iter>& e;
    diag_type diagnostics;
    reloc_type relocs;
    std::ostream& out;
};

template <typename Iter>
void listing_line<Iter>::gen_addr(core_expr_dot const& dot)
{
    if (addr_field.empty())
        addr_field = e.fmt.fmt_addr(4, dot);
}

template <typename Iter>
void listing_line<Iter>::gen_data(data_type const& data, bool last)
{
    auto p = data.begin();
    auto e = data.end();

    if (p != e)
        data_field = *p++;

    while (p != e) {
        auto max_size = data_size;
        if (last)
            max_size -= 3;      // reserve sizeof ellipse

        if ((data_field.size() + p->size() + 1) > max_size)
            break;

        data_field += ' ' + *p++;
    }

    // test for case where data fits w/o ellipse on last
    if (last && std::distance(p, e) == 1)
        if ((data_field.size() + p->size() + 1) <= data_size)
            data_field += ' ' + *p++;

    if (p != e)
        data_overflow = { p, e };
}

template <typename Iter>
void listing_line<Iter>::set_force_addr(data_type const& data)
{
    if (!data.empty())
        force_addr = true;
}

template <typename Iter>
void listing_line<Iter>::gen_equ(data_type const& equ)
{
    if (equ_field.empty() && !equ.empty())
        equ_field = std::string("= ") + equ.front();
}

template <typename Iter>
void listing_line<Iter>::append_diag(diag_type& new_diags)
{
    diagnostics.splice(diagnostics.end(), new_diags);
}

template <typename Iter>
void listing_line<Iter>::append_reloc(reloc_type& new_relocs)
{
    relocs.splice(relocs.end(), new_relocs);
}

template <typename Iter>
Iter listing_line<Iter>::emit_line(Iter first, Iter last)
{
    // see if source contains newline
    std::match_results<Iter> m;
    if (!std::regex_search(first, last, m, std::regex("\\n")))
        return first;

    // emit source upto newline
    do_emit(first, m[0].first);
    auto next = m[0].second;

    // if more data, emit continuation lines
    while(continuation_lines--)
        if (!data_overflow.empty())
            do_emit(next, next);

    // output pending relocations
    for (; !relocs.empty(); relocs.pop_front())
    {
        // select column
        out << std::string(addr_size, ' ');
        out << "[RELOC: " << relocs.front() << "]" << std::endl;
    }

    // output pending diagnostics
    for (; !diagnostics.empty(); diagnostics.pop_front())
    {
        auto& diag = parser::kas_diag_t::get(diagnostics.front());
        auto message = diag.level_msg() + diag.message;
        out << std::string(addr_size + data_size + 2, ' ');
        //std::cout << "emit_diag: loc = " << diag.loc << std::endl;
        if (diag.loc())
            parser::error_handler<Iter>::err_message(out, diag.loc(), message);
        else 
            out << message << std::endl;
    }

    // tail recursion for rest of source
    continuation_lines = cont_lines;
    return emit_line(next, last);
}

template <typename Iter>
void listing_line<Iter>::do_emit(Iter first, Iter last)
{
    if (data_field.empty() && !data_overflow.empty())
    {
        // swap twiddles pointers. better than copying overflow vector
        data_type overflow;
        overflow.swap(data_overflow);
        gen_data(overflow);
    }

    switch (data_overflow.size())
    {
    case 0:
        break;
    case 1:
        if ((data_field.size() + data_overflow.front().size() + 1) < data_size)
        {
            data_field += " " + data_overflow.front();
            data_overflow.clear();
            break;
        }
        // FALLSTHRU
    default:
        if (!continuation_lines) {
            data_field += ' ' + ellipse;
            data_overflow.clear();
        }
        break;
    }

    out << std::left;

    // if data field present, emit size & address
    if (equ_field.size() && data_field.size() == 0)
    {
        data_field = equ_field;
        addr_field.clear();
    }

    out << std::setw(addr_size + 1) << addr_field;
    out << std::setw(data_size + 1) << data_field;

    addr_field.clear();
    data_field.clear();
    equ_field.clear();

    // now copy source, expanding tabs
    short tab_position{tab_space};
    while (first != last)
    {
        switch (auto c = *first++)
        {
            case '\t':
                if (tab_position)
                    out << std::string(tab_position, ' ');
                tab_position = tab_space;
                break;
            default:
                out << c;
                if (!--tab_position)
                    tab_position = tab_space;
        }
    }
    out << '\n';
}
}


#endif
