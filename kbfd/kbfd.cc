#include "kbfd.h"

#include "kbfd_reloc_ops.h"
#include "kbfd_object_impl.h"

namespace kbfd
{
}


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

