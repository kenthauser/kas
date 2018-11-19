#ifndef KAS_KAS_CORE_HARDWARE_DEFNS_PO_H
#define KAS_KAS_CORE_HARDWARE_DEFNS_PO_H

#include "hardware_defns.h"

namespace kas::core::hardware
{

template <typename T>
struct cb_hw_isa
{
    // implicit deduction guide
    cb_hw_isa(T) {}

    static int name2cpu(const char *name);

    void operator()(void *cb_arg, const char *cpu)
    {
        T& defs = *static_cast<T*>(cb_arg);

        auto cpu_id = name2cpu(cpu);
        defs.set(cpu_id + 1);
    }
};

// name -> cpu_index (for command line options)
// XXX case insensative compare
template <typename T>
int cb_hw_isa<T>::name2cpu(const char *name)
{
    int n{};
    auto p = T::is_names;

    for (; p < &p[T::is_list::size()]; ++n, ++p)
        if (!std::strcmp(*p, name))
            return n;
    
    return -1;
}


template <typename T>
struct cb_hw_has
{
    // implicit deduction guide
    constexpr cb_hw_has(T, typename T::hw_tst tst) : tst(tst) {}

    void operator()(void *cb_arg, const char *value)
    {
        T& defs = *static_cast<T*>(cb_arg);

        auto yn = std::atoi(value);
        defs.set(tst, yn);
    }

    typename T::hw_tst tst;
};

}

#endif
