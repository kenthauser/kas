#ifndef KAS_TARGET_TGT_MCODE_H
#define KAS_TARGET_TGT_MCODE_H


#include "expr/expr.h"
#include "kas/kas_string.h"

#include "kas_core/core_emit.h"
#include "kas_core/core_fits.h"

namespace kas::tgt::opc
{
// forward declare templates
template <typename MCODE_T> struct tgt_validate;
template <typename MCODE_T> struct tgt_format;
template <typename MCODE_T> struct tgt_info_fn_t;
template <typename MCODE_T> struct tgt_validate_args;
template <typename MCODE_T> struct tgt_defn;
template <typename MCODE_T> struct tgt_defn_adder;
template <typename MCODE_T> struct tgt_opc_base;
template <typename MCODE_T> struct tgt_mcode_defn;
template <typename MOCDE_T> struct tgt_mcode_sizes;
template <typename MCODE_T> struct tgt_size;
}
// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::tgt
{

using namespace meta;
using expression::expr_fits;
using expression::fits_result;

// default defn_info: just hold arg size & info_fn (or nothing)
template <typename MCODE_T
        , typename VALUE_T = uint8_t, unsigned FN_BITS = 4>
struct tgt_defn_info_t
{
    static constexpr auto INFO_T_BITS = std::numeric_limits<VALUE_T>::digits;
    static constexpr auto VALUE_BITS  = INFO_T_BITS - FN_BITS;
    
    using defn_t       = typename MCODE_T::defn_t;
    using stmt_info_t  = typename MCODE_T::stmt_info_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using code_t       = typename MCODE_T::code_t;

    constexpr tgt_defn_info_t(VALUE_T v = 0, VALUE_T fn_idx = 0)
        : value(v), fn_idx(fn_idx) {}

    // insert/extract stmt_info values from `code`
    void        insert (code_t&, stmt_info_t const&) const;
    stmt_info_t extract(mcode_size_t const *code_p)  const;
    uint8_t     sz     (stmt_info_t const&)          const;
    code_t      mask   (MCODE_T const&)              const;

    VALUE_T operator()() const { return value; }
    
//private:
    VALUE_T value  : INFO_T_BITS;
    VALUE_T fn_idx : FN_BITS;
};

// declare base size types
struct tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS        = 2;
    static constexpr auto MAX_MCODE_WORDS = 2;
    static constexpr auto NUM_ARCHS       = 1;

    // set default code size to `e_data_t`
    using mcode_size_t  = expression::e_data_t;
    using code_defn_t   = uint32_t;

    // index types reflect number of mcodes, fmts, etc...
    // commonly overridden as result of compile error..
    using mcode_idx_t   = uint8_t;
    using defn_idx_t    = uint8_t;
    using name_idx_t    = uint8_t;
    using fmt_idx_t     = uint8_t;
    using val_idx_t     = uint8_t;
    using val_c_idx_t   = uint8_t;
   
    // defaults for `tgt_defn_info`
    using defn_info_value_t = uint8_t;
    static constexpr auto defn_info_fn_bits = 4;
};

template <typename MCODE_T> struct tgt_size { };

template <typename MCODE_T, typename STMT_T, typename ERR_MSG_T, typename SIZE_T = tgt_mcode_size_t>
struct tgt_mcode_t
{
    using BASE_NAME = KAS_STRING("TGT");

    // CRTP types
    using base_t    = tgt_mcode_t;
    using derived_t = MCODE_T;
    
    // save template variables
    using mcode_t      = MCODE_T;
    using stmt_t       = STMT_T;
    using err_msg_t    = ERR_MSG_T;
    using size_trait_t = SIZE_T;

    // extract types from STMT
    using insn_t       = typename stmt_t::insn_t;
    using arg_t        = typename stmt_t::arg_t;
    using argv_t       = typename stmt_t::argv_t;
    using stmt_info_t  = typename stmt_t::info_t;
    using stmt_args_t  = decltype(stmt_t::args);

    using arg_mode_t   = typename arg_t::arg_mode_t;
    using reg_t        = typename arg_t::reg_t;
    using regset_t     = typename arg_t::regset_t;
    
    using ok_bitset_t  = typename insn_t::ok_bitset_t;
    static_assert(std::is_same_v<derived_t, typename insn_t::mcode_t>);

    using hw_tst       = typename insn_t::hw_tst;
    
    // convenience type references
    using op_size_t     = typename core::opcode::op_size_t;

    // supporting types
    using fmt_t         = opc::tgt_format       <MCODE_T>;
    using info_fn_t     = opc::tgt_info_fn_t    <MCODE_T>;
    using val_t         = opc::tgt_validate     <MCODE_T>;
    using val_c_t       = opc::tgt_validate_args<MCODE_T>;
    using defn_t        = opc::tgt_mcode_defn   <MCODE_T>;
    using adder_t       = opc::tgt_defn_adder   <MCODE_T>;
    using opcode_t      = opc::tgt_opc_base     <MCODE_T>;
    using mcode_sizes_t = opc::tgt_mcode_sizes  <MCODE_T>;

    // declare "default" fomatter
    using fmt_default  = opc::tgt_fmt_opc_gen   <MCODE_T>;
    
    // declare "default" code size_fn
    using code_size_t  = void;      // specified in `MCODE_T` if needed

    // override sizes in `SIZE_T` if required
    using mcode_size_t = typename SIZE_T::mcode_size_t;
    using mcode_idx_t  = typename SIZE_T::mcode_idx_t;
    using code_defn_t  = typename SIZE_T::code_defn_t;
    using defn_idx_t   = typename SIZE_T::defn_idx_t;
    using name_idx_t   = typename SIZE_T::name_idx_t;
    using fmt_idx_t    = typename SIZE_T::fmt_idx_t;
    using val_idx_t    = typename SIZE_T::val_idx_t;
    using val_c_idx_t  = typename SIZE_T::val_c_idx_t;

    // get `defn_info` template definitions
    static constexpr auto defn_info_fn_bits = SIZE_T::defn_info_fn_bits;
    using defn_info_value_t  = typename SIZE_T::defn_info_value_t;
    using defn_info_t   = tgt_defn_info_t<MCODE_T
                                        , defn_info_value_t
                                        , defn_info_fn_bits>;
    
    static constexpr auto MAX_ARGS        = SIZE_T::MAX_ARGS;
    static constexpr auto MAX_MCODE_WORDS = SIZE_T::MAX_MCODE_WORDS;
    static constexpr auto NUM_ARCHS       = SIZE_T::NUM_ARCHS;

    // declare "code" object type
    using code_t = std::array<mcode_size_t, MAX_MCODE_WORDS>;

    // allocate instances in `std::deque`
    using obstack_t = std::deque<derived_t>;
    static inline const obstack_t *index_base;
    static inline const defn_t    *defns_base;

    // CRTP casts
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto constexpr& derived()
        { return *static_cast<derived_t*>(this); }

    // constructor: just save indexes
    tgt_mcode_t(mcode_idx_t index, defn_idx_t defn_index)
        : index{index}, defn_index{defn_index}
        {}

    // declare as template to defer definition of `ARGS_T`
    // validate arg count & arg_modes are supported on run-time target
    template <typename ARGS_T>
    std::pair<const char *, int> validate_mcode (ARGS_T& args, stmt_info_t const& info, std::ostream *trace = {}) const;

    // calculate total size of opcode + args
    // return `fits_result` based if immed size, branch displacement, goes out of range
    fits_result size(argv_t& args, stmt_info_t const& info, op_size_t& size, expr_fits const&, std::ostream *trace = {}) const;

    // emit method: allow override of sub-methods
#if 1
    void emit(core::core_emit&, argv_t&, stmt_info_t const& info) const;
#else  
    template <typename ARGS_T>
    void emit(core::core_emit&, ARGS_T&&, stmt_info_t const& info) const;
#endif
   
    // apply args to initial binary code
    template <typename ARGS_T>
    void emit_args(core::core_emit&, ARGS_T&&, stmt_info_t const& info) const;
    
    // emit base binary code
    template <typename ARGS_T>
    void emit_base(core::core_emit&, ARGS_T&&, stmt_info_t const& info) const;
    
    // emit immediate info after base code
    template <typename ARGS_T>
    void emit_immed(core::core_emit&, ARGS_T&&, stmt_info_t const& info) const;
    
    // retrieve instance from index
    static auto& get(uint16_t idx) { return (*index_base)[idx]; }

    // retrieve support types
    defn_t     const& defn() const;
    fmt_t      const& fmt()  const;
    val_c_t    const& vals() const;
    std::string       name() const;

    // determine `arch` for `defn_t`
    constexpr uint8_t defn_arch() const
    {
        return 0;       // default: single arch
    }

    // machine code size in bytes (for format & serialize)
    uint8_t code_size() const;

    // machine code "base" size used in total size calculation (defaults to `code_size`)
    // can include prefixed from INFO, etc
    auto base_size() const
    {
        return derived().code_size();
    }

    // calculate branch_mode (eg: branch BYTE, WORD, LONG, etc) from insn size
    // return appropriate `MODE` for branch argument
    uint8_t calc_branch_mode(uint8_t size) const
    {
        return arg_mode_t::MODE_BRANCH;
    }
    
    // machine code arranged as words: big-endian array (ie highest order words first)
    code_t code(stmt_info_t const& stmt_info) const;
    stmt_info_t extract_info(mcode_size_t const *) const;

    void print(std::ostream&) const;

    mcode_idx_t index;         // -> access this instance (zero-based)
    mcode_idx_t insn_index;    // -> access canonical instruction index (1-based)
    defn_idx_t  defn_index;    // -> access associated defn for name, fmt, etc (zero-based)
};

}
#endif
