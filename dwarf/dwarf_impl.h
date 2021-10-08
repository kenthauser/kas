#ifndef KAS_CORE_CORE_DWARF_2_H
#define KAS_CORE_CORE_DWARF_2_H

#include "kas/kas_string.h"
#include "kas_core/insn_inserter.h"
#if 0
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
#include "kas_core/opc_symbol.h"
#include "kas_core/opc_segment.h"
#endif
#include "dwarf_fsm.h"
#include "dwarf_opc.h"
#include "dwarf_emit.h"

#include <meta/meta.hpp>

namespace kas::dwarf
{
using namespace meta;
using namespace meta::placeholders;

// create an "core_addr()" entry
// NB: only works if the target fragment is "relaxed"
// XXX see if can refactor to use core_section::for_frags()
// XXX should move to `core_addr`
inline decltype(auto) gen_addr_ref(unsigned segment_idx, std::size_t offset
                                  , core::core_fragment const *frag_p = nullptr)
{
    static std::deque<core::addr_offset_t> offsets;

    if (!frag_p)
    {
        auto& segment = core::core_segment::get(segment_idx); 
        frag_p = segment.initial();
    }

    if (!frag_p)
        throw std::runtime_error("gen_addr_ref: empty segment");

    // now find fragment for "offset"
    while (frag_p) {
        // is address in this frag?
        // NB: allow at end for zero size or no following frag
        // NB: address is relaxed -- don't let `best` beat `good`
        auto end = frag_p->base_addr()() + frag_p->size()();
        if (offset <= end)
            break;

        // advance to next    
        frag_p  = frag_p->next_p();
    }

    if (!frag_p)
        throw std::runtime_error("gen_addr_ref: offset out of range");

    // save offset in static area
    offset -= frag_p->base_addr()();
    offsets.emplace_back(offset);

    // create address with frag_p/offset_p pair
    return core::core_addr_t::add(frag_p, &offsets.back());
}

template <typename T>
auto& gen_dwarf_32_header(T& emit)
{
    using core::core_symbol_t;
    using core::opc::opc_label;

    // declare symbols forward referenced
    parser::kas_loc loc{};
    auto& end_data_line = core::core_addr_t::add();
    auto& end_hdr       = core::core_addr_t::add();

    // section length (not including section length field)
    auto& length = end_data_line - emit.get_dot(core::core_addr_t::DOT_NEXT);
    emit(UWORD(), length);
    //emit(UWORD(), end_data_line - emit.get_dot(core::core_addr_t::DOT_NEXT));
    std::cout << "dwarf_32_header: end_data_line = " << end_data_line << std::endl;
    emit(UHALF(), 4);                       // dwarf version
    emit(UWORD(), end_hdr - emit.get_dot(core::core_addr_t::DOT_NEXT));

    emit(UBYTE(), K_LNS_MIN_INSN_LENGTH);
    emit(UBYTE(), K_LNS_MAX_OPS_PER_INSN);  // added version 4
    emit(UBYTE(), K_LNS_DEFAULT_IS_STMT);
    emit(UBYTE(), K_LNS_LINE_BASE);
    emit(UBYTE(), K_LNS_LINE_RANGE);
    emit(UBYTE(), K_LNS_OPCODE_BASE);
    
    // emit # of operands for std opcodes
    using STD_INSNS = meta::filter<DW_INSNS, insn_is_type<STD>>;
    using STD_SIZES = meta::transform<STD_INSNS, meta::quote<meta::size>>;

    // handle OPCODE_BASE less than or equal to defined STD_INSNS
    meta::for_each(STD_SIZES{}, [&emit, n = K_LNS_OPCODE_BASE](auto&& e) mutable
        {
            if (--n > 0)
                // XXX if arg is `size_t` a zero is emited... 
                emit(UBYTE(), (uint8_t)e() - DW_INSN_ARG_INDEX);
        });

    // handle OPCODE_BASE larger than defined STD_INSNS 
    auto vendor_ext_insns = K_LNS_OPCODE_BASE - (int)STD_INSNS::size();
    while (--vendor_ext_insns > 0)
        emit(UBYTE(), 0);

    // emit directories
    //emit(UBYTE(), 0);
    emit(NAME(), "");
    // emit files
    auto emit_file = [&emit](dwarf_file const& f) mutable
        {
            // proper file #
            emit(NAME(), f.name.c_str());
            emit(ULEB(), 0);      // always current directory
            emit(ULEB(), f.file_mtime);
            emit(ULEB(), f.file_size);
        };
    dwarf_file::for_each(emit_file);
    emit(UBYTE(), 0);

    // header complete
    emit(end_hdr);
    return end_data_line;
}

// select rule to update Dwarf Line state variable
// NB: there is always a rule to update
namespace detail
{
    using namespace meta;

    template <unsigned COL>
    struct is_update
    {
        static constexpr auto N = COL + DW_INSN_OP_INDEX;

        template <typename T>
        using invoke = std::is_base_of<OP_before, at_c<T, N>>;
    };

    struct is_update2
    {
        template <typename T>
        using invoke = std::is_base_of<OP_before, T>;
    };

    struct is_after
    {
        template <typename T>
        using invoke = std::is_base_of<OP_after, T>;
    };

    template <unsigned COL>
    struct RULE_for_COL_impl
    {
        // find rule to update column
        // NB: DL_opc doesn't have rule -- default to `copy` 
        using rule_list = find_if<DW_INSNS, is_update<COL>>;
        using type = front<std::conditional_t<
                                  empty<rule_list>::value
                                , DW_INSNS
                                , rule_list
                                >
                            >;
    };

    template <typename T>
    struct COL_for_RULE_impl
    {
        using COLS = drop_c<T, DW_INSN_OP_INDEX>;
        using col_list = find_if<COLS, is_update2>;
    
        static constexpr std::size_t index = size<COLS>::value
                                           - size<col_list>::value;

        static constexpr auto value = index < DW_INSN_STATE_COLS()
                                      ? index : DW_INSN_STATE_COLS();
    };

}

template <unsigned DL_COL>
using RULE_for_COL = _t<detail::RULE_for_COL_impl<DL_COL>>;

template <typename RULE>
constexpr auto COL_for_RULE = detail::COL_for_RULE_impl<RULE>::value;

// name FSM rules used in manual program generation
using RULE_copy         = RULE_get_c<STD,  1>;
using RULE_set_address  = RULE_get_c<EXT,  2>;
using RULE_special      = RULE_get_c<SPL,  1>;
using RULE_const_add_pc = RULE_get_c<STD,  8>;

using RULE_advance_pc   = RULE_get_c<STD,  2>;
using RULE_advance_line = RULE_get_c<STD,  3>;
using RULE_end_sequence = RULE_get_c<EXT,  1>;

constexpr int op_advance(int addr_delta, int op_index_delta = 0)
{
    int n  = addr_delta / K_LNS_MIN_INSN_LENGTH;
        n *= K_LNS_MAX_OPS_PER_INSN;
        n += op_index_delta;

    return n;
}

template <typename EMIT, typename...COLS>
constexpr void emit_side_effects(DL_STATE& s, EMIT& emit, meta::list<COLS...>, unsigned N = 0)
{
    //using COLS = drop_c<T, DW_INSN_OP_INDEX>;
    using col_list = meta::find_if<meta::list<COLS...>, detail::is_after>;

    constexpr auto to_do = meta::size<col_list>::value;
    if constexpr (to_do != 0)
    {
        N += sizeof...(COLS) - to_do;
        using oper = typename meta::front<col_list>::oper;
        oper{}(s, emit, N, meta::list<>());
        emit_side_effects(s, emit, meta::drop_c<col_list, 1>(), N+1);
    }
}

// c++17 `if constexpr` allows single constexpr recursive function
template <typename...Args>
constexpr int ext_arg_len(meta::list<Args...> args, int n = 0)
{
    using arg_t = decltype(args);
    if constexpr (sizeof...(Args) == 0)
    {
        return n;
    } 
    else
    {
        constexpr int arg_size = meta::front<arg_t>::size;
        if (arg_size < 0)
            return -1;      // variable size: emit labels
        return ext_arg_len(meta::drop_c<arg_t, 1>(), n + arg_size);
    }
}

template <typename R, typename EMIT, typename...Ts>
void emit_rule(DL_STATE& s, EMIT& emit, Ts...args)
{
    using INSN_TYPE = meta::at_c<R, DW_INSN_TYPE_INDEX>;
    using INSN_LIST = meta::filter<DW_INSNS, insn_is_type<INSN_TYPE>>;
    using INDEX     = meta::find_index<INSN_LIST, R>;

    core::symbol_ref ext_ref;
    int code = 0;

    if constexpr (std::is_same<INSN_TYPE, STD>())
    {
        // standard opcode: emit index + 1
        emit(UBYTE(),(int) INDEX::value + 1);
    } 
    else if constexpr (std::is_same<INSN_TYPE, EXT>())
    {
        // extended opcode: format: 
        // 1. emit zero byte
        // 2. emit length of opcode & data
        //    (length doesn't include zero nor length field)
        // 3. emit opcode (unsigned byte)
        // 4. use common routine for values

        emit(UBYTE(),(int) 0);              // extended
        
        constexpr auto arg_len = ext_arg_len(meta::drop_c<R, DW_INSN_ARG_INDEX>());

        // if fixed length, emit data
        if constexpr (arg_len < 0) {
            // variable length -- drop labels & emit `core_expr`
            
            auto& dot = emit.get_dot(core::core_addr_t::DOT_NEXT);
            ext_ref   = core::core_symbol_t::add("next").ref();

            emit(ULEB(), ext_ref.get() - dot);
        } else if constexpr (arg_len < 128) {
            // add INSN byte to fixed length
            emit(UBYTE(),(int)arg_len + 1);   // assume fixed length not >= 128
        } 

        // extended opcode
        emit(UBYTE(),(int) INDEX::value + 1); // assume extended code not >= 128
        
        // FALLSTHRU to common routine for data
    }
    else if constexpr (std::is_same<INSN_TYPE, SPL>())
    {
        auto pc_delta   = std::get<0>(std::forward_as_tuple(args...));
        auto line_delta = std::get<1>(std::forward_as_tuple(args...));
             code       = pc_delta * K_LNS_LINE_RANGE;
             code      += line_delta - K_LNS_LINE_BASE;

        emit(UBYTE(), (int)code + K_LNS_OPCODE_BASE);
    }
    else throw std::logic_error("bad DL insn type"); 
    
    std::cout << insn_name<R>(code);

    constexpr auto col = COL_for_RULE<R>;
    if constexpr (col < NUM_DWARF_LINE_STATES) {
        using fmts = meta::drop_c<R, DW_INSN_ARG_INDEX>;
        using oper = typename meta::at_c<R, col + DW_INSN_OP_INDEX>::oper;
        oper{}(s, emit, col, fmts{}, std::forward<Ts>(args)...);
    }
    
    // emit label if needed to calculate length for EXTENDED format
    if (ext_ref)
        emit(opc_label(), ext_ref);

    emit_side_effects(s, emit, meta::drop_c<R, DW_INSN_OP_INDEX>());
    std::cout << std::dec << std::endl;
}

template <typename EMIT, std::size_t...COLS>
constexpr auto emit_fns(std::index_sequence<COLS...>)
{
    using dl_value_t = typename DL_STATE::dl_value_t;
    using fn_t = void (*)(DL_STATE&, EMIT&, dl_value_t);

    constexpr std::array<fn_t, sizeof...(COLS)> fns = 
        { emit_rule<RULE_for_COL<COLS>, EMIT, dl_value_t>... };

    return fns;
}

template <std::size_t...RULES, std::size_t...COLS>
constexpr auto fsm_xlate(std::index_sequence<RULES...>, std::index_sequence<COLS...>)
{
    std::string rule_names[] = { insn_name<meta::at_c<DW_INSNS, RULES>>()... };
    int col_for_rule[] = { COL_for_RULE<meta::at_c<DW_INSNS, RULES>>... };
    std::string rule_for_col[] = { insn_name<RULE_for_COL<COLS>>()... };

    std::cout << "\nCol for Rule" << std::dec << std::endl;
    for (auto n = 0; n < sizeof...(RULES); ++n) {
        std::cout << "Rule " << n << ": " << rule_names[n];
        auto col = col_for_rule[n];
        if (col < NUM_DWARF_LINE_STATES)
            std::cout << " -> " << dl_state_names::value[col] << std::endl;
        else
            std::cout << " -> None (" << std::hex << (int)col << std::dec << ")" << std::endl;
    }

    std::cout << "\nRule for Column" << std::endl;
    for (auto n = 0; n < sizeof...(COLS); ++n) {
        std::cout << "State: " << dl_state_names::value[n] << ": ";
        std::cout << rule_for_col[n] << std::endl;
    }

    std::cout << "\n" << std::dec << std::endl;


}

template <typename EMIT>
void emit_rule(DL_STATE& s, EMIT& emit, unsigned col, DL_STATE::dl_value_t value = true)
{
    constexpr auto fns = emit_fns<EMIT>(std::make_index_sequence<NUM_DWARF_LINE_STATES>());
    return fns[col](s, emit, value);
}


#if 1
template <typename EMIT>
bool gen_pgm(dl_data& d, DL_STATE& s, EMIT& emit)
{
#if 0
    std::cout << "applying dl_data: " << std::dec << d.line_num();
    std::cout << " " << core::core_section::get(d.section());
    std::cout << "+" << std::hex << d.address() << std::dec << std::endl;
#endif   
    // save current "line state info" and update state
    auto old_line_num = s.state[DL_line];
    auto old_op_index = s.state[DL_op_index];
    bool do_emit_addr = false;
    int  line_delta   = 0;

    if (auto n = d.line_num())        // ignore zero values
        line_delta = n - old_line_num;

    // process keywords
    d.do_apply([&](auto key, auto value)
        {
            // ignore key if no change in value
            if (s.state[key] == value)
                return;
            
            // look for special keys
            switch (key)
            {
            case DL_line:
                // override line_delta (didn't fit in dl_data field)
                line_delta = value - old_line_num;
                break;
            case DL_address:
                // DL_address holds segment num. must emit
                do_emit_addr = true;
                // FALLSTHRU
    
            // just record (don't emit) new state
            case DL_end_sequence:
            case DL_op_index:
                s.state[key] = value;
                break;
            default:
                emit_rule(s, emit, key, value); 
                break;
            }
        });

    // test if new segment -> requires EMIT_ADDR
    // XXX refactor when section stored in DL_address key
    if (d.segment() != s.state[DL_address])
    {
        auto& addr_ref = gen_addr_ref(d.segment(), d.address());
        emit_rule<RULE_set_address>(s, emit, expr_t(addr_ref));
        s.state[DL_address] = d.segment();
        s.address           = d.address();
    }

    // convert PC delta to address "units"
    // XXX need to add in op_index logic
    auto addr_delta       = d.address() - s.address;
    auto op_advance_delta = op_advance(addr_delta);

    // check for end-of-sequence flag
    if (s.state[DL_end_sequence]) {
        // end of sequence -- update address & exec rule
        if (op_advance_delta) 
            emit_rule(s, emit, DL_address, op_advance_delta);
        emit_rule(s, emit, DL_end_sequence);
        return true;
    }

    // check if line_delta is out-of-range of the "special" opcode
    // if so, emit "advance line" rule & clear delta
    if (line_delta <   K_LNS_LINE_BASE ||
        line_delta >= (K_LNS_LINE_BASE + K_LNS_LINE_RANGE)) {

        // out-of-range. Emit advance-line opcode
        emit_rule(s, emit, DL_line, line_delta);
        line_delta = 0;
    }

    // NB: op_advance_delta can't be < 0
    // XXX worst case for when max line_delta won't fit with max op_advance
    // XXX for now, just limit to 2*MAX-1
    if (op_advance_delta > (K_LNS_OP_RANGE * 2 - 1)) {
        emit_rule(s, emit, DL_address, op_advance_delta);
        op_advance_delta = 0;
    }
    
    if (line_delta == 0 && op_advance_delta == 0) {
        emit_rule<RULE_copy>(s, emit);
    } else {
        if (op_advance_delta > K_LNS_OP_RANGE) {
            emit_rule<RULE_const_add_pc>(s, emit);
            op_advance_delta -= K_LNS_OP_RANGE;
        }
        emit_rule<RULE_special>(s, emit, op_advance_delta, line_delta);
    }

    return false;
}

#endif


template <typename Inserter>
void dwarf_gen(Inserter&& inserter)
{
    std::cout << __FUNCTION__ << std::endl;
    
    emit_insn emit{inserter};
    DL_STATE state;

    // generate header. Return "end" symbol to be defined after data emited
    auto& end= gen_dwarf_32_header(emit);
    
    // iterate thru dwarf_line data instructions to generate FSM program
    auto gen_line = [&state, &emit](auto& d)
        {
            auto do_init = gen_pgm(d, state, emit);
            if (do_init)
                state.init();
        };
    dl_data::for_each(gen_line);

    // now emit end label.
    emit(end);
}


struct OP_ZERO
{
    using type = OP_ZERO;
    using dl_value_t = typename DL_STATE::dl_value_t;

    template <typename EMIT, typename...FMTS>
    void operator()(DL_STATE& s, EMIT& emit, unsigned col
                  , meta::list<FMTS...>, dl_value_t value = {})
    {
        s.state[col] = {};
    }
};

struct OP_SET
{
    using type = OP_SET;
    using dl_value_t = typename DL_STATE::dl_value_t;
    
    template <typename EMIT, typename T, typename...FMTS>
    void operator()(DL_STATE& s, EMIT& emit, unsigned col, meta::list<FMTS...>, T value = {})
    {
        std::cout << " to " << value;
        if constexpr(std::is_assignable_v<decltype(s.state[col]), T>)
            s.state[col] = value;
        if constexpr (sizeof...(FMTS))
            emit(FMTS{}..., value);
    }
};

struct OP_INVERT
{
    using type = OP_INVERT;
    using dl_value_t = typename DL_STATE::dl_value_t;
    
    template <typename EMIT, typename...FMTS>
    void operator()(DL_STATE& s, EMIT& emit, unsigned col, meta::list<FMTS...>, dl_value_t value)
    {
        // always name this    
        s.state[col] = !s.state[col];
        std::cout << " to " << s.state[col];
    }
};

struct OP_ADVANCE
{
    using type = OP_ADVANCE;
    using dl_value_t = typename DL_STATE::dl_value_t;
    
    template <typename EMIT, typename...FMTS>
    void operator()(DL_STATE& s, EMIT& emit, unsigned col, meta::list<FMTS...>, dl_value_t delta)
    {
        s.state[col] += delta;
        std::cout << ": " << "by " << (int)delta << " to " << s.state[col];
        if constexpr (sizeof...(FMTS))
            emit(FMTS{}..., (int)delta);
    }
};

struct OP_ADD_PC
{
    using type = OP_ADD_PC;
    using dl_value_t = typename DL_STATE::dl_value_t;
    
    template <typename EMIT, typename...FMTS>
    void operator()(DL_STATE& s, EMIT& emit, unsigned col, meta::list<FMTS...>
                                    ,dl_value_t op_adv_delta = K_LNS_OP_RANGE, dl_value_t line_delta = 0) 
    {
#if 0
        // XXX simplify for no op_index
        auto addr_delta  = (s.state[DL_op_index] + op_adv_delta)/K_LNS_MAX_OPS_PER_INSN;
             addr_delta *= addr_delta * K_LNS_MIN_INSN_LENGTH;

        auto new_op_index = (s.state[DL_op_index] + op_adv_delta) % K_LNS_MAX_OPS_PER_INSN;
#else
        auto addr_delta  = op_adv_delta * K_LNS_MIN_INSN_LENGTH;
#endif
        // XXX op-index stuff
        s.address += addr_delta;
        std::cout << " advance " << dl_state_names::value[col];
        std::cout << " by " << addr_delta;
        std::cout << std::hex << " to 0x" << s.address << std::dec;
        
        if (line_delta)
        {
            s.state[DL_line] += line_delta;
            std::cout << " and " << dl_state_names::value[DL_line];
            std::cout << " by " << (int)line_delta << " to " << s.state[DL_line];
        }
        if constexpr (sizeof...(FMTS))
            emit(FMTS{}..., (int)op_adv_delta);
    }
};
}
#endif

