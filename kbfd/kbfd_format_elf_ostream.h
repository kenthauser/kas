#ifndef KBFD_KBFD_FORMAT_ELF_OSTREAM_H
#define KBFD_KBFD_FORMAT_ELF_OSTREAM_H

namespace kbfd
{

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM& os, std::pair<kbfd_object const&, Elf64_Sym const&> s) 
{
    
    using decode = std::pair<unsigned, const char *>;

    static constexpr decode decode_type[] = {
              { STT_NOTYPE, "NOTYPE" }
            , { STT_OBJECT, "OBJECT" }
            , { STT_FUNC,   "FUNC"   }
            , { STT_SECTION, "SECTION" }
            , { STT_FILE,   "FILE"   }
            , { STT_COMMON, "COMMON" }
            , { STT_TLS,    "TLS"    }
            , { STT_RELC,   "RELC"   }
            , { STT_SRELC,  "SRELC"  }
            , { STT_GNU_IFUNC, "GNU_IFUNC" }
            , {}        // char * == nullptr marks end
        };

    static constexpr decode decode_bind[] = {
              { STB_LOCAL   , "LOCAL"   }
            , { STB_GLOBAL  , "GLOBAL"  }
            , { STB_WEAK    , "WEAK"    }
            , { STB_GNU_UNIQUE, "GNU_UNIQUE" }
            , {}        // char * == nullptr marks end
        };

    static constexpr decode decode_ndx[]  = {
              { SHN_UNDEF   , "UND"     }
            , { SHN_ABS     , "ABS"     }
            , { SHN_COMMON  , "COM"     }
            , { SHN_XINDEX  , "XIN"     }
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
    uint16_t st_type = ELF64_ST_TYPE(sym.st_info);
    uint16_t st_bind = ELF64_ST_BIND(sym.st_info);
   
    str << std::hex << "[elf_sym:";
    print(sym.st_value  , 8, true, nullptr, '0');
    print(sym.st_size   , 4, true);
    print(st_type       , 7, false, decode_type);
    print(st_bind       , 6, false, decode_bind);
    print(sym.st_shndx  , 4, true,  decode_ndx);
    print(obj.sym_name(sym), 16);
    str << "] ";

    return os << str.str();
}

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM& os, std::pair<kbfd_object const&, Elf64_Rel const&> s)
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
    uint16_t r_sym  = ELF64_R_SYM (rel.r_info);
    uint16_t r_type = ELF64_R_TYPE(rel.r_info);
#if 0
    str << std::hex << "[elf_rel:";
    print(rel.r_offset  , 8, true, nullptr, '0');
    print(r_type        , 2, true, nullptr, '0');
    print(obj.fmt.relocs.get_info(r_type),  0);
    print(obj.sym_name(r_sym)        , 16);
    str << "] ";
    return os << str.str();
#else
    return os;
#endif

}

template <typename OSTREAM>
OSTREAM& operator<< (OSTREAM& os, std::pair<kbfd_object const&, Elf64_Rela const&> s)
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
    uint16_t r_sym  = ELF64_R_SYM (rel.r_info);
    uint16_t r_type = ELF64_R_TYPE(rel.r_info);

    str << std::hex << "[elf_rela:";
    print(rel.r_offset  , 8, true, nullptr, '0');
    print(rel.r_addend  , 8, true, nullptr, '0');
    print(r_type        , 2, true, nullptr, '0');
    //print(obj.fmt.relocs.get_info(r_type),  0);
    //print(obj.sym_name(r_sym)        , 16);
    str << "] ";
    return os << str.str();

}


}

#endif
