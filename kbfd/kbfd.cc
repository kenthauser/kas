#include "kbfd.h"

// include target declaration files
#include "target/m68k.h"
#include "target/z80.h"
#include "target/arm.h"

// include target support files
#include "target/arm_reloc_impl.h"

// include implementation files
#include "kbfd_reloc_ops_impl.h"
#include "kbfd_object_impl.h"
#include "kbfd_target_format_impl.h"
#include "kbfd_section_impl.h"
#include "kbfd_section_sym_impl.h"

// include "format" _impl files
#include "kbfd_format_elf_write.h"

// include all supported target formats
#include "target/arm_elf.h"
#include "target/m68k_elf.h"
#include "target/z80_aout.h"


// include utility files
#include "kas/init_from_list.h"
#include "utility/print_type_name.h"

namespace kbfd
{

using namespace meta;

// use `init_from_list` to create array of `kbfd_target_format` pointers.
// create pointer to derived type (or nullptr) for each format in `format_tags`
struct gen_target_formats
{
    // type for `init_for_list` instantiation of target_format array
    using RT = kbfd_target_format const * const * const;

    // generate `kbfd_target_format *` array for `target`
    template <typename TARGET_FORMAT_LIST>
    using gen_init_list = kas::init_from_list<kbfd_target_format const *
                                            , TARGET_FORMAT_LIST
                                            , kas::VT_CTOR
                                            >;

    // extract `format_list` for specified target
    // pass resulting list to `gen_init_list`
    template <typename TARGET>
    struct do_gen_list
    {
        using TGT_FORMATS = _t<transform<format_tags
                                       , _t<kbfd_targets_v<TARGET>>
                                       >>;
        using type = gen_init_list<TGT_FORMATS>;
    };

    // actual `init_from_list` CTOR for single target
    struct gen_target_formats_ctor
    {
        template <typename TARGET>
        using invoke = _t<invoke<quote<do_gen_list>, TARGET>>;
    };
    
    // consume `init_from_list` default CTOR_ARG of `void`
    template <typename>
    using invoke = gen_target_formats_ctor;
};

kbfd_target_format const *get_obj_format(const char *target, const char *format)
{
    // compare ignoring case; `it` is upper, `v` is mixed case
    // return `0` for not found
    auto find_idx = [](auto it, auto cnt, const char *v) -> unsigned
    {
        auto strcasecmp = [](auto upper, auto mixed) -> unsigned
            {
                for (; *upper; ++upper, ++mixed)
                    if (*upper != std::toupper(*mixed))
                        return 1;
                return !!*mixed;
            };

        for (unsigned idx = 0; idx < cnt; ++it, ++idx)
        {
            for (; *it; ++it)
                if (!strcasecmp(*it, v))
                    return idx + 1;
                else
                    break;
        }
        return 0;
    };

    std::cout << "get_obj_format: target = " << target << ", format = ";
    std::cout << (format ? format : "default") << std::endl;

    // XXX need to deal with aliases & defaults...
    constexpr auto targets = kas::init_from_list<const char *, target_tags>();
    constexpr auto formats = kas::init_from_list<const char *, format_tags>();

    // instantiate all defined targets & store in array of arrays
    constexpr auto tgts = kas::init_from_list<
                                typename gen_target_formats::RT
                              , target_tags
                              , gen_target_formats
                              >::value;


    auto tgt_idx = find_idx(targets.value, targets.size, target);
    unsigned fmt_idx{};
    if (format)
        fmt_idx = find_idx(formats.value, formats.size, format);

    // if no format selected, select first defined
    if (fmt_idx == 0)
    {
        for (auto p = tgts[tgt_idx-1]; fmt_idx < formats.size; ++fmt_idx)
            if (*p++)       // break if target format is defined
                break;
    
        // no format defined...
        if (fmt_idx == formats.size)
            fmt_idx = 0;
        else
            ++fmt_idx;      // make `1` based
    }

    std::cout << "get_obj_format: tgt_idx = " << tgt_idx;
    std::cout << ", fmt_idx = " << fmt_idx << std::endl;

    std::cout << "gen_obj_format: target = " << targets.value[tgt_idx-1];
    std::cout << ", format = " << formats.value[fmt_idx-1] << std::endl;

    // extract selected target format;
    return tgts[tgt_idx-1][fmt_idx-1];
}

}

