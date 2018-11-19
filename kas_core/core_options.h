#ifndef KAS_CORE_CORE_OPTIONS_H
#define KAS_CORE_CORE_OPTIONS_H

#include "program_options/po_defn.h"
#include "expr/expr_types.h"

namespace kas::core
{

struct {
    bool    fold_data;

} core_options;

struct {
    const char *file;
    bool    do_listing;
    bool    omit_conditionals;
    bool    omit_debug_directives;
    bool    general_info;
    bool    incl_source;
    bool    incl_assembly;
    bool    incl_macro;
    bool    omit_forms;
    bool    incl_symbols;
    uint8_t lhs_width;
    uint8_t lhs_width2;
    uint8_t rhs_width;
    uint8_t cont_lines;
} listing_options;

struct {
    uint8_t compression;
    uint8_t size_check;
    uint8_t use_stt_common;
    uint8_t no_pad_sections;
    uint8_t strip_absolute;
    uint8_t no_strip_locals;
    uint8_t dwarf_version;
} elf_options;


struct add_core_options
{
    void operator()(options::po_defns& defns) const
    {
        do_core(defns);
        do_elf(defns);
        do_listing(defns);
    }

    void do_core(options::po_defns& defns) const
    {
        auto& o = core_options;
        defns.add()
            ("-R"                     , "fold data section into text section"   , o.fold_data)
            
            // parsed & ignored
            ("--execstack"            , "require executable stack for this object")
            ("--noexecstack"          , "don't require executable stack for this object")
            ("-g,--gen-debug"         , "generate debugging information")
            ("-J"                     , "don't warn about signed overflow")
            ("-K"                     , "warn when differences altered for long displacements")
            ;
    }

    void do_elf(options::po_defns& defns) const
    {
        auto& o = elf_options;
        defns.add("ELF Options")
            ("--compress-debug-sections,:, none, zlib, zlib-gnu, zlib-gabi"
                                  , "compress DWARF debug sections using zlib"  , o.compression)
            ("#--compress-debug-sections,--nocompress-debug-sections,=0,0"
                                      , "don't compress DWARF debug sections"   , o.compression)
            ("--size-check,:error,error,warning"
                                      , "ELF .size directive check"             , o.size_check)
            ("--elf-stt-common"       , "generate ELF common symbols with STT_COMMON type"
                                                                                , o.use_stt_common)
            ("-no-pad-sections"       , "do not pad the end of sections to alignment boundaries"
                                                                                , o.no_pad_sections)
            ("--strip-local-absolute" , "strip local absolute symbols"          , o.strip_absolute)
            ("-L,--keep-locals"       , "keep local symbols (e.g. starting with `L')"
                                                                                , o.no_strip_locals)
            ("--gdwarf-2,=2"          , "generate DWARF2 debugging information" , o.dwarf_version)
            ("--gdwarf-4,=4"          , "generate DWARF2 debugging information" , o.dwarf_version)
            ("--gdwarf,:,4"           , "specify DWARF version"                 , o.dwarf_version)
            
            // parsed & ignored
            ("--gstabs"               , "generate STABS debugging information")
            ("--gstabs+"              , "generate STABS debug info with GNU extensions")
            ("--gdwarf-sections"      , "generate per-function section names for DWARF line information")
            ;
    }

    
    void do_listing(options::po_defns& defns) const
    {
        auto& o = listing_options;
        defns.add("Assembler Listing options")
            ("-a,&,hls"               , "turn on listings", o.do_listing)
                                 // XXX 	  Sub-options [default hls]:")
            ("&-a,c,&"                , "omit false conditionals"   , o.omit_conditionals)
            ("&-a,d,&"                , "omit debugging directives" , o.omit_debug_directives)
            ("&-a,g,&"                , "include general info"      , o.general_info)
            ("&-a,h,&"                , "include high-level source" , o.incl_source) 
            ("&-a,l,&"                , "include assembly"          , o.incl_assembly)
            ("&-a,m,&"                , "include macro expansions"  , o.incl_macro)
            ("&-a,n,&"                , "omit forms processing"     , o.omit_forms)
            ("&-a,s,&"                , "include symbols"           , o.incl_symbols)
            ("&-a,=,&:FILE,a.lst"     , "list to FILE (must be last sub-option)", o.file)

            ("--listing-lhs-width,:"  , "set the width in words of the output data column of the listing"
                                                                    , o.lhs_width)
            ("--listing-lhs-width2,:" , "set the width in words of the continuation lines of the output"
                                        " data column; ignored if smaller than the width of the first line"
                                                                    , o.lhs_width2)
            ("--listing-rhs-width,:"  , "set the max width in characters of the lines from the source file"
                                                                    , o.rhs_width)
            ("--listing-cont-lines,:" , "set the maximum number of continuation lines used"
                                        " for the output data column of the listing"
                                                                    , o.cont_lines)
            ;
    }

};

}

namespace kas::expression::detail
{
    template <> struct options_types_v<defn_core> : meta::id<core::add_core_options> {};
}


#endif
