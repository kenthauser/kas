#ifndef KAS_BSD_SECTION_OPS_DEF_H
#define KAS_BSD_SECTION_OPS_DEF_H

#include "bsd_stmt.h"
#include "bsd_elf_defns.h"
// #include "kas_core/core_data_insn.h"
#include "kas_core/opcode.h"
#include "kas_core/opc_misc.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/core_section.h"
#include "kas_core/core_symbol.h"
//#include "parser/ast_parser.h"      // need print functions

#include "elf/elf_external.h"

#include "kas/defn_utils.h"
#include "utility/string_mpl.h"

#include <ostream>

namespace kas::bsd
{
// section opcode
//
// called: opc_section(container<expr_t>, section_name, subsection = 0)
//
// if `seg_name` argument provided, use as section name
//     otherwise, first argument is section name
// if `arg` is non-zero, use as subsection number
// if final argument is integral, use as subsection number.
// remaining argments are section flags

struct bsd_section_base :  opc_section
{
    using seg_index_t = typename core::core_section::index_t;
    using kas_loc     = ::kas::parser::kas_loc;

    // support the BSD section stack.
    static auto& stack()
    {
        static auto _stack = new std::list<seg_index_t>;
        return *_stack;
    }

    // section & push_section allow section to be specified by name
    // ELF flags, etc, can be complicated.
    struct segment_result
    {
        segment_result(const char *msg, kas_loc const& loc)
                : diag{ kas_diag_t::error(msg).ref(loc) } {}
        segment_result(seg_index_t idx)
                : idx{ idx } {}

        kas_error_t diag {};
        seg_index_t idx  {};
    };

    segment_result
    get_segment(bsd_args&& args, const char *seg_name = {}, short subsection = {})
    {
        kas_token loc = args.front();
        
        // at a minimum, need a name...
        if (validate_min_max(args, !seg_name))
            return { "No section name", loc };

        auto it  = args.begin();
        auto end = args.end();

        // If `seg_name` look for trailing subsection arg,
        // otherwise process as `ELF` name, flags, etc...
        if (seg_name) {
            sh_name = seg_name;
            // check for subsegment (trailing fixed arg)
            if (it != end) {
                if (auto p = args.back().get_fixed_p()) {
                    // if fixed value specified -- error
                    if (subsection)
                        return {"Invalid subsection", *it};
                    subsection = *p;
                    it++;
                    }
                }

        // else, process as ELF segment pseudo-op
        } else if (auto err_msg = proc_elf(it, end)) {
            return {err_msg, *it};
        }

        // were all arguments consumed?
        if (it != end)
            return {"Invalid arguments", *it};

        // test if `reserved` section name
        // NB: test only if `sh_type` or `sh_flags` specified.
        // NB: `sh_type` & `sh_flags` overriden in ctor if reserved
        if (sh_type || sh_flags)
            if (auto p = core::core_section::is_reserved(sh_name)) {
                if (sh_type && sh_type != p->sh_type)
                    return {"Invalid section type", loc};
                if (sh_flags && sh_flags != p->sh_flags)
                    return {"Invalid section flags", loc};
            }

        // add subsection to name
        if (subsection)
            sh_name += std::to_string(subsection);

        auto& seg = core::core_section::get(sh_name, sh_type, sh_flags, sh_entsize
                                    , kas_group, kas_linkage
                                    );

        return { seg[subsection].index() };

    }
    
    template <typename It>
    const char *proc_elf(It& it, It const& end)
    {
        // get segment name from first argument
        // NB: first arg may have been parsed as "symbol" or "string"
        expr_t&& e = *it++;
        if (auto p = e.template get_p<core::symbol_ref>()) {
            auto& sym = p->get();
            sh_name = sym.name();
        } else if (auto p = e.template get_p<expression::e_string_t>()) {
            sh_name = p->get().c_str();
        } else {
            return "Invalid section name";
        }

        // if only name, done
        if (it == end) return nullptr;

        // get "flags (string)"
        auto flags_p = it++->template get_p<expression::e_string_t>();
        if (!flags_p)
            return "Invalid Section Flags";

        if (auto err = proc_elf_flags(flags_p))
            return err;

        // next arg is "type", if present
        // XXX eg @progbits
        if (it != end) {
            // increment it later
            auto value = -1;
            if (auto p = it->template get_p<token_at_ident>())
                value = parser::get_section_type(*p, true);
 
            if (value == -1)
                return "Invalid section type";

            sh_type = value;
            ++it;
        }

        // get flag-specific args
        if (flag_entsize) {
            if (it == end)
                return "Missing section entsize";
            if (auto p = it++->get_fixed_p())
                sh_entsize = *p;
            else
                return "Invalid section entsize";
        }

        // XXX unsure what "GroupName" is/does
        if (flag_group) {
            if (it == end)
                return "Missing section group";
            if (auto p = it++->template get_p<core::symbol_ref>())
                kas_group = p->get().name();
            else
                return "Invalid section group";
        }

        // XXX unsure what "linkage" is
        if (it != end) {
            if (auto p = it++->get_fixed_p())
                kas_linkage = *p;
            else
                return "Invalid section linkage";
        }

        if (it != end)
            return "Invalid argument to section";

        return nullptr;
    }

    template <typename T>
    const char *proc_elf_flags(T const& flags_p)
    {
        for (auto p = flags_p->get().c_str(); *p; ++p) {
            elf::Elf32_Word bsd_flag{};
            switch (*p) {
                case 'a':
                    bsd_flag = SHF_ALLOC;
                    break;
                case 'd':
                    bsd_flag = SHF_GNU_MBIND;
                    break;
                case 'e':
                    bsd_flag = SHF_EXCLUDE;
                    break;
                case 'w':
                    bsd_flag = SHF_WRITE;
                    break;
                case 'x':
                    bsd_flag = SHF_EXECINSTR;
                    break;
                case 'M':
                    bsd_flag = SHF_MERGE;
                    flag_entsize = true;
                    break;
                case 'S':
                    bsd_flag = SHF_STRINGS;
                    break;
                case 'G':
                    bsd_flag = SHF_GROUP;
                    flag_group = true;
                    break;
                case 'T':
                    bsd_flag = SHF_TLS;
                    break;
                case '?':
                    // XXX ???
                    break;
                case '"':
                    // XXX fix e_string...
                    continue;
                default:
                    return "Invalid section flag";
            };
            sh_flags |= bsd_flag;
        }
        return nullptr;
    }

    std::string     sh_name;
    elf::Elf32_Word sh_type     {};
    elf::Elf32_Word sh_flags    {};
    elf::Elf32_Word sh_entsize  {};
    std::string     kas_group   {};
    elf::Elf32_Word kas_linkage {};

    bool flag_group   {};
    bool flag_entsize {};
};

struct bsd_section : bsd_section_base
{

    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(data, std::move(args), str_v[0], num_v[1]);
    }

    void proc_args(data_t& data, bsd_args&& args,
                    const char *seg_name = {}, short subsection = {});
};

void bsd_section::proc_args(data_t& data, bsd_args&& args
                                , const char *seg_name, short subsection)
{
    auto [ err, seg_num ] = get_segment(std::move(args), seg_name, subsection);
    if (err)
        return make_error(data, err);

    opc_section::proc_args(data, seg_num);
}

struct bsd_push_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        auto [ err, seg_num ] = get_segment(std::move(args), str_v[0], num_v[1]);
        if (err)
            return make_error(data, err);

        stack().push_back(opc_section::current);
        opc_section::proc_args(data, seg_num);
    }
};

struct bsd_pop_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(data, std::move(args), str_v[0], num_v[1]);
    }

    void proc_args(data_t& data, bsd_args&& args,
                    const char *seg_name = {}, short subsection = {})
    {
        if (auto msg = validate_min_max(args, 0, 0))
            return make_error(data, msg);

        auto& s = stack();
        auto seg_num = s.back();
        s.pop_back();
        opc_section::proc_args(data, seg_num);
    }
};

struct bsd_previous_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        proc_args(data, std::move(args), str_v[0], num_v[1]);
    }

    void proc_args(data_t& data, bsd_args&& args,
                    const char *seg_name = {}, short subsection = {})
    {
        if (auto msg = validate_min_max(args, 0, 0))
            return make_error(data, msg);

        opc_section::proc_args(data, opc_section::previous);
    }
};


}


#endif
