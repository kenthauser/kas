//Usage: m68k-elf-as [option...] [asmfile...]
//Options:

#include "po_defn.h"
#include <list>

std::list<const char *> i_dirs;
const char *o_file;

void kas_gen_options(kas::options::po_defns& defns)
{

    defns.add()
("--alternate"            , "initially turn on alternate macro syntax")
("--MD,:FILE"             , "write dependency information in FILE")
("-I,@DIR,."              , "add DIR to search list for .include directives", i_dirs)
("-nocpp"                 , "ignored")
("-f"                     , "skip whitespace and comment preprocessing")

("-D"                     , "produce assembler debugging messages")
("--debug-prefix-map,@OLD=NEW"
                          , "map OLD to NEW in debug information")
("--defsym,@SYM[=VALUE]"  , "define symbol SYM to given VALUE [default: 1]")
("--execstack"            , "require executable stack for this object")
("--noexecstack"          , "don't require executable stack for this object")
("--sectname-subst"       , "enable section name substitution sequences")
("-g,--gen-debug"         , "generate debugging information")
("--gstabs"               , "generate STABS debugging information")
("--gstabs+"              , "generate STABS debug info with GNU extensions")
("--gdwarf-2"             , "generate DWARF2 debugging information")
("--gdwarf-sections"      , "generate per-function section names for DWARF line information")
("-J"                     , "don't warn about signed overflow")
("-K"                     , "warn when differences altered for long displacements")
("-R"                     , "fold data section into text section")
("-w")
("-X")

("-o,:OBJFILE,a.out"      , "name the object-file output OBJFILE", o_file)
("--reduce-memory-overheads"
                          , "prefer smaller memory use at the cost of longer assembly times")
("--statistics"           , "print various measured statistics from execution")
("--traditional-format"   , "Use same format as native assembler when possible")
("--version"              , "print assembler version number and exit")
("-W,--no-warn"           , "suppress warnings")
("--warn,=0,0"            , "don't suppress warnings")
("--fatal-warnings"       , "treat warnings as errors")
("--hash-size,:VALUE"     , "set the hash table size close to <VALUE>")
("-Z"                     , "generate object file even after errors")
("@FILE,H"                , "read options from FILE")
;
}

void kas_elf_options(kas::options::po_defns& defns)
{
    static bool size_check;
    static const char *zlib;

    defns.add()
("--compress-debug-sections,:, none, zlib, zlib-gnu, zlib-gabi"
                          , "compress DWARF debug sections using zlib", zlib)
("#--compress-debug-sections,--nocompress-debug-sections,=0,0"
                          , "don't compress DWARF debug sections")
("--size-check,:error,error,warning"
			              , "ELF .size directive check", size_check)
("--elf-stt-common"
                          , "generate ELF common symbols with STT_COMMON type")
("-no-pad-sections"       , "do not pad the end of sections to alignment boundaries")
("--strip-local-absolute" , "strip local absolute symbols")
("-L,--keep-locals"       , "keep local symbols (e.g. starting with `L')")
;
}

void kas_listing_options(kas::options::po_defns& defns)
{

    defns.add("Assembler Listing options")
("-a,&,hls"               , "turn on listings")
                     // XXX 	  Sub-options [default hls]:")
("&-a,c,&"                , "omit false conditionals")
("&-a,d,&"                , "omit debugging directives")
("&-a,g,&"                , "include general info")
("&-a,h,&"                , "include high-level source")
("&-a,l,&"                , "include assembly")
("&-a,m,&"                , "include macro expansions")
("&-a,n,&"                , "omit forms processing")
("&-a,s,&"                , "include symbols")
("&-a,=,&:FILE,a.lst"     , "list to FILE (must be last sub-option)")

("--listing-lhs-width,:"  , "set the width in words of the output data column of the listing")
("--listing-lhs-width2,:" , "set the width in words of the continuation lines of the output data column; ignored if smaller than the width of the first line")
("--listing-rhs-width,:"  , "set the max width in characters of the lines from the source file")
("--listing-cont-lines,:" , "set the maximum number of continuation lines used"
                            " for the output data column of the listing")
;
}

struct set_hw_defn
{
    set_hw_defn(int target) : target(target) {}

    const char *operator()(void *cb_arg, const char *value)
    {
        std::cout << "set_hw_defn: " << value;
        std::cout << " target = " << target << std::endl;
        return {};
    }


    int target;    
};

struct cb_yn
{
    cb_yn(const char *arg_name) : arg_name(arg_name) {}

    void operator()(void *cb_arg, const char *value)
    {
        std::cout << "set arg_name: " << arg_name << " to " << value << std::endl;
    }
    
    const char *arg_name;
};

void kas_m68k_options(kas::options::po_defns& defns)
{
    int dummy;

    defns.add("M68K/Coldfire options")
("#-m,-march,:<arch>"	  , "set architecture")
("#-m,-mcpu,:<cpu>,m68020", "set cpu", dummy, set_hw_defn(1))
("-m,!68851"              , "enable/disable  m68k architecture extension"
    , dummy, cb_yn("68851")) 
("-m,!68881"              , "enable/disable  m68k architecture extension")
("-m,!68882"              , "enable/disable  m68k architecture extension")
("-m,!float"              , "enable/disable  architecture extension")
("-m,!div"                , "enable/disable  ColdFire architecture extension")
("-m,!usp"                , "enable/disable  ColdFire architecture extension")
("-m,!mac"                , "enable/disable  ColdFire architecture extension")
("-m,!emac"               , "enable/disable  ColdFire architecture extension")
("-l,:,2"			      , "use 1 word for refs to undefined symbols")
("-pic, -k"		          , "generate position independent code")
("-S"			          , "turn jbsr into jsr")
("--pcrel"                , "never turn PC-relative branches into absolute jumps")
("--register-prefix-optional"
			              , "recognize register names without prefix character")
("--mri,-M"               , "assemble in MRI compatibility mode")
("--bitwise-or"		      , "do not treat `|' as a comment character")
("--base-size-default-16,=0"   , "base reg without size is 16 bits")
("--base-size-default-32,=1,1" , "base reg without size is 32 bits")
("--disp-size-default-16,=0"   , "displacement with unknown size is 16 bits")
("--disp-size-default-32,=1,1" , "displacement with unknown size is 32 bits")
;
}
#ifdef EPILOGUE
//Architecture variants are: 68000 | 68010 | 68020 | 68030 | 68040 | 68060 |
//cpu32 | fidoa | isaa | isaaplus | isab | isac | cfv4 | cfv4e

//Processor variants are: 68000 | 68ec000 | 68hc000 | 68hc001 | 68008 | 68302 |
//68306 | 68307 | 68322 | 68356 | 68010 | 68020 | 68k | 68ec020 | 68030 |
//68ec030 | 68040 | 68ec040 | 68060 | 68ec060 | cpu32 | 68330 | 68331 | 68332 |
//68333 | 68334 | 68336 | 68340 | 68341 | 68349 | 68360 | 51 | 51ac | 51ag |
//51cn | 51em | 51je | 51jf | 51jg | 51jm | 51mm | 51qe | 51qm | 5200 | 5202 |
//5204 | 5206 | 5206e | 5207 | 5208 | 5210a | 5211a | 5211 | 5212 | 5213 | 5214
//| 5216 | 521x | 5221x | 52221 | 52223 | 52230 | 52233 | 52234 | 52235 | 5224
//| 5225 | 52274 | 52277 | 5232 | 5233 | 5234 | 5235 | 523x | 5249 | 5250 |
//5253 | 52252 | 52254 | 52255 | 52256 | 52258 | 52259 | 5270 | 5271 | 5272 |
//5274 | 5275 | 5280 | 5281 | 5282 | 528x | 53011 | 53012 | 53013 | 53014 |
//53015 | 53016 | 53017 | 5307 | 5327 | 5328 | 5329 | 532x | 5372 | 5373 | 537x
//| 5407 | 54410 | 54415 | 54416 | 54417 | 54418 | 54450 | 54451 | 54452 |
//54453 | 54454 | 54455 | 5470 | 5471 | 5472 | 5473 | 5474 | 5475 | 547x | 5480
//| 5481 | 5482 | 5483 | 5484 | 5485 | 548x | fidoa | fido


//Report bugs to <http://www.sourceware.org/bugzilla/>
#endif
