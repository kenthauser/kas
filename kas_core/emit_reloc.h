#ifndef KAS_CORE_EMIT_RELOC_H
#define KAS_CORE_EMIT_RELOC_H

namespace kas::core
{


enum reloc_type { R_NONE, R_DIRECT, R_PCREL, R_TLS, NUM_RELOC_TYPE };

struct m_reloc
{
    uint8_t value_8, value_16, value_32, value_64;

    auto operator()(std::size_t width = 1) const 
    {
        switch (width)
        {
            case 1: return value_8;
            case 2: return value_16;
            case 4: return value_32;
            case 8: return value_64;

            default:
                throw std::logic_error("m_reloc: bad width");
        }
    }
};

struct relocs
{
    m_reloc r_none, r_dir, r_pcrel;

    auto& operator[](reloc_type t) const
    {
        switch (t) 
        {
            case R_NONE:    return r_none;
            case R_DIRECT:  return r_dir;
            case R_PCREL:   return r_pcrel;
            
            default:
                throw std::logic_error("relocs: bad type");
        }
    }
};



// XXX Temporary until generalized per CPU
// XXX these are M68K reloc values
enum kas_reloc : uint16_t {
          RELOC_NONE
        , RELOC_32
        , RELOC_16
        , RELOC_8
        , RELOC_32PC
        , RELOC_16PC
        , RELOC_8PC
        , RELOC_64
        , RELOC_64PC 
        };

constexpr relocs emit_relocs 
    {
        // none
        {},
        // direct
        { RELOC_8, RELOC_16, RELOC_32 },
        // pcrel
        { RELOC_8PC, RELOC_16PC, RELOC_32PC }
    };



// forward declaration
struct emit_base;


// relocation Abstract Base Class
struct emit_reloc_t
{
    using result_type = void;
    
    emit_reloc_t(std::size_t width = {}) : width(width) {}
    virtual ~emit_reloc_t() = default;

    emit_base& operator<<(expr_t const&);
    emit_base& operator<<(core_expr const&);
    emit_base& operator<<(core_addr const&);
    emit_base& operator<<(core_symbol const&);
    
protected:
    // emit_reloc_t is ABC, must use references
    friend emit_reloc_t&& operator<<(emit_base& b, emit_reloc_t&& r);
    
    friend expr_t;      // for apply_visitor
    friend core_expr;   // participates in emit

    // hooks to customize relocations
    virtual void do_symbol(core_symbol const&) = 0;
    virtual void do_addr(core_addr const&) = 0;

    // catch relocatable types
    void operator()(expr_t      const&);
    void operator()(core_expr   const&);
    void operator()(core_symbol const&);
    void operator()(core_addr   const&);
    
    // pass uninteresting types to `emit_base``
    template <typename T>
    void operator()(T const&);

    // internal support methods
    emit_base& reloc_done() const;
    void put_symbol_reloc (uint32_t reloc, core_symbol const&) const;
    void put_section_reloc(uint32_t reloc, core_section const&) const;

    emit_base  *base_p;
    std::size_t width;
    bool        reloc_complete{};
};

// standard PC relative relocations (displacement)
struct reloc_pcrel : emit_reloc_t
{
    reloc_pcrel(core_expr_dot const& dot, std::size_t width, int64_t offset = {})
        : dot(dot), offset(offset), emit_reloc_t(width) {}
private:
    void do_symbol(core_symbol const&) override;
    void do_addr(core_addr const&) override;
    
    core_expr_dot const& dot;
    int64_t              offset;
};


}
#endif
