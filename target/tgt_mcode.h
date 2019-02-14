#ifndef KAS_TARGET_TGT_MCODE_H
#define KAS_TARGET_TGT_MCODE_H


namespace kas::tgt::opc
{
// forward declare templates
template <typename MCODE_T> struct tgt_validate;
template <typename MCODE_T> struct tgt_format;
template <typename MCODE_T> struct tgt_validate_args;
template <typename MCODE_T> struct tgt_insn_defn;
template <typename MCODE_T> struct tgt_insn_adder;

}
// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::tgt
{

using namespace meta;
using expression::expr_fits;
using expression::fits_result;

// declare base size types
struct tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS        = 2;
    static constexpr auto MAX_MCODE_WORDS = 2;

    //using mcode_size_t = void;          // must specify machine code size
    using mcode_size_t = uint8_t;
    using mcode_idx_t  = uint8_t;
    using defn_idx_t   = uint8_t;
    using name_idx_t   = uint8_t;
    using fmt_idx_t    = uint8_t;
    using val_idx_t    = uint8_t;
    using val_c_idx_t  = uint8_t;
};


template <typename MCODE_T, typename STMT_T, typename ERR_MSG_T, typename SIZE_T = tgt_mcode_size_t>
struct tgt_mcode_t
{
    // CRTP types
    using base_t    = tgt_mcode_t;
    using derived_t = MCODE_T;
    
    // save template variables
    using mcode_t      = MCODE_T;
    using stmt_t       = STMT_T;
    using err_msg_t    = ERR_MSG_T;

    // extract types from STMT
    using insn_t       = typename stmt_t::insn_t;
    using arg_t        = typename stmt_t::arg_t;
    using stmt_args_t  = decltype(stmt_t::args);
    
    using bitset_t     = typename insn_t::bitset_t;
    static_assert(std::is_same_v<derived_t, typename insn_t::mcode_t>);

    // convenience type references
    using op_size_t    = typename core::opcode::op_size_t;

    // supporting types
    using fmt_t     = opc::tgt_format       <MCODE_T>;
    using val_t     = opc::tgt_validate     <MCODE_T>;
    using val_c_t   = opc::tgt_validate_args<MCODE_T>;
    using defn_t    = opc::tgt_insn_defn    <MCODE_T>;
    using adder_t   = opc::tgt_insn_adder   <MCODE_T>;

    // override sizes in `SIZE_T` if required
    using mcode_size_t = typename SIZE_T::mcode_size_t;
    using mcode_idx_t  = typename SIZE_T::mcode_idx_t;
    using defn_idx_t   = typename SIZE_T::defn_idx_t;
    using name_idx_t   = typename SIZE_T::name_idx_t;
    using fmt_idx_t    = typename SIZE_T::fmt_idx_t;
    using val_idx_t    = typename SIZE_T::val_idx_t;
    using val_c_idx_t  = typename SIZE_T::val_c_idx_t;
    
    static constexpr auto MAX_ARGS        = SIZE_T::MAX_ARGS;
    static constexpr auto MAX_MCODE_WORDS = SIZE_T::MAX_MCODE_WORDS;

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
        : index(index), defn_index(defn_index)
        {}

    // declare as template to defer definition of `ARGS_T`
    // validate arg count & arg_modes are supported on run-time target
    template <typename ARGS_T>
    const char * validate_args (ARGS_T& args, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    fits_result size(ARGS_T& args, op_size_t& size, expr_fits const&, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    void emit(core::emit_base&, mcode_size_t*, ARGS_T&&, core::core_expr_dot const * = {}) const;

    // retrieve instance from index
    static auto& get(uint16_t idx) { return (*index_base)[idx]; }

    // retrieve support types
    defn_t  const& defn()  const;
    fmt_t   const& fmt()   const;
    val_c_t const& vals()  const;
    auto  sz()    const { return 0; }
    std::string name() const;

    // machine code size in bytes (for format & serialize)
    uint8_t code_size() const;

    // machine code "base" size used in calculation
    auto base_size() const
    {
        return derived().code_size();
    }
    
    // machine code arranged as words: big-endian
    auto code() const -> std::array<mcode_size_t, MAX_MCODE_WORDS>;

    mcode_idx_t index;         // -> access this instance (zero-based)
    defn_idx_t  defn_index;    // -> access associated defn for name, fmt, validator (zero-based)
};

}
#endif
