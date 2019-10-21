#ifndef KAS_BSD_BSD_ELF_DEFNS_H
#define KAS_BSD_BSD_ELF_DEFNS_H

#include "elf/elf_common.h"

namespace kas::bsd::parser
{

namespace detail
{
    using elf_token_base_t =  x3::symbols<expression::e_fixed_t>;
    struct elf_section_type_x3 : elf_token_base_t
    {
        elf_section_type_x3()
        {
            add
                ("progbits"         , SHT_PROGBITS)
                ("nobits"           , SHT_NOBITS)
                ("note"             , SHT_NOTE)
                ("init_array"       , SHT_INIT_ARRAY)
                ("fini_array"       , SHT_FINI_ARRAY)
                ("preinit_array"    , SHT_PREINIT_ARRAY)
                ;
        }
    };

    struct elf_symbol_type_x3 : elf_token_base_t
    {
        elf_symbol_type_x3()
        {
            add
                ("STT_FUNC"         , STT_FUNC)
                ("function"         , STT_FUNC)
                
                ("STT_GHU_IFUNC"    , STT_GNU_IFUNC)
                ("gnu_indirect_function", STT_GNU_IFUNC)
                
                ("STT_OBJECT"       , STT_OBJECT)
                ("object"           , STT_OBJECT)

                ("STT_TLS"          , STT_TLS)
                ("tls_object"       , STT_TLS)

                ("STT_COMMON"       , STT_COMMON)
                ("common"           , STT_COMMON)

                ("STT_NOTYPE"       , STT_NOTYPE)
                ("notype"           , STT_NOTYPE)

#if 0
                // in gas documentation, but not elf/elf_common.h
                // closest is "binding" STB_GNU_UNIQUE
                ("STT_GNU_UNIQUE"   , STT_GNU_UNIQUE)
                ("gnu_unique_object", STT_GNU_UNIQUE)
#endif
                ;
        }
    };


    
    auto lookup(elf_token_base_t const& p, kas::parser::kas_token const& tok, bool skip_first = true)
    {
        static constexpr expression::e_fixed_t err {-1};
        
        auto  first = tok.begin();
        auto& last  = tok.end();

        // if not skipping '@' character, (eg symbol) require "STT_* format"
        if (!skip_first && *first != 'S')
            return err;

        if (skip_first)
            ++first;

        auto *entry = p.find(std::string(first, last));
        return entry ? *entry : err;
    }
}

auto get_section_type(kas::parser::kas_token const& tok, bool skip_first = true)
{
    return detail::lookup(detail::elf_section_type_x3(), tok, skip_first);
}

auto get_symbol_type(kas::parser::kas_token const& tok, bool skip_first = true)
{
    return detail::lookup(detail::elf_symbol_type_x3(), tok, skip_first);
}

}


#endif
