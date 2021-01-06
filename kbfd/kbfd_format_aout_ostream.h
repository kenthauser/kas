#ifndef KBFD_KBFD_FORMAT_AOUT_OSTREAM_H
#define KBFD_KBFD_FORMAT_AOUT_OSTREAM_H

#include "aout_gnu.h"

namespace kbfd
{

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM& os, std::pair<kbfd_object const&, nlist const&> s) 
{
    
    using decode = std::pair<unsigned, const char *>;

    static constexpr decode decode_type[] = {
              { N_UNDF          , "UNDF"     }
            , { N_UNDF + N_EXT  , "UNDF EXT" }
            , { N_ABS           , "ABS"      }
            , { N_ABS  + N_EXT  , "ABS  EXT" }
            , { N_TEXT          , "TEXT"     }
            , { N_TEXT + N_EXT  , "TEXT EXT" }
            , { N_DATA          , "DATA"     }
            , { N_DATA + N_EXT  , "DATA EXT" }
            , { N_BSS           , "BSS"      }
            , { N_BSS  + N_EXT  , "BSS  EXT" }
            , { N_COMM          , "COMM"     }
            , { N_FN            , "FILE"     }
            , { N_TYPE          , "TYPE"     }
            , { N_STAB          , "STAB"     }
            , { N_INDR          , "INDR"     }
            , { N_SETA          , "SETA"     }
            , { N_SETT          , "SETT"     }
            , { N_SETD          , "SETD"     }
            , { N_SETB          , "SETB"     }
            , { N_SETV          , "SETV"     }
            , { N_WARNING       , "WARN"     }
            , { N_WEAKU         , "WEAKU"    }
            , { N_WEAKA         , "WEAKA"    }
            , { N_WEAKT         , "WEAKT"    }
            , { N_WEAKD         , "WEAKD"    }
            , { N_WEAKB         , "WEAKB"    }
            , {}        // char * == nullptr marks end
        };


    std::ostringstream str;
    auto print = [&str](auto value, uint8_t width = 0, bool right = false
                      , decode const *dp = nullptr, char fill = ' ')
        {
            // set stream manipulators
            str << ' ' << std::setw(width) << std::setfill(fill);
            if (right)
                str << std::right;
            else
                str << std::left;

            // process "decode" for integral types
            if constexpr (std::is_integral_v<decltype(value)>)
                if (dp)
                    for (; dp->second; ++dp)
                        if (value == dp->first)
                            return (void)(str << dp->second);

            // if decode didn't match, just emit to stream
            str << value;
        };


    auto&    obj     = s.first;
    auto&    sym     = s.second;
    
    str << std::hex << "[aout_sym:";
    print(sym.n_value  , 8, true, nullptr, '0');
    print(+sym.n_type   , 8, false, decode_type);

    auto  symtab_p = obj.symtab_p;
    if (symtab_p)
    {
        auto& str_tab = obj.symtab_p->strtab();
        if (auto idx = sym.n_un.n_strx)
            print (str_tab.begin() + idx - 4, 16);
        else
            print ("[unnamed]", 16);
    }
    else
        print("[null symtab_p]", 16);

    str << "] ";

    return os << str.str();
}


template <typename OS>
OS& operator<< (OS& os, std::pair<kbfd_object const&, relocation_info const&> s) 
{
    using decode = std::pair<unsigned, const char *>;
   
    std::ostringstream str;
    auto print = [&str](auto value, uint8_t width = 0, bool right = false
                      , decode const *dp = nullptr, char fill = ' ')
        {
            // set stream manipulators
            str << ' ' << std::setw(width) << std::setfill(fill);
            if (right)
                str << std::right;
            else
                str << std::left;

            // process "decode" for integral types
            if constexpr (std::is_integral_v<decltype(value)>)
                if (dp)
                    for (; dp->second; ++dp)
                        if (value == dp->first)
                            return (void)(str << dp->second);

            // if decode didn't match, just emit to stream
            str << value;
        };

    auto&    obj     = s.first;
    auto&    rel     = s.second;
    
    str << std::hex << "[aout_rel:";
    print(rel.r_address                   , 8, true, nullptr, '0');
    print(std::to_string(rel.r_length * 8), 2, true);
    print((rel.r_pcrel ? "PC" : "")       , 2);

    static constexpr auto NAME_WIDTH = 16;
    if (rel.r_extern)
        print(obj.sym_name(rel.r_symbolnum), NAME_WIDTH);
    else switch (rel.r_symbolnum)
    {
        case N_TEXT:    print("N_TEXT", NAME_WIDTH); break;
        case N_DATA:    print("N_DATA", NAME_WIDTH); break;
        case N_BSS:     print("N_BSS",  NAME_WIDTH); break;
        case N_ABS:     print("N_ABS",  NAME_WIDTH); break;
        default:        print(rel.r_symbolnum, NAME_WIDTH); break;
    }

    str << "] ";

    return os << str.str();
}

#if 0
// No RELA in a.out
template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM& os, Elf64_Rela const& rel);
#endif

}

#endif
