#ifndef KAS_TARGET_TGT_OPC_QUICK_H
#define KAS_TARGET_TGT_OPC_QUICK_H

// Special opcode for the extremely common case of fully resolved
// instructions.
//
// All args are constant (registers or fixed point)
//
// All args must be "written" as power-of-2 multiple of size of opcode (`mcode_size_t).
// NB: the sizeof the arg need not be power-of-2 multiple, but can be emitted
// as multiple power-of-2 writes (eg: m68k extension word with inner/outer expressions)
//
// Note that this `opcode` is instruction-set agnostic. Only the
// `machine-code` size is used.

// Implementation notes:
//
// A new `core::emit_stream` subclass created which stores formatted data in
// the `opcode_data` area for insn. A special `core::emit_base` subclass
// created to trampoline data to/from `emit_stream` subclass.
//
// Data is stored as a sequence of `{width,value}` pairs. The first element
// is always the opcode & has (assumed) width `mcode_size_t`. Additional widths
// are stored dynamically created `width iterator` managed by `detail::quick_arg_iter`.
//
// A single special case of an insn consisting of two `mcode_size_t` values is handled
// specially. This case is especially common and allows two 16-bit values to fit in the
// 32-bit opcode fixed data area. The code to handle this case is found in the co-operating
// methods: `write_quick_data::operator()` & `read_quick_data::operator()`


#include "tgt_opc_quick_detail.h"
#include "kas_core/opcode.h"


namespace kas::tgt::opc
{

// base for `tgt_opc_quick`, dependent on `opcode size` only
template <typename mcode_size_t>
struct tgt_opc_quick_base : core::opcode
{
    using emit_value_t = typename core::emit_base::emit_value_t;
    using iter_t       =  detail::quick_arg_iter<mcode_size_t, emit_value_t>;

    struct write_quick_data 
    {
        write_quick_data(data_t& data) : inserter(data), size(data.size())
            {}
        
        // money function. write `arg`
        void operator()(uint8_t width, emit_value_t value);

        // static trampoline to money function
        static void cb_fn(void *cb_handle, uint8_t width, emit_value_t value)
        {
            (*static_cast<write_quick_data *>(cb_handle))(width, value);
        }

        tgt_data_inserter_t<mcode_size_t, emit_value_t> inserter;
        iter_t  iter;
        int16_t size;
    };
    
    struct read_quick_data
    {
        read_quick_data(data_t const& data) : reader(data), size(data.size())
            {}

        operator bool() const { return size > 0; }
       
        // money function. read `arg`
        std::pair<std::uint8_t, emit_value_t> operator()();
        
        tgt_data_reader_t<mcode_size_t, emit_value_t> reader;
        iter_t  iter;
        int16_t size;
        bool    first_read{true};
    };
    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        auto reader = read_quick_data(data);
        auto sep = "";
        os << std::hex;
        while (reader)
        {
            auto [width, value] = reader();
            os << sep << value;
            sep = " ";
        }
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *) const override
    {
        std::cout << "target_opc_quick" << std::endl;
        auto reader = read_quick_data(data);
        while (reader)
        {
            auto [width, value] = reader();
            base << core::set_size(width) << value;
        }
    }
};

// just a "standard" opcode; not derived from `tgt_opc_base`
template <typename MCODE_T>
struct tgt_opc_quick : tgt_opc_quick_base<typename MCODE_T::mcode_size_t>
{
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using stmt_info_t  = typename MCODE_T::stmt_info_t;
    using base_t       = tgt_opc_quick_base<mcode_size_t>;

    OPC_INDEX();

    using NAME = string::str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_QUICK")>;
    const char *name() const override { return NAME::value; }
   
    template <typename ARGS>
    void proc_args(core::opcode::data_t& data
                 , MCODE_T const& mcode
                 , ARGS& args
                 , stmt_info_t const& info)
    {
        // NB: data.size already set
        auto writer = typename base_t::write_quick_data(data);
        auto base   = detail::quick_base<mcode_size_t>(writer.cb_fn, &writer);
        mcode.emit(base, std::move(args), info);
    }
};

//
// cooperating read/write methods to store quick data as `{width, value}` pairs
//
// NB: since this is a templated implemetation, it can be overridden via a 
// NB: specialization in the proper namespace.
//

template <typename mcode_size_t>
void tgt_opc_quick_base<mcode_size_t>::write_quick_data::operator()
                                            (uint8_t width, emit_value_t value)
{
    // Opcode is first with `sizeof(mcode_size_t)`. Always write w/o iter.
    if (!size && !iter)
        iter = { inserter() };      // construct new width iterator

    // if `width` iter, store width first
    if (iter)
        *iter++ = width;

    // save value
    inserter(value, width);
    
    // allow single arg with width `sizeof(mcode_size_t)` to be written w/o iter
    if (size == 2 * sizeof(mcode_size_t))
        size = sizeof(mcode_size_t);
    else
        size = 0;       // otherwise all args use `iter`
}


template <typename mcode_size_t>
auto tgt_opc_quick_base<mcode_size_t>::read_quick_data::operator()()
    -> std::pair<std::uint8_t, emit_value_t>
{
    // if first read, get opcode
    auto width = sizeof(mcode_size_t);
    if (iter)
        width = *iter++;

    // retrieve data
    auto value = reader.get_fixed(width);
    size -= width;

    // prepare for next cycle if not done
    // no iter if done or if single arg with size `sizeof(mcode_size_t)`
    if (size && !iter)
    {
        if (!first_read || size != sizeof(mcode_size_t))
            iter = { reader.get_fixed_p() };    // construct new width iterator
        first_read = false;
    }
    
    // return result pair
    return { width, value };
}

}

#endif

