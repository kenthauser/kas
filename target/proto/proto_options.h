#ifndef KAS_PROTO_UC_PROTO_UC_OPTIONS_H
#define KAS_PROTO_UC_PROTO_UC_OPTIONS_H

#include "program_options/po_defn.h"
#include "expr/expr_types.h"
#include "PROTO_LC_hw_defns.h"
#include "kas_core/hardware_defns_po.h"

namespace kas::PROTO_LC
{

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
}

namespace kas::expression::detail
{
    // EXAMPLE
    //template <> struct options_types_v<defn_cpu> : meta::id<PROTO_LC::add_PROTO_LC_options> {};
}


#endif
