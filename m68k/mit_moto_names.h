#ifndef KAS_M68K_MIT_MOTO_NAMES_H
#define KAS_M68K_MIT_MOTO_NAMES_H

// Convert IS<> names to actual `const char *` values for m68k processor
//
// Assembler supports two flavors of name: MIT & Motorola
//
// MIT names look like:  movel a2, d0
// The same in Motorola: move.l %a2, %d0
//
// The addressing modes have different formats, but that is handled
// in {mit,moto}_parser_def.h
//
// command-line `options` gen_mit, gen_moto, mit_canonical control names generated.


#include "kas/defn_utils.h"
#include "kas/kas_string.h"

#include "m68k_options.h"

#include "m68k_size_lwb.h"      // code size fn
#include "target/tgt_defn_sizes.h"

namespace kas::m68k::opc
{
struct mit_moto_names
{
    // default: rationalize command-line options
    mit_moto_names()
    {
        // if neither MIT nor MOTO set, generate MOTO)
        gen_mit       = m68k_options.gen_mit;
        gen_moto      = m68k_options.gen_moto;
        mit_canonical = m68k_options.mit_canonical && gen_mit;
        
        if (!gen_mit)
            gen_moto = true;

        // testing
        gen_mit = gen_moto = true;
        mit_canonical = true;
        mit_canonical = false;
    }
   
    // ctor to generate `insn` names
    mit_moto_names(const char *base, std::pair<const char *, const char *> sfxs)
        :  mit_moto_names()
    {
        // initialize in body so can also delegate thru default ctor
        this->base = base;
        this->sfx1 = sfxs.first;
        this->sfx2 = sfxs.second;
#if 0
        std::cout << "mit_moto_names: base = " << base;
        std::cout << " sfx1 = \"" << (sfx1 ? sfx1 : "none") << "\"";
        std::cout << " sfx2 = \"" << (sfx2 ? sfx2 : "none") << "\"";
        std::cout << std::endl;
#endif
    }


    std::string gen_name(bool is_canon, const char *sfx) const
    {
        // SFX_VOID same for both
        if (!*sfx)
            return base;

        // mit omits '.'
        bool is_mit = is_canon == mit_canonical;
        return base + std::string(sfx + is_mit);
    }


    // use iterator to allow easy access to list
    struct iter
    {
        // generate in following order: canonical, moto, mit
        enum { CANON, NON_CANON, DONE };

        iter(mit_moto_names const& obj, bool is_begin = {}) : obj(obj)
        {
            state = is_begin ? CANON : DONE;
        }

        auto operator*() const
        {
            return obj.gen_name(state == CANON, do_sfx_2 ? obj.sfx2 : obj.sfx1);
        }

        auto& operator++() 
        {
            // NEXT: if doing sfx1, see if need to do sfx2
            // only do "void" sfx for canonical (moto & mit are identical)
            if (!do_sfx_2) 
                if (auto sfx2 = obj.sfx2)
                    if (state == CANON || sfx2)
                    {
                        do_sfx_2 = true;
                        return *this;
                    }
            // skip to next state
            do_sfx_2 = false;
            if (state != CANON || !obj.gen_mit || !obj.gen_moto)
            {
                state = DONE;
                return *this;
            }
            state = NON_CANON;

            // since SFX_VOID identical for MIT/MOTO
            // if sfx1 was "void", skip to sfx2
            if (!*obj.sfx1)
            {
                if (obj.sfx2)
                    do_sfx_2 = true;
                else
                    state = DONE;
            }
            return *this;
        }

        bool operator!=(iter const& o) const
        {
            return o.state != state || o.do_sfx_2 != do_sfx_2;
        }

        uint8_t state;
        bool    do_sfx_2{};
        mit_moto_names const& obj;

    };

    iter begin() const { return iter(*this, true); }
    iter end()   const { return iter(*this);       }

    const char *base;
    const char *sfx1;
    const char *sfx2;
    bool gen_moto;
    bool gen_mit;
    bool mit_canonical;
};

}

namespace kas::tgt::opc
{
template <>
auto tgt_defn_sizes<m68k::m68k_mcode_t>::operator()(const char *base_name, uint8_t sz) const
{
    // list of suffixes by size (motorola syntax)
    static constexpr const char *m68k_size_suffixes[] = 
        { 
            ".l",   // 0 LONG
            ".s",   // 1 SINGLE
            ".x",   // 2 XTENDED
            ".p",   // 3 PACKED
            ".w",   // 4 WORD
            ".d",   // 5 DOUBLE
            ".b"    // 6 BYTE
        };

    auto sfx = m68k_size_suffixes[sz];
    return m68k::opc::mit_moto_names(base_name, suffixes(sfx));
}
}
#endif

