#include "kbfd.h"

// enumerate architectures
#include "target/m68k.h"

// define `reloc_ops` to allow targets to be constexpr defined
#include "kbfd_reloc_ops_impl.h"

// define all supported target formats
#include "target/m68k_elf.h"

#include "kbfd_object_impl.h"
#include "kbfd_format_impl.h"
#include "kbfd_section_impl.h"
#include "kbfd_section_sym_impl.h"
#include "kbfd_format_elf_write.h"


namespace kbfd
{
}

#if 0 
#include "utility/print_type_name.h"
#include "kas/init_from_list.h"

namespace
{
    using namespace kbfd;


    struct xxx
    {
        xxx()
        {
            std::cout << "KBFD: print init" << std::endl;
#if 1
            using actions_names = init_from_list<const char *
                                               , meta::transform<
                                                            all_reloc_ops
                                                          , meta::quote<meta::front>
                                                            >
                                                >;
            auto actions = actions_names::value;
            print_type_name{"kbfd::actions"}.name<decltype(actions)>();

            auto p = actions_names::value;
            for (auto n = 0; n < actions_names::size; ++n, ++p)
            {
                std::cout << "action: " << n << ": " << *p << std::endl;
            }
#endif
#if 1
            using actions_ops_t = init_from_list<reloc_op_fns const *
                                               , all_reloc_ops
                                               , VT_CTOR
                                               >;
            auto actions_ops = actions_ops_t::value;

            print_type_name{"kbfd::actions_ops"}.name<decltype(actions_ops)>();

#if 1
            auto ops_p = actions_ops;
            for (auto n = 0; n < actions_ops_t::size; ++n, ++ops_p)
            {
                std::cout << "action op: " << n << ": " << (*ops_p)->name << std::endl;
            }
#endif
#endif
            std::cout << "END_ACTION\n" << std::endl;
        }
    } _x;
}
#endif
