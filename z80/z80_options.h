#ifndef KAS_Z80_Z80_OPTIONS_H
#define KAS_Z80_Z80_OPTIONS_H

#include "program_options/po_defn.h"
#include "expr/expr_types.h"
//#include "z80_hw_defns.h"
#include "kas_core/hardware_defns_po.h"

namespace kas::z80
{

struct {
    uint8_t undef_words;
    bool    do_pic;
    bool    use_jsr;
    bool    do_pcrel;
    bool    do_mri;
    bool    bitwise_or;
    bool    basereg_long;
    bool    branch_long;
    uint8_t reg_prefix;
    bool    gen_moto;
    bool    gen_mit;
    bool    mit_canonical;
} z80_options;


struct set_hw_defn
{
    set_hw_defn(int target) : target(target) {}

    const char *operator()(const char *name, void *cb_arg, const char *value)
    {
        std::cout << "set_hw_defn: " << name << " = " << value;
        std::cout << " target = " << target << std::endl;
        return {};
    }


    int target;    
};


const char * const z80_families[] = {
	// format is: arch, [members]+, nullptr
	  "68000", "68000", "68ec000", "68hc000", "68hc001", "68008", "68302", "68306", "68307", "68322", "68356", nullptr
	, "68010", "68010", nullptr
	, "68020", "68020", "68k", "68ec020", nullptr
	, "68030", "68030", "68ec030", nullptr
	, "68040", "68040", "68ec040", nullptr
	, "68060", "68060", "68ec060", nullptr
	, "cpu32", "cpu32", "68330", "68331", "68332", "68333", "68334", "68336", "68340", "68341", "68349", "68360", nullptr
	, "51", "51", "51ac", "51ag", "51em", "51je", "51jf", "51jg", "51mm", "51qe", "51qm", nullptr
	, "5206", "5202", "5204", "5206", nullptr
	, "5206e", "5206e", nullptr
	, "5208", "5207", "5208", nullptr
	, "5211a", "5210a", "5211a", nullptr
	, "5213", "5211", "5212", "5213", nullptr
	, "5216", "5214", "5216", nullptr
	, "52235", "52230", "52231", "52232", "52233", "52234", "52235", nullptr
	, "5225", "5224", "5225", nullptr
	, "52259", "52252", "52254", "52255", "52256", "52258", "52259", nullptr
	, "5235", "5232", "5233", "5234", "5235", "523x", nullptr
	, "5249", "5249", nullptr
	, "5250", "5250", nullptr
	, "5271", "5270", "5271", nullptr
	, "5272", "5272", nullptr
	, "5275", "5274", "5275", nullptr
	, "5282", "5280", "5281", "5282", "528x", nullptr
	, "53017", "53011", "53012", "53013", "53014", "53015", "53016", "53017", nullptr
	, "5307", "5307", nullptr
	, "5329", "5327", "5328", "5329", "532x", nullptr
	, "5373", "5372", "5373", "537x", nullptr
	, "5407", "5407", nullptr
	, "5475", "5470", "5471", "5472", "5473", "5474", "5475", "547x", "5480", "5481", "5482", "5483", "5484", "5485", nullptr
	};

struct add_z80_options
{
    void operator()(options::po_defns& defns) const
    {
    }
};


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
    
}

namespace kas::expression::detail
{
    template <> struct options_types_v<defn_cpu> : meta::id<z80::add_z80_options> {};
}


#endif
