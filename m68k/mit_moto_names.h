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
// bool's gen_mit, gen_moto, mit_canonical control names generated.


#include "kas/defn_utils.h"
#include "kas/kas_string.h"

#include "m68k_options.h"
#include "m68k_size_defn.h"

namespace kas::m68k::opc
{
namespace mpl = boost::mpl;

#if 0
// configuration constants:
constexpr static bool gen_moto      = true;
constexpr static bool gen_mit       = true;
// constexpr static bool mit_canonical = false;
constexpr static bool mit_canonical = true;

#endif

struct mit_moto_names
{
    // pick up "VOID" suffix tag
    static constexpr auto SFX_VOID = m68k_insn_size::suffix_void;

    mit_moto_names(const char *base, std::pair<char, char> sfxs)
        : base(base), sfx1(sfxs.first), sfx2(sfxs.second)
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


    std::string gen_name(bool is_canon, char sfx) const
    {
        // SFX_VOID same for both
        if (sfx == SFX_VOID)
            return base;

        // create zero terminated char array
        char sfx_str[3] = { '.', sfx };

        // mit omits '.'
        bool is_mit = is_canon == mit_canonical;
        return base + std::string(sfx_str + is_mit);
    }


    // use iterator to allow easy access to list
    struct iter
    {
        // generate in following oreder: canonical, moto, mit
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
                    if (state == CANON || sfx2 != SFX_VOID)
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
            if (obj.sfx1 == SFX_VOID)
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
        uint8_t do_sfx_2{};
        mit_moto_names const& obj;

    };

    iter begin() const { return iter(*this, true); }
    iter end()   const { return iter(*this);       }

    const char *base;
    char sfx1, sfx2;
    bool gen_moto;
    bool gen_mit;
    bool mit_canonical;
};

}

#endif

