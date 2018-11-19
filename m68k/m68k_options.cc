#include "program_options.h"





const char * const m68k_families[] = {
	// format is: family, [members]+, nullptr
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

struct m68k_cpus {
	// m68k_cpus =
	const char * const names = m68k_families;

	const char *begin() {}
	const char *end () = cpus.end();



};




variables_map parse_command_line(int argc, char **argv) {
	const char *usage = "[option...] [asmfile...]";
	options_description desc(usage);

	desc.add_options()
		("--alternate", "initially turn on alternate macro syntax")
		("--", "compress-debug-sections", "compress/don't DWARF debug sections", opt_yn_prefix_tag())
		("-D", "debug", "produce assembler debugging messages", opt_short_only_tag())
		("--defsym", "define symbol SYM to given value", opt_str_arg_list_tag(), nullptr, "SYM=VAL")
		("-f", "fast", "skip whitespace and comment preprocessing", opt_short_only_tag())
		("-g", "--gen-debug", "generate debugging information")
		("--gstabs", "generate STABS debugging information")
		("--gstabs+", "generate STABS debugging information with GNU extensions")
		("--gdwarf-2", "generate DWARG2 debugging information")
		("--hash-size", "set the has table size close to <value>", opt_int_arg_tag(), nullptr, "<value>")
		("--help", "show this message and exit")
		("--target-help", "show the target specific options")
		("-I", "add DIR to search list for .include directives", opt_str_arg_list_tag(), nullptr, "DIR")
		("-J", "ignore-signed-overflow", "don't warn about signed overflow", opt_short_only_tag())
		("-K", "warn-long-displacements", "warn when differences altered for long displacements", opt_short_only_tag())
		("-L", "--keep-locals", "keep local symbols (e.g. starting with `L')")
		("-M", "--mri", "assemble in MRI compatibility mode")
		("--MD", "write dependency information in FILE", opt_str_arg_tag(), nullptr, "FILE")
		("-o", "name the object-file output OBJFILE", opt_str_arg_tag(), "a.out", "OBJFILE")
		("-R", "fold-data", "fold data section into text section", opt_short_only_tag())
		("--reduce-memory-overheads", "prefer smaller memory use")
		("--statistics", "print various measured statistics from execution")
		("--strip-local-absolute", "strip local absolute symbols")
		("--traditional-format", "Use same format as native assembler when possible")
		("--version", "print assembler version number and exit")
		("-W", "--no-warn", "suppress warnings")
		("--warn", "don't suppress warnings")
		("--fatal-warnings", "treat warnings as errors")
		("-w", "ignored")
		("-X", "ignored")
		("-Z", "object-with-errors", "generate object file even after errors", opt_short_only_tag())

		("-pic","generate position independent code")
		;

	options_description listing_opts("Listing generation options");
	listing_opts.add_options()
		("--listing-lhs-width", "set the width in words of the output data column", opt_int_arg_tag())
		("--listing-lhs-width2", "set the width in words of continuation data columns", opt_int_arg_tag())
		("--listing-rhs-width", "set the maximum width of source file", opt_int_arg_tag())
		("--listing-cont-lines", "set the maximum continuation lines", opt_int_arg_tag())

		("-m", "68851", "enable/disable m68k architecture extension", opt_yn_prefix_tag())
		("-m", "68881", "enable/disable m68k architecture extension", opt_yn_prefix_tag())
		("-m", "68882", "enable/disable m68k architecture extension", opt_yn_prefix_tag())
		("-m", "float", "enable/disable architecture extension", opt_yn_prefix_tag())
		("-m", "div",   "enable/disable Coldfile architecture extension", opt_yn_prefix_tag())
		("-m", "usp",   "enable/disable Coldfile architecture extension", opt_yn_prefix_tag())
		("-m", "mac",   "enable/disable Coldfile architecture extension", opt_yn_prefix_tag())
		("-m", "emac",  "enable/disable Coldfile architecture extension", opt_yn_prefix_tag())
		;


	desc.add_group(listing_opts);

	auto vm = desc.parse_command_line(argc, argv);

	std::cout << "variables:\n" << vm << std::endl;

	if (vm["help"])
		std::cout << desc << std::endl;

	const char * output_file = vm["o"];
	int o2 = vm["o"];

	std::cout << "output file: " << output_file << std::endl;
	return vm;
}
