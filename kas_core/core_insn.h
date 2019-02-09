#ifndef KAS_CORE_CORE_INSN_H
#define KAS_CORE_CORE_INSN_H

///////////////////////////////////////////////////////////////////////////
//
//                          C O R E _ I N S N
//
///////////////////////////////////////////////////////////////////////////
//
// The `core_insn` class holds the result of evalutation of a `parser::parser_stmt`.
//
// The result is a tuple: {core::opcode, core::insn_data, parser::position_loc },
// which is then written to the `insn_container`.
//
// When evaluating assembled data, information stored in `insn_container` is 
// reconstitued as `core_insn` & evaluated, updated `insn_container` as appropriate.
//
///////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////

#include "opcode.h"
#include "core_fits.h"
//#include "parser/parser.h"
//#include "parser/parser_variant.h"

#include <ostream>


namespace kas::core
{

struct core_insn
{
    using op_size_t = opcode::op_size_t;
   
    // used by internal methods the use opcode ctor
    template <typename...Ts>
    core_insn(opcode const&, Ts&&...);
    core_insn(parser::kas_loc loc = {});
    core_insn(insn_container_data const);
    void calc_size(core_fits const&) const;
    void emit(emit_base& base, core_expr_dot const *dot_p) const;
    op_size_t size() const;
    kas_loc const& get_loc() const;
    bool is_relaxed() const;
    
    struct print_fmt_t
    {
        core_insn& insn;
    };

    print_fmt_t print_fmt()
    {
        return { *this };
    }
    
    template <typename OS>
    friend OS& operator<<(OS& os, print_fmt_t  arg)
    {
#ifdef XXX
        auto& insn = arg.insn;
        auto iter = data.begin() + insn.first;
        // XXX duplicate?
        auto& op = insn.get_opcode();
        os << op.name() << " " << *op.size_p << ": ";
        op.fmt(iter, insn.cnt, os);
#endif
        return os;
    }

    // test fixture interface
    void raw(std::ostream& out)
    {
#ifdef XXX
        //std::cout << "core_insn::raw: first = " << std::dec << first << ", cnt = " << cnt << std::endl;
        auto iter = data.begin() + first;
        get_opcode().raw(std::move(iter), cnt, out);
#endif
    }
    
    struct print_raw_t {
        core_insn & insn;
    };

    print_raw_t print_raw()
    {
        return { *this };
    }

    template <typename OS>
    friend OS& operator<<(OS& os, print_raw_t  arg)
    {
#ifdef XXX
        auto& insn = arg.insn;
        auto iter = data.begin() + insn.first;
        insn.get_opcode().raw(iter, insn.cnt, os);
#endif
        return os;
    }



#if 0
    using INSN_DATA = typename opcode::data_t;

    static void clear()
    {
        INSN_DATA::clear();
    }

//    static inline kas_clear _c{clear};

    using fixed_t     = typename INSN_DATA::fixed_t;
    using Iter        = typename INSN_DATA::Iter;
    using opc_index_t = typename INSN_DATA::opc_index_t;
    using op_size_t   = typename INSN_DATA::op_size_t;
    // ctors

    // index of zero is empty instruction
    //core_insn() = default;

    core_insn()
    {
        // XXX
        data_size = data.size();
    }


    static auto& data_inserter()
    {
        static auto di = data.back_inserter();
        return di;
    }
    
    template <typename T>
    core_insn gen_insn(T&);

    template <typename T>
    core_insn(T&& op) : index(op.index()), fixed(*(op.fixed_p)), size(*(op.size_p)) {}

    void set_loc(parser::kas_position_tagged loc)
    {
        static size_t data_index;
        first = data_index;
        data_index = data.size();
        cnt = data_index - first;
        this->loc = loc;
        // generate err message from last expr_t()
        if (size.is_error()) {
            // index = opc_error().index();
            // if (auto msg = data.back().get_p<std::string>())
            //     fixed.fixed = kas_diag::error(loc, *msg).index();
            // else
            //     fixed.fixed = 0;
            size = 0;
        }
    }

    bool is_empty() const
    {
        return index == 0;
    }

    // provide public interface to `opcode` functions
    auto& get_opcode()
    {
        // "splice" base type values with derived type vtable...
        return opcode::get(index).init(fixed, size);
    }

    auto name()
    {
        return get_opcode().name();
    }

    // using Iter = typename opcode::Iter;

    // auto& get_data
    // {
    //     return opcode::data;
    // }

    auto calc_size(Iter& iter, core_fits const& fits)
    {
        auto old_size = size;

        // `opcode.calc_size` return true if change
        auto new_size = get_opcode().calc_size(iter, cnt, fits);
        if (old_size != new_size) {
            if (new_size.is_error()) {
                index = opc::opc_error().index();
                size = old_size.min;
            } else {
                size = new_size;
            }
        }
        iter += cnt;
        return size;
    }

    auto calc_size(core_fits const& fits)
    {
        auto iter = data.begin() + first;
        return calc_size(iter, fits);
    }


    void emit(Iter& iter, uint16_t n, emit_base& base, core_expr_dot const *dot_p)
    {
        // XXX move this to test fixture impl.
        auto wr_size = size();
        auto expect = base.position() + wr_size;
        auto section_p = &base.get_section();
        auto save_iter = iter;

        // emit copies iter
        get_opcode().emit(iter, cnt, base, dot_p);
        iter += cnt;

        if (&base.get_section() != section_p)
            return;     // no match because changed sections

        if (auto delta = expect - base.position()) {
            std::cout << "core_insn::emit: expected  = " << std::hex << wr_size;
            std::cout << " actual = " << (wr_size - delta) << std::endl;
            std::cout << "core_insn::emit: insn: ";
            get_opcode().fmt(save_iter, cnt, std::cout);
            std::cout << std::endl;
        }
    }

    // test fixture interface
    void raw(std::ostream& out)
    {
        //std::cout << "core_insn::raw: first = " << std::dec << first << ", cnt = " << cnt << std::endl;
        auto iter = data.begin() + first;
        get_opcode().raw(std::move(iter), cnt, out);
    }

    void fmt(std::ostream& out)
    {
        //std::cout << "core_insn::fmt: first = " << std::dec << first << ", cnt = " << cnt << std::endl;
        auto iter = data.begin() + first;
        auto& op= get_opcode();

        out << op.name() << " " << *op.size_p << ": ";
        op.fmt(iter, cnt, out);
    }

    void emit(emit_base& base, core_expr_dot const *dot_p)
    {
        //std::cout << "core_insn::emit: first = " << std::dec << first << ", cnt = " << cnt << std::endl;
        // extra tests are in test fixture interface method only.
        auto iter = data.begin() + first;
        // auto position = base.position;
        emit(iter, cnt, base, dot_p);
        // if ((position + size.min) != base.position) {
        //     std::cout << "core_insn::emit: expected: " << size.min;
        //     std::cout << " actual: " << base.position - position;
        //     std::cout << " insn: ";
        //     fmt(std::cout);
        // }

    }

    // getters
    auto& get_fixed()
    {
        return fixed;
    }

    auto& get_size() const
    {
        return size;
    }

    bool is_relaxed() const
    {
        return size.is_relaxed();
    }

    // update size: used by `org` & `skip`
    void set_size(op_size_t new_size)
    {
        this->size = new_size;
    }

    // update index: used by `org` & `skip`
    void set_index(uint16_t new_index)
    {
        this->index = new_index;
    }

    struct print_raw_t {
        core_insn & insn;
    };

    print_raw_t print_raw()
    {
        return { *this };
    }

    struct print_fmt_t {
        core_insn& insn;
    };

    print_fmt_t print_fmt()
    {
        return { *this };
    }

    template <typename OS>
    friend OS& operator<<(OS& os, print_raw_t  arg)
    {
        auto& insn = arg.insn;
        auto iter = data.begin() + insn.first;
        insn.get_opcode().raw(iter, insn.cnt, os);
        return os;
    }

    template <typename OS>
    friend OS& operator<<(OS& os, print_fmt_t  arg)
    {
        auto& insn = arg.insn;
        auto iter = data.begin() + insn.first;
        // XXX duplicate?
        auto& op = insn.get_opcode();
        os << op.name() << " " << *op.size_p << ": ";
        op.fmt(iter, insn.cnt, os);
        return os;
    }

    uint16_t  index {};
    uint16_t  cnt {};
    fixed_t   fixed {};
    op_size_t size ;
    uint32_t  first {};
    parser::kas_loc   loc {};
    static inline uint32_t data_size;
#endif
};
}

#endif
