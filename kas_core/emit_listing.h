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
struct emit_listing : emit_formatted
{
    using diag_map_key_t   = typename parser::kas_loc;
    using diag_map_value_t = parser::kas_diag_t const *;
    using diag_map_t       = std::map<diag_map_key_t, diag_map_value_t>;

    // override `emit_stream` principle ctor
    emit_listing(kbfd::kbfd_object& kbfd, std::ostream& out)
            : emit_formatted(kbfd, out)
    {
        // create ordered list of diagnostics
        auto add  = [this](auto& obj) { diag_map.emplace(obj.loc(), &obj); };
        parser::kas_diag_t::for_each(add);

        // init iterators into diagnostics map
        diag_iter = diag_map.begin();
        diag_end  = diag_map.end();
    }

    // dtor: flush buffers
    ~emit_listing()
    {
        //get_line().flush();
    }

    // override `emit` to generate listing line after each insn
    void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
    {
        // dot always specified for listing
        insn.emit(*base_p, dot_p);
        gen_listing(*dot_p, insn.loc());
    }


private:
    // implement `emit_string` ABC methods
    void do_put(e_chan_num chan, std::string const& s) override
    {
        buffers[chan].push_back(std::move(s));
    }

    void do_put_diag(e_chan_num chan, uint8_t width, parser::kas_diag_t const& diag)
        override
    {
        buffers[chan].push_back(fmt_diag(width));
    }

    void do_put_reloc(e_chan_num chan, uint8_t width, std::string const& msg)
        override
    {
        if (chan == EMIT_DATA)
            relocs.push_back(std::move(msg));
    }
    
    //
    // accumulate "listing" in `listing_line` instance
    //

    auto& get_line()
    {
        static listing_line<Iter> line{out, *this};
        return line;
    }
    
    // push listing after insn
    void gen_listing(core_expr_dot const& dot, parser::kas_loc loc);

    auto prev_it(size_t num)
    {
        auto it = current_pos.find(num);
        if (it != current_pos.end())
            return it;

        auto initial = parser::error_handler<Iter>::initial(num);
        auto result = current_pos.emplace(num, initial);
        return result.first;
    }

    // concatinate strings with trailing space (assume leading space)
    static auto splice(std::vector<std::string>& v) 
    {
        std::string out;
        for (auto const& s : v)
            out += s + ' ';
        return out;
    }


    friend listing_line<Iter>;

    std::array<std::vector<std::string>, NUM_EMIT_FMT> buffers{};
    std::list<parser::kas_diag_t::index_t> diagnostics;
    std::list<std::string> relocs;
    std::map<size_t, Iter> current_pos;
    parser::kas_loc  prev_loc;

    // "map" diagnostics by "loc"
    using diag_iter_t = typename diag_map_t::iterator;
    diag_map_t  diag_map {};
    diag_iter_t diag_iter;
    diag_iter_t diag_end {};
};

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
    //void flush() { emit_line(prev, {}); }

    void gen_addr(data_type const&, core_expr_dot const& dot);
    void gen_data(data_type const&, bool last = false);
    void gen_equ(data_type  const&);
    void set_force_addr(data_type const&);
    void append_diag(diag_type&, parser::kas_loc const&);
    void append_reloc(reloc_type&);
    Iter emit_line(Iter first, Iter const& last);
private:
    void do_emit(Iter first, Iter const& last);

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
void emit_listing<Iter>::gen_listing(core_expr_dot const& dot, parser::kas_loc loc)
{
    // accumulate listing into `line`
    auto& line = get_line();
    
    ////std::cout << "emit_listing: insn_loc = " << loc.get();
    ////std::cout << ", src = " << loc.where() << std::endl;

    // don't generate listing for internally generated insns
    if (!loc)
        return;

    if (prev_loc && !(prev_loc < loc))
        throw std::logic_error{"Backwards listing: src = " + loc.where()};
    prev_loc = loc;

    // unpack location into file_num/first/last
    auto where = parser::error_handler<Iter>::raw_where(loc);
    auto idx   = where.first;
    auto first = where.second.begin();
    auto last  = where.second.end();

    // get iter to where in the source file we left off
    auto& prev = prev_it(idx)->second;

    // finish `prev_loc` insn with any pending diagnostics
    line.append_diag(diagnostics, prev_loc);

    // emit (comment) lines with no object code before this insn
    // NB: only emits complete source code lines (which end with new-line)
    prev = line.emit_line(prev, first);

    // format collected object code for this insn
    line.gen_addr(buffers[EMIT_ADDR], dot);
    line.gen_data(buffers[EMIT_DATA]);
    line.gen_equ (buffers[EMIT_EXPR]);
    line.set_force_addr(buffers[EMIT_ADDR]);
    line.append_reloc(relocs);
    buffers = {};       // data accumulated in buffers is now "emitted"

    // append diagnostics based on `loc`
    line.append_diag(diagnostics, loc);

    // emit source and formatted object code
    // NB: only emits complete source code lines (which end with new-line)
    // NB: lines with comments and/or multiple insns are left in buffers
    prev = line.emit_line(prev, last);
}

template <typename Iter>
void listing_line<Iter>::gen_addr(data_type const& addr, core_expr_dot const& dot)
{
    if (addr_field.empty())
    {
        if (!addr.empty())
            addr_field = addr.front();
        else
            addr_field = e.fmt_addr(dot);
    }
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
void listing_line<Iter>::append_diag(diag_type& new_diags
                                   , parser::kas_loc const& loc)
{
    while(e.diag_iter != e.diag_end)
    {
        auto [key, ptr] = *e.diag_iter;

        ////std::cout << "append_diag: insn = " << loc.get();
        ////std::cout << ", next_diag = " << key.get() << std::endl;

        if (loc < key)
            break;
        ////std::cout << "appending diag " << ptr->ref() << std::endl;
        new_diags.push_back(ptr->index());
        ++e.diag_iter;
    }
    diagnostics.splice(diagnostics.end(), new_diags);
}

template <typename Iter>
void listing_line<Iter>::append_reloc(reloc_type& new_relocs)
{
    relocs.splice(relocs.end(), new_relocs);
}

template <typename Iter>
Iter listing_line<Iter>::emit_line(Iter first, Iter const& last)
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

    // output pending diagnostics
    for (; !diagnostics.empty(); diagnostics.pop_front())
    {
        auto& diag = parser::kas_diag_t::get(diagnostics.front());
        auto message = diag.level_msg() + diag.message;
        out << std::string(addr_size + data_size + 2, ' ');
        ////std::cout << "emit_diag: loc = " << diag.ref() << std::endl;
        if (diag.loc())
            parser::error_handler<Iter>::err_message(out, diag.loc(), message);
        else 
            out << message << std::endl;
    }

    // output pending relocations
    for (; !relocs.empty(); relocs.pop_front())
    {
        // select column
        out << std::string(addr_size, ' ');
        out << "[RELOC: " << relocs.front() << "]" << std::endl;
    }

    // tail recursion for rest of source
    continuation_lines = cont_lines;
    return emit_line(next, last);
}

template <typename Iter>
void listing_line<Iter>::do_emit(Iter first, Iter const& last)
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
