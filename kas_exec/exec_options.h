#ifndef KAS_EXEC_EXEC_OPTIONS_H
#define KAS_EXEC_EXEC_OPTIONS_H

#include "program_options/po_defn.h"

namespace kas::exec
{

struct {
    const char *obj_file;
    int  kas_debug;
    bool statistics;
    bool suppress_warnings;
    bool fatal_warnings;
    bool ignore_error;
} exec_options;

void add_exec_options(kas::options::po_defns& defns)
{
    auto& o = exec_options;

    defns.add()
        ("-D,+"                   , "produce assembler debugging messages"
                                        , o.kas_debug)
        ("-o,:OBJFILE,a.out"      , "name the object-file output OBJFILE"
                                        , o.obj_file)
        ("--statistics"           , "print various measured statistics from execution"
                                        , o.statistics)
        ("-W,--no-warn"           , "suppress warnings"
                                        , o.suppress_warnings)
        ("--warn,=0,0"            , "don't suppress warnings"
                                        , o.suppress_warnings)
        ("--fatal-warnings"       , "treat warnings as errors"
                                        , o.fatal_warnings)
        ("-Z"                     , "generate object file even after errors"
                                        , o.ignore_error)

        // legacy options. parsed & ignored
        ("--reduce-memory-overheads"
                                  , "prefer smaller memory use at the cost"
                                    " of longer assembly times")
        ("--traditional-format"   , "Use same format as native assembler when possible")
        ("--hash-size,:VALUE"     , "set the hash table size close to VALUE")
        ("-w")
        ("-X")
    ;
}
}
#endif
