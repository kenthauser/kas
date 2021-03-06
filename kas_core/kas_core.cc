// kas_core/kas_core.cc: instantiate kas_core structures

#include "expr/expr.h"
#include "core_expr_impl.h"
#include "core_expr_fits.h"
#include "core_expr_emit.h"
#include "core_addr.h"
#include "core_insn.h"
#include "core_section.h"           // ostream core_fragment
#include "core_symbol_impl.h"
#include "core_section_impl.h"
#include "core_fragment_impl.h"
#include "core_data_impl.h"
#include "core_emit_impl.h"
#include "core_emit_reloc.h"        // second impl file for `core_emit`
#include "core_reloc_impl.h"
#include "emit_stream_impl.h"
#include "emit_kbfd_impl.h"
#include "emit_kbfd_symbol.h"

#include "dwarf/dwarf_impl.h"

#include "expr/literal_float.h"

namespace kas {
    using namespace expression;
    struct xxx {
        xxx()
        {
            print_type_name{"int"}.name<typename emits_value<int>::type>();
            print_type_name{"kas_error_t"}.name<typename emits_value<kas::parser::kas_error_t>::type>();

            struct t_emits_value
            {
                using emits_value = std::true_type;
            };

            print_type_name{"t_has_emit"}.name<typename emits_value<t_emits_value>::type>();
            // print_type_name{"symbol_ref"}.name<typename emits_value<core::symbol_ref>::type>();
            // print_type_name{"symbol_ref"}.name<typename emits_value<core::symbol_ref>::type>();

            print_type_name{"symbol_ref"}.name<typename emits_value<core::symbol_ref>::type>();
            print_type_name{"addr_ref"}.name<typename emits_value<core::addr_ref>::type>();
            //print_type_name{"missing_ref"}.name<typename emits_value<core::missing_ref>::type>();
        }
    };// _x ;

}

namespace kas::core
{
    std::ostream& operator<<(std::ostream& os, core_symbol_t const& s)
    {
        return os << "[ident " << s.index() - 1 << ":" << s.name() << "]";
    }

    template <typename OS>
    void core_section::print(OS& os) const
    {
        os << name();
    }

    template void core_expr_t  ::print(std::ostream&) const;
    template void core_addr_t  ::print(std::ostream&) const;
    template void core_symbol_t::print(std::ostream&) const;

    template void addr_ref     ::print(std::ostream&) const;
    template void symbol_ref   ::print(std::ostream&) const;
    //template void missing_ref  ::print(std::ostream&) const;

    template void core_section ::print(std::ostream&) const;
    template void core_segment ::print(std::ostream&) const;
    template void core_fragment::print(std::ostream&) const;


    //template std::ostream& operator<<(std::ostream&, core_section const&);
    //template std::ostream& kas::core::operator<<(std::ostream&, core_segment const&);
}

namespace kas::expression::print
{
    // template void print_expr(core::core_addr  const&, std::ostream&);
    // template void print_expr(core::symbol_ref const&, std::ostream&);
    // template void print_expr(core::addr_ref   const&, std::ostream&);
}

namespace kas::dwarf
{   
    template void dwarf_gen(core::insn_inserter_t&&);
}
