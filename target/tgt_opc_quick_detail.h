#ifndef KAS_TARGET_TGT_OPC_QUICK_DETAIL_H
#define KAS_TARGET_TGT_OPC_QUICK_DETAIL_H

#include "tgt_data_inserter.h"
#include "kas_core/core_emit.h"
#include "kas_core/emit_stream.h"
#include "kas_core/core_insn.h"


namespace kas::tgt::opc::detail
{

// declare basic constexpr utility
constexpr uint8_t log2(std::size_t n)
{
    return (n < 2) ? 1 : 1 + log2(n/2);
}

template <typename mcode_size_t, typename emit_value_t>
struct quick_arg_iter
{
    // calculate bits to save valid `n` for emit_size_t in emit_value_t
    // NB: zero length writes not allowed, thus `n-1`
    static constexpr auto log_max_n = log2(sizeof(emit_value_t)/sizeof(mcode_size_t));
    static constexpr auto arg_bits  = log_max_n - 1;
    static constexpr auto arg_mask  = (1 << arg_bits) - 1;

    // calculate args of `max_n_bits` in `emit_value_t`
    static constexpr auto max_args  = sizeof(emit_value_t)/arg_bits;

    // this helper type simplifies usage casaes of `iter`
    struct arg_inserter
    {
        arg_inserter(quick_arg_iter& iter) : arg_p(iter.arg_p)
        {
            shift = arg_bits * (max_args - iter.arg_c);
        }

        uint8_t get() const
        {
            auto n = (*arg_p >> shift) & arg_mask;
            return (n+1) * sizeof(mcode_size_t);
        }

        void set(uint8_t n) const
        {
            if constexpr (sizeof(mcode_size_t) > 1)
                n /= sizeof(mcode_size_t);
            --n;
            *arg_p |= (n & arg_mask) << shift;
        }

        auto& operator=(uint8_t n) const
        {
            set(n);
            return *this;
        }

        // support `*iter++ = n`, but not `n = *iter++`;
        auto& operator*() const
        {
            return *this;
        }

        // make `n = *iter++` work
        operator uint8_t() const
        {
            return get();
        }
    private:
        mcode_size_t *arg_p;
        uint8_t       shift;
    };

    quick_arg_iter(mcode_size_t *p = {}) : arg_p(p) {}
  
    // use helper to insert/extract
    auto operator*()
    {
        return arg_inserter(*this);
    }

    // true iff room in `iter`
    operator bool() const { return arg_p && arg_c; }

    // iter `increment` operations
    auto& operator++()    { --arg_c; return *this; }
    auto  operator++(int)
    {   
        auto obj = arg_inserter(*this);
        --arg_c;
        return obj;
    }
private:
    mcode_size_t *arg_p;
    uint8_t       arg_c {max_args};
};


// allow all 
template <typename mcode_size_t>
struct quick_stream : core::emit_stream_base
{
    using e_chan_num   = core::e_chan_num;

    // calculate maximum "chunks" data can occupy
    using emit_value_t  = typename emit_stream_base::emit_value_t;
    using put_uint_fn_t = void(*)(void *cb_handle, uint8_t width, emit_value_t value);

    static constexpr auto max_n = log2(sizeof(emit_value_t)/sizeof(mcode_size_t));

    using iter_t = quick_arg_iter<mcode_size_t, emit_value_t>;

    quick_stream(put_uint_fn_t cb_fn, void *cb_handle)
            : cb_fn(cb_fn), cb_handle(cb_handle)
            {} 

    // define "fixed" data stream method
    void put_uint(e_chan_num num, uint8_t width, emit_value_t data) override
    {
        cb_fn(cb_handle, width, data);
    }

    //
    // error out other backend methods
    //
    void put_raw(e_chan_num num, void const *, uint8_t, unsigned) override
    {
        set_error(__FUNCTION__);        // always error
    }
    
    // emit reloc
    void put_symbol_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , uint8_t offset
            , core::core_symbol_t const& sym
            , int64_t  addend
            ) override
    {
        set_error(__FUNCTION__);
    }
    
    void put_section_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , uint8_t offset
            , core::core_section const& section
            , int64_t  addend
            ) override
    {
        set_error(__FUNCTION__);
    }

    void put_bare_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , uint8_t offset
            , int64_t  addend
            ) override
    {
        set_error(__FUNCTION__);
    }

    // emit diagnostics
    void put_diag(e_chan_num num, uint8_t, parser::kas_diag_t const&) override
    {
        set_error(__FUNCTION__);
    };

    // current section interface
    void set_section(core::core_section const&) override
    {
        set_error(__FUNCTION__);
    };

    std::size_t position() const override
    {
        set_error(__FUNCTION__);
    }

private:
    [[noreturn]] void set_error(const char *fn) const;

    put_uint_fn_t cb_fn;
    void         *cb_handle;
};

// NB: don't inline errors
template <typename mcode_size_t>
void quick_stream<mcode_size_t>::set_error(const char *fn) const
{
    std::string fn_str{fn};
    throw std::logic_error("quick_stream: " + fn_str + " unimplemented");
}

}

#endif

