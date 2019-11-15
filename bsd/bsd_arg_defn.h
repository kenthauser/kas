#ifndef KAS_BSD_BSD_ARG_DEFN_H
#define KAS_BSD_BSD_ARG_DEFN_H

#include "parser/parser_types.h"
#include "bsd_symbol.h"
#include "kas/kas_string.h"
#include <ostream>

namespace kas::bsd
{

using bsd_arg  = kas_token;
using bsd_args = std::vector<bsd_arg>;

}

#endif

