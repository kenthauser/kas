#ifndef KAS_BSD_OPS_SECTION_H
#define KAS_BSD_OPS_SECTION_H


// section opcode
//
// called: opc_segment(container<expr_t>, section_name, subsection = 0)
//
// if `seg_name` argument provided, use as section name
//     otherwise, first argument is section name
// if `arg` is non-zero, use as subsection number
// if final argument is integral, use as subsection number.
// remaining argments are section flags

// required `kas_core` includes are in core header

#include "bsd_arg_defn.h"
#include "kas_core/opc_segment.h"

namespace kas::bsd
{

struct bsd_section_base : opc_segment
{
    // this is a helper-base class. restrict access
protected:

    using seg_index_t  = typename core::core_segment::index_t;
    using kas_loc      = ::kas::parser::kas_loc;

    // analyze args & return appropriate segment index
    // NB: emit error & return zero if args invalid
    auto get_segment(data_t& data, bsd_args&& args
                   , const char *name = {}
                   , short subsection = {}) -> seg_index_t;

    // implement "set_segment" via `opc_segment` 
    void set_segment(data_t& data, seg_index_t index)
    {
        previous = current;
        current  = index;
        opc_segment::proc_args(data, index);
    }
    
    // support the BSD section stack.
    static auto& stack()
    {
        static auto _stack = new std::list<seg_index_t>;
        return *_stack;
    }
    
    // get_segment support routines
    template <typename It>
    const char *proc_elf_args(It& it, It const& end);
    
    // ELF flags are passed as quoted string
    const char *proc_elf_flags(e_string_t const& flags);
    
    // for section flag calculation
    kbfd::kbfd_word sh_type     {};
    kbfd::kbfd_word sh_flags    {};
    kbfd::kbfd_word kas_entsize  {};
    kbfd::kbfd_word kas_linkage {};
    core::core_segment const *kas_group_p {};   // support ELF section groups

    // for "push" "pop" section support
    static inline seg_index_t current, previous;
};

struct bsd_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                 , short arg_c, const char * const *str_v, short const *num_v)
    {
        // if `name` and `subsection` fixed args present, use them 
        if (arg_c > 1)
            ++num_v;        // subsection is second numeric arg
        if (arg_c > 0)
            proc_args(data, std::move(args), *str_v, *num_v);
        else
            proc_args(data, std::move(args));
    }

    void proc_args(data_t& data, bsd_args&& args,
                    const char *name = {}, short subsection = {})
    {
        // if `name` and `subsection` specified, no args allowed
        // if `name` specified, max is 1 (subsection)
        // if `name` not specified, min is 1 (name)
        kas_error_t err;
        if (name)
            err = validate_min_max(args, 0, !!subsection);
        else
            err = validate_min_max(args, 1);
        if (err)
            return make_error(data, err);
        
        auto new_seg = get_segment(data, std::move(args), name, subsection);
        if (new_seg)
            set_segment(data, new_seg);
    }
};


struct bsd_push_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        // examine arguments to get "new" segment index
        auto new_seg = get_segment(data, std::move(args));
        
        // if proper, execute command
        if (new_seg)
        {
            stack().push_back(current);
            set_segment(data, new_seg);
        }
    }
};

struct bsd_pop_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        if (auto msg = validate_min_max(args, 0, 0))
            return make_error(data, msg);

        auto& s = stack();
        auto seg_num = s.back();
        s.pop_back();
        set_segment(data, seg_num);
    }
};

struct bsd_previous_section : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                    , short arg_c, const char * const *str_v, short const *num_v)
    {
        if (auto msg = validate_min_max(args, 0, 0))
            return make_error(data, msg);
        
        set_segment(data, previous);
    }
};


struct bsd_subsection : bsd_section_base
{
    void proc_args(data_t& data, bsd_args&& args
                 , short arg_c, const char * const *str_v, short const *num_v)
    {
        // single argument required
        if (auto msg = validate_min_max(args, 1, 1))
            return make_error(data, msg);
       
        // examine "subsection" value
        auto p = args[0].get_fixed_p();
        if (!p)
            return make_error(data, "fixed argument required", args.front());

        // get current "section" (from segment)
        auto& curr_seg = core::core_segment::get(current);
        auto& section  = curr_seg.section();

        // now change to new segment in same section
        auto& new_seg  = section.segment(*p);
        set_segment(data, new_seg.index());
    }
};


}


#endif
