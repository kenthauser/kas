#ifndef KAS_BSD_OPS_SECTION_IMPL_H
#define KAS_BSD_OPS_SECTION_IMPL_H


// section opcode
//
// called: opc_segment(container<expr_t>, section_name, subsection = 0)
//
// if `seg_name` argument provided, use as section name
//     otherwise, first argument is section name
// if `arg` is non-zero, use as subsection number
// if final argument is integral, use as subsection number.
// remaining argments are section flags

#include "bsd_ops_section.h"
#include "bsd_elf_defns.h"

#include "kas_core/core_section.h"


namespace kas::bsd
{

// translate arguments into section & section definitions
// use section to retrive segment index
// return index of zero if error emitted

// The format of the commands (and thus args) is as follows:
//
// .section name [, subsection] [, "flags", [, @type [, flag_specific_args]]]
//
// if `seg_name` is specified, parsing begins with "flags"

auto bsd_section_base::get_segment(data_t& data
                                 , bsd_args&& args
                                 , const char *seg_name
                                 , short subsection) -> seg_index_t
{
    auto it  = args.begin();
    auto end = args.end();
   
    // utility to normalize error reporting
    auto make_error = [&](auto msg, auto& it) -> seg_index_t
        {
            if (it == end)
                this->make_error(data, msg, args.back());
            else
                this->make_error(data, msg, *it);
            return {};
        };

    std::cout << "get_segment: ";
    if (seg_name) std::cout << "name = \"" << seg_name << "\", ";
    if (subsection) std::cout << "subsection = " << subsection << ", ";
    std::cout << "arg count = " << args.size() << std::endl;

    //
    // name is required. retrieve from `args` if not supplied
    //
    std::string _name;      // std::string to retrieve token value

    if (!seg_name)
    {
        if (it == end)
            return make_error("Section name required", it);
        
        _name = *it++;   // consume name
        seg_name = _name.c_str();
       
        // if numeric arg follows, it's subsection
        if (it != end)
            if (auto p = it->get_fixed_p())
            {
                subsection = *p;
                ++it;       // consume subsection
            }
    }

    // lookup name/subsection to get standard type & flags
    // default `sh_type` to SHT_PROGBITS
    auto [defn_p, sub] = core::core_section::lookup(seg_name, subsection);
    std::cout << "get_segment: name = " << seg_name;
    std::cout << ", subsection = " << sub;
    
    if (defn_p)
        std::cout << ", defn_p = " << defn_p->name;
    std::cout << std::endl;

    if (defn_p)
    {
        // normalize name & type. allow non-standard "flags" if defined
        seg_name = defn_p->name;
        sh_type  = defn_p->sh_type;
    }
    else 
        sh_type = SHT_PROGBITS;     // default type (ie not SHT_NULL)

    //
    // now process rest of arguments
    //

    // process args according to ELF definitions
    validate_min_max(args);         // delete empty arglist

    // XXX support formats other than ELF
    if (auto err = proc_elf_args(it, end))
        return make_error(err, it);

    // default the flags according to well-known-section, if not defined
    if (defn_p && !sh_flags)
        sh_flags = defn_p->sh_flags;

    // warn if flags don't match defaults
    if (defn_p)
    {
        if (sh_type != defn_p->sh_type)
            std::cout << "get_segment: sh_type doesn't match default" << std::endl;
        if (sh_flags!= defn_p->sh_flags)
            std::cout << "get_segment: sh_flags doesn't match default" << std::endl;
    }
    
    // retrieve section (creating if neccesary)
    auto& section = core::core_section::get(seg_name, sh_type, sh_flags);

    // update values not passed in ctor
    if (kas_entsize) section.set_entsize(kas_entsize);
    if (kas_linkage) section.set_linkage(kas_linkage);
    if (kas_group_p) section.set_group(*kas_group_p);

    // retrieve segment (creating if neccesary)
    return section.segment(sub).index();
}

template <typename It>
const char *bsd_section_base::proc_elf_args(It& it, It const& end)
{
    // first arg must be a "string" of flags
    {
        if (it == end) return {};
        auto& tok = *it++;
        if (auto p = tok.is_token_type(expression::tok_string()))
        {
            // evaluate parsed value as expected type
            auto ks_p = p->bind(tok)();

            // process flag string
            if (auto msg = proc_elf_flags(*ks_p))
                return msg;
        }
        else
        {
            std::cout << "tok = " << tok << std::endl;
            return "Expected section flag string";
        }
    }
    
    // next arg is "type" (if present) (eg @progbits)
    {
        if (it == end) return {};
        auto& tok = *it++;
        if (tok.is_token_type(tok_bsd_at_ident()))
        {
            auto value = parser::get_section_type(tok, true);

            if (value == -1)
                return "Invalid section type";
            else
                sh_type = value;
        }
        else if (tok.is_token_type(tok_bsd_at_num())) 
            sh_type = *tok.get_fixed_p();

        else
            return "Invalid section type";
    }

    // if flag requires additional argument, extract it
    // NB: order matters:
    // 1. 'M': merge flag : numeric
    // 2. 'o': linked-to section : symbol
    // 3. 'G': group : GroupName (section)
    // 4. optional "linkage"
    // 5. optional string "unique" followed by additional numeric arg

    // get flag-specific args
    if (sh_flags & SHF_MERGE && !kas_entsize)
    {
        if (it == end)
            return "Missing section entsize";
        auto p = it++->get_fixed_p();
        if (p && *p)
            kas_entsize = *p;
        else
            return "Invalid section entsize";
    }

#if 0
    // XXX unsure what "GroupName" is/does
    if (flag_group) {
        if (it == end)
            return "Missing section group";
        if (auto p = it++->template get_p<core::symbol_ref>())
            kas_group = p->get().name();
        else
            return "Invalid section group";
    }

    // XXX unsure what "linkage" is/does
    if (it != end) {
        if (auto p = it++->get_fixed_p())
            kas_linkage = *p;
        else
            return "Invalid section linkage";
    }
#endif
    // done. test for too many args.
    if (it != end)
        return "Invalid arguments to section";
    return {};
}

// interpret elf flags
const char *bsd_section_base::proc_elf_flags(e_string_t const& flags)
{
    // passed string object. examine & set `sh_flags`
    for (auto& c : flags())
    {
        kbfd::kbfd_word bsd_flag;
        switch (c)
        {
            //
            // Simple flag commands
            //
            case 'a': bsd_flag = SHF_ALLOC;     break;
            case 'd': bsd_flag = SHF_GNU_MBIND; break;
            case 'w': bsd_flag = SHF_WRITE;     break;
            case 'x': bsd_flag = SHF_EXECINSTR; break;
            case 'S': bsd_flag = SHF_STRINGS;   break;
            case 'T': bsd_flag = SHF_TLS;       break;
            
            // XXX SHF_GNU_RETAIN not defined...(just absorb?)
            // case 'R': bsd_flag = SHF_GNU_RETAIN;break;
           
            // member of previously-current section's group
            case '?': 
            {
                auto& cur_seg = core::core_segment::get(current);
                kas_group_p   = cur_seg.section().group_p();
                if (!kas_group_p)
                {
                    // NB: memory leak for each section definition error
                    auto err_p = new std::string{"invalid section flag: ?"};
                    *err_p += ": previous section: \"";
                    *err_p += cur_seg.section().name();
                    *err_p += "\" not in a group";
                    return err_p->c_str();
                }
                bsd_flag      = SHF_GROUP;
                break;
            }
            
            //
            // argument retrieved by calling routine
            //
            case 'M': bsd_flag = SHF_MERGE;     break;
            case 'G': bsd_flag = SHF_GROUP;     break;
            case 'e': bsd_flag = SHF_EXCLUDE;   break;
            case 'o': bsd_flag = SHF_INFO_LINK; break;
                
#if 0
            //
            // numeric... (hex?), value or bit #?
            //
#endif
            default:
            {
                // NB: memory leak for each section definition error
                // so keep section definition errors to a minimum...
                auto err_p = new std::string{"Invalid section flag: "};
                *err_p += c;
                *err_p += ':';
                return err_p->c_str();
            }
        };
        sh_flags |= bsd_flag;
    }
    return {};
}

}
#endif
