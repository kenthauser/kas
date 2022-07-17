#ifndef KBFD_KBFD_RELOC_OPS_IMPL_H
#define KBFD_KBFD_RELOC_OPS_IMPL_H

#include "kbfd_reloc_ops.h"
#include "kbfd_reloc.h"
#include "kas/defn_utils.h"
#include "kas/init_from_list.h"

#include <map>
#include <iostream>
#include <cstring>

namespace kbfd
{

// 
// `reloc_op_fns` default implementations
//

auto reloc_op_fns::extract(value_t data) const -> value_t
{
    return data;
}

const char *reloc_op_fns::insert(
          value_t& data
        , value_t  value) const
{
    data = value;
    return {};
}

// get relocation "value" given extracted data
// not used by assembler
const char *reloc_op_fns::eval(
          flags_t flags
        , value_t& value) const
{
    static_assert(std::is_same_v<flags_t, typename kbfd_reloc::flags_t>,
        "adjust reloc_op_fns::flags_t to match kbfd_reloc::flags_t");
    return {};
}

// update `accum` according to `value` and `flags`
const char *reloc_op_fns::update(
          flags_t flags
        , value_t& accum
        , value_t const& value) const
{
    return {};
}

//
// `reloc host_fns` default implementations
//



//
// `reloc_action` implementation
//

using all_reloc_ops = kas::all_defns<reloc_ops_v, reloc_ops_defn_tags>;
using action_names  = meta::transform<all_reloc_ops, meta::quote<meta::front>>;
using action_ops    = meta::transform<all_reloc_ops, meta::quote<meta::back>>;

// initialize static values from definition lists
decltype(reloc_action::MAX_ACTIONS) reloc_action::MAX_ACTIONS 
    = all_reloc_ops::size();
decltype(reloc_action::names)       reloc_action::names
    = kas::init_from_list<const char *, action_names>::value;
decltype(reloc_action::ops)         reloc_action::ops
    = kas::init_from_list<const reloc_op_fns *, action_ops, kas::VT_CTOR>::value; 

// lookup action by type (constexpr)
template <typename T>
constexpr auto reloc_action::as_action(T) -> index_t
{
    return meta::find_index<action_names, T>::value;
}

// lookup action by name (runtime)
auto reloc_action::lookup(const char *name) -> index_t
{
    auto gen_map = []
        {
            struct cmp_str
            {
                bool operator()(const char *a, const char *b) const
                {
                    return std::strcmp(a, b) < 0;
                }
            };
            
            std::map<const char *, index_t, cmp_str> action_map;
            auto p = names;
            for (index_t i = 0; i < MAX_ACTIONS; ++i, ++p)
            {
                std::cout << "kbfd::reloc_action::lookup: installing ";
                std::cout << *p << " = " << +i << std::endl;
                action_map[*p] = i;
            }
            return action_map;
        };

    static auto map = gen_map();
    std::cout << "kbfd::reloc_action: resolving " << name;
    std::cout << " as " << +map[name] << std::endl;
    return map[name];
}

}
#endif
