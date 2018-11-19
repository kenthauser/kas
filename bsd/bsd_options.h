#ifndef KAS_BSD_BSD_OPTIONS
#define KAS_BSD_BSD_OPTIONS


#include "program_options/po_defn.h"
#include "expr/expr_types.h"

#include <list>

namespace kas::bsd
{

struct {
    std::list<const char *> m_include_path;
    std::list<const char *> m_predefined;
    bool    m_alternate;
    bool    m_no_preprocessing;
    bool    m_sectname_subst;

} bsd_options;


struct add_bsd_options
{
    void operator()(options::po_defns& defns) const
    {
        auto& o = bsd_options;
        defns.add()
            ("--alternate"            , "initially turn on alternate macro syntax"  , o.m_alternate)
            ("-I,@DIR,."              , "add DIR to search list for .include directives"
                                                                                    , o.m_include_path)
            ("-I-"                    , "clear .include search list"                , o.m_include_path)
            ("--MD,:FILE"             , "write dependency information in FILE")
            ("-f"                     , "skip whitespace and comment preprocessing" , o.m_no_preprocessing)
            ("--defsym,@SYM[=VALUE]"  , "define symbol SYM to given VALUE [default: 1]"
                                                                                    , o.m_predefined)
            ("--sectname-subst"       , "enable section name substitution sequences", o.m_sectname_subst)
            
            // parsed & ignored
            ("-nocpp")
            ;
    }

};

}

namespace kas::expression::detail
{
    template <> struct options_types_v<defn_fmt> : meta::id<bsd::add_bsd_options> {};
}


#endif
