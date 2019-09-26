#ifndef KAS_CORE_CORE_RELOC_IMPL_H
#define KAS_CORE_CORE_RELOC_IMPL_H

#include "core_reloc.h"

namespace kas::core
{

// XXX M68K relocations here for now

constexpr reloc_info_t m68k_elf_relocs[] =
{
    {  0 , "R_68K_NONE"     , { K_REL_NONE      ,  0, 0 }}
  , {  1 , "R_68K_32"       , { K_REL_ADD       , 32, 0 }}
  , {  2 , "R_68K_16"       , { K_REL_ADD       , 16, 0 }}
  , {  3 , "R_68K_8"        , { K_REL_ADD       ,  8, 0 }}
  , {  4 , "R_68K_32PC"     , { K_REL_ADD       , 32, 1 }}
  , {  5 , "R_68K_16PC"     , { K_REL_ADD       , 16, 1 }}
  , {  6 , "R_68K_8PC"      , { K_REL_ADD       ,  8, 1 }}
  , {  7 , "R_68K_GOT32"    , { K_REL_GOT       , 32, 1 }}
  , {  8 , "R_68K_GOT16"    , { K_REL_GOT       , 16, 1 }}
  , {  9 , "R_68K_GOT8"     , { K_REL_GOT       ,  8, 1 }}
  , { 10 , "R_68K_GOT32O"   , { K_REL_GOT       , 32, 0 }}
  , { 11 , "R_68K_GOT16O"   , { K_REL_GOT       , 16, 0 }}
  , { 12 , "R_68K_GOT8O"    , { K_REL_GOT       ,  8, 0 }}
  , { 13 , "R_68K_PLT32"    , { K_REL_PLT       , 32, 1 }}
  , { 14 , "R_68K_PLT16"    , { K_REL_PLT       , 16, 1 }}
  , { 15 , "R_68K_PLT8"     , { K_REL_PLT       ,  8, 1 }}
  , { 16 , "R_68K_PLT32O"   , { K_REL_PLT       , 32, 0 }}
  , { 17 , "R_68K_PLT16O"   , { K_REL_PLT       , 16, 0 }}
  , { 18 , "R_68K_PLT8O"    , { K_REL_PLT       ,  8, 0 }}
  , { 19 , "R_68K_COPY"     , { K_REL_COPY      , 32, 0 }}
  , { 20 , "R_68K_GLOB_DAT" , { K_REL_GLOB_DAT  , 32, 0 }}
  , { 21 , "R_68K_JMP_SLOT" , { K_REL_JMP_SLOT  , 32, 0 }}
};


static const auto R_OPT_ARM_NC = 1;

constexpr reloc_info_t arm_elf_relocs[] =
{
    {  0 , "R_ARM_NONE"         , { K_REL_NONE      ,  0, 0 }}
  , { 20 , "R_ARM_COPY"         , { K_REL_COPY      , 32, 0 }}
  , { 21 , "R_ARM_GLOB_DAT"     , { K_REL_GLOB_DAT  , 32, 0 }}
  , { 22 , "R_ARM_JMP_SLOT"     , { K_REL_JMP_SLOT  , 32, 0 }}
  , {  2 , "R_ARM_ABS32"        , { K_REL_ADD       , 32, 0 }}
  , {  3 , "R_ARM_REL32"        , { K_REL_ADD       , 32, 1 }}
  , {  5 , "R_ARM_ABS16"        , { K_REL_ADD       , 16, 0 }}
  , {  6 , "R_ARM_ABS12"        , { K_REL_ADD       , 12, 0 }}
  , {  8 , "R_ARM_ABS8"         , { K_REL_ADD       ,  8, 0 }}
  , { 42 , "R_ARM_PREL31"       , { K_REL_ADD       , 32, 0 }}
  , { 43 , "R_ARM_MOVW_ABS_NC"  , { ARM_REL_MOVW    , 32, 0  }}
  , { 44 , "R_ARM_MOVT_ABS"     , { ARM_REL_MOVT    , 32, 0  }}
  , { 45 , "R_ARM_MOVW_PCREL_NC", { ARM_REL_MOVW    , 32, 1  }}
  , { 46 , "R_ARM_MOVT_PCREL"   , { ARM_REL_MOVT    , 32, 1  }}

#if 0
  , {  4 , "R_ARM_LDR_PC_G0"    , { K_REL_ADD       , 32, 1 }}
  , {  7 , "R_ARM_THM_ABS5"     , { K_REL_GOT       , 32, 1 }}
  , {  9 , "R_ARM_SBREL32"      , { K_REL_GOT       ,  8, 1 }}
  , { 10 , "R_ARM_THM_CALL"     , { K_REL_GOT       , 32, 0 }}
  , { 11 , "R_ARM_THM_PC8"      , { K_REL_GOT       , 16, 0 }}
  , { 12 , "R_ARM_BREL_ADJ"     , { K_REL_GOT       ,  8, 0 }}
  , { 13 , "R_ARM_TLS_DESC"     , { K_REL_PLT       , 32, 1 }}
//  , {  1 , "R_ARM_PC24"       , { K_REL_ADD   , 32, 0 }}
//  , { 14 , "R_ARM_THM_SWI8"    , { K_REL_PLT       , 16, 1 }}
//  , { 15 , "R_ARM_XPC25"     , { K_REL_PLT       ,  8, 1 }}
//  , { 16 , "R_ARM_THM_XPC22"   , { K_REL_PLT       , 32, 0 }}
  , { 17 , "R_ARM_TLS_DTPMOD32" , { K_REL_PLT       , 16, 0 }}
  , { 18 , "R_ARM_TLS_DTPOFF32" , { K_REL_PLT       ,  8, 0 }}
  , { 19 , "R_ARM_TLS_TPOFF32"  , { K_REL_COPY      , 32, 0 }}
  , { 23 , "R_ARM_RELATIVE"     , { K_REL_ADD  , 32, 0 }}
  , { 24 , "R_ARM_GOTOFF32"     , { K_REL_ADD  , 32, 0 }}
  , { 25 , "R_ARM_BASE_PREL"    , { K_REL_ADD  , 32, 0 }}
  , { 26 , "R_ARM_GOT_BREL"     , { K_REL_ADD  , 32, 0 }}
  , { 27 , "R_ARM_PLT32"        , { K_REL_ADD  , 32, 0 }}
  , { 28 , "R_ARM_CALL"         , { K_REL_ADD  , 32, 0 }}
  , { 29 , "R_ARM_JMP24"        , { K_REL_ADD  , 32, 0 }}
  , { 30 , "R_ARM_THM_JUMP24"   , { K_REL_ADD  , 32, 0 }}
  , { 31 , "R_ARM_BASE_ABS"     , { K_REL_ADD  , 32, 0 }}
#endif
};

int64_t read_4_12(int64_t data)
{
    auto value = (data >> 4) & 0xf000;
    return value | (data & 0xfff);
}

int64_t write_4_12(int64_t data, int64_t value)
{
    auto mask = 0xf0fff;
    data  &=~ mask;
    value |= (value & 0xf000) << 4;
    return data | (value & mask);
}



constexpr reloc_op_t kas_reloc_ops [] =
{
     { K_REL_ADD    , reloc_op_t::update_add }
   , { K_REL_SUB    , reloc_op_t::update_sub }
   , { K_REL_COPY   , reloc_op_t::update_set }
   , { ARM_REL_MOVW , reloc_op_t::update_add, write_4_12, read_4_12 }
};

auto deferred_reloc_t::get_ops(uint8_t reloc_op) const -> reloc_op_t const *
{
    using vector_t = std::vector<reloc_op_t const *>;
    static vector_t *vec_p;

    if (!vec_p)
    {
        vec_p = new vector_t;
        for (auto& op : kas_reloc_ops)
        {
            if (op.op >= vec_p->size())
                vec_p->resize(op.op + 10);  // don't extend each loop
            (*vec_p)[op.op] = &op;
        }
    }
    if (reloc_op < vec_p->size())
        return (*vec_p)[reloc_op];
    return {};
}



auto deferred_reloc_t::get_info(emit_base& base) const -> reloc_info_t const *
{
    // get type of `reloc` hash key & declare map
    using key_t = decltype(std::declval<core_reloc>().key());
    using map_t = std::map<key_t, reloc_info_t const *>;

    // create `std::map` of `core_reloc` -> `reloc_info_t` relationship
    static map_t *map_p;
    if (!map_p)
    {
        map_p = new map_t;
        for (auto& info : m68k_elf_relocs)
            map_p->emplace(info.reloc.key(), &info);
    }

    // if unknown "op", return not found
    if (!get_ops(reloc.reloc))
        return nullptr;
        
    // convert `reloc` to target machine `info`
    auto iter = map_p->find(reloc.key());       // lookup current relocation

    // return pointer to info if found
    return iter != map_p->end() ? iter->second : nullptr;
}

// complete construction of `reloc`
void deferred_reloc_t::operator()(expr_t const& e)
{
    if (!loc_p)
        loc_p = e.get_loc_p();      // if `tagged` store location

    // use `if tree` to initialize relocation
    // don't need `apply_visitor` as most types don't relocate
    if (auto p = e.get_fixed_p())
        addend += *p;
    else if (auto p = e.template get_p<core_symbol_t>())
        (*this)(*p);
    else if (auto p = e.template get_p<core_expr_t>())
        (*this)(*p);
    else if (auto p = e.template get_p<core_addr_t>())
        (*this)(*p);
    else
    {
        std::cout << "deferred_reloc_t::operator(): unsupported type" << std::endl;
    }
}

// symbols can `vary`. Sort by type of symbol
void deferred_reloc_t::operator()(core_symbol_t const& value)
{
    // see if resolved symbol
    if (auto p = value.addr_p())
        (*this)(*p);

    // if `EQU`, interpret value
    else if (auto p = value.value_p())
        (*this)(*p);

    // otherwise, unresolved symbol
    else
        sym_p = &value;
}

void deferred_reloc_t::operator()(core_addr_t const& value)
{
    addend   +=  value.offset()();
    section_p = &value.section();
}

void deferred_reloc_t::operator()(core_expr_t const& value)
{
    if (auto p = value.get_fixed_p())
        addend += *p;
    else
        core_expr_p = &value;
}

void deferred_reloc_t::emit(emit_base& base)
{
    // if symbol, reinterpret before emitting
    if (sym_p)
    {
        auto& sym = *sym_p;
        sym_p = {};
        (*this)(sym);
    }

    // if no width specified by reloc, use current width
    if (reloc.bits == 0)
        reloc.bits = base.width * 8;
    
    // absorb section_p if PC_REL && matches
    // NB: could be done in `add`, but `deferred_reloc_t` doesn't know `base`
    if (section_p == &base.get_section())
        if (reloc.flags & core_reloc::RFLAGS_PC_REL)
        {
            section_p    = {};
            addend      -= base.position();
            reloc.flags &=~ core_reloc::RFLAGS_PC_REL;
        }
   
    // get pointer to machine-specific info matching `reloc`
    auto info_p = get_info(base);

    bool use_rela = false;
//    use_rela = true;
    int64_t base_addend = {};
    if (!use_rela)
    {
        base_addend = addend;
        addend = {};
    }
   
    // emit relocations to backend
    if (section_p)
        base.put_section_reloc(*this, info_p, *section_p, addend);
    else if (sym_p)
        base.put_symbol_reloc(*this, info_p, *sym_p, addend);
    else if (core_expr_p)
        core_expr_p->emit(base, *this);
    else if (addend)
        base_addend = addend;
    addend = {};

    // if "base_addend", apply relocation to base
    if (base_addend)
        apply_reloc(base, base_addend);
}

// Apply `reloc_fn`: deal with offsets & width deltas
void deferred_reloc_t::apply_reloc(emit_base& base, int64_t& addend)
{
    auto read_subfield = [&](int64_t data) -> std::tuple<int64_t, uint64_t, uint8_t>
        {
            uint64_t mask  {};
            uint8_t  shift {};
            auto     width = reloc.bits / 8;
          
            if (base.width < (width + offset))
                throw std::logic_error { "apply_reloc: invalid width/offset" };

            // normal case: matching widths
            if (base.width == width)
                return { data, 0, 0 };

            // constexpr to calculate mask
            mask  = (width == 4) ? 0xffff'ffff : (width == 2) ? 0xffff : (width == 1) ? 0xff : ~0; 
            shift = (base.width - offset - 1) * 8;
           
            // apply offset & delta width operations
            mask <<= shift;
            data = (data & mask) >> shift;

            // sign-extend data
            switch(width)
            {
                case 1: data = static_cast<int8_t >(data); break;
                case 2: data = static_cast<int16_t>(data); break;
                case 4: data = static_cast<int32_t>(data); break;
                default: break;
            }
            return { data, mask, shift };
        };

    auto write_subfield = [](auto base, auto value, auto mask, auto shift)
                -> decltype(base)
        {
            // normal case. matching widths
            if (!mask) return value;

            // remove result bits from original data & insert shifted value
            base &=~ mask;
            base |=  mask & (value << shift);
            return base;
        };

    // read data as subfield
    auto [data, mask, shift] = read_subfield(base.data);

    // working copy of data is base `addend`
    auto value = data;
    
    // retrieve reloc methods
    auto& ops = *get_ops(reloc.reloc);
    
    // if `read_fn` extract addend from `data`
    if (ops.read_fn)
        value = ops.read_fn(value);

    // apply `update_fn` on value
    value = ops.update_fn(value, addend);

    // if `write_fn` update extracted data
    if (ops.write_fn)
        value = ops.write_fn(data, value);

    // insert new `data` as subfield
    base.data = write_subfield(base.data, value, mask, shift);
};

// static method
// return true iff `relocs` emited OK
bool deferred_reloc_t::done(emit_base& base) { return true; }


}

#endif
