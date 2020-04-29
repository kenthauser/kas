#ifndef KAS_BSD_OPS_DWARF_H
#define KAS_BSD_OPS_DWARF_H

#include "kas_core/opc_symbol.h"
#include "kas_core/opc_dw_line.h"
#include "bsd_stmt.h"

#include "parser/parser_obj.h"

#include <ostream>

namespace kas::bsd
{
struct bsd_file : core::opc::opc_dw_file
{
    template <typename...Ts>
    void proc_args(data_t& data, bsd_args&& args, Ts&&...)
    {
        if (auto err = validate_min_max(args, 1, 2))
            return make_error(data, err);
        auto iter = args.begin();
        int  index{};

        if (args.size() != 1) {
            if (auto p = iter++->get_fixed_p())
                index = *p;
            else
                return make_error(data, "fixed index required", args.front());
        }

        auto name_p = iter->template get_p<e_string_t>();
        if (!name_p)
            return make_error(data, "file name required", *iter);

        opc_dw_file::proc_args(data, index, (*name_p)().c_str(), args.back());
    }
};

struct bsd_elf_ident : core::opcode
{
    OPC_INDEX();

    const char *name() const override { return "IDENT"; }

    template <typename...Ts>
    void proc_args(data_t& data, bsd_args&& args, Ts&&...)
    {
        if (auto err = opcode::validate_min_max(args, 1, 1))
            return make_error(data, err);
        auto iter = args.begin();
        auto p = iter->template get_p<e_string_t>();
        if (!p)
            return make_error(data, "string required", *iter);

        std::ostringstream s;

        s << ".section \".comment\"\n";
        s << ".ascii \" comment \"" << std::endl;
        s << ".previous\n";

        auto macro = new std::string(s.str());

        ::kas::parser::parser_src::push(macro->begin(), macro->end(), "ident");
        auto& di = data.di();
        *di++ = std::move(iter->expr());
    }
};

// the ".loc" opcode is fundamental for dwarf line debugging
//
// arg format: file line [column] [basic_block] [prologue_end] [epilogue_begin]
//             ["is_stmt" value] ["isa" value] ["discriminator" value]...
//
// in other words: two required arguments (file/line)
//                 four optional positional arguments
//                 unlimited keyword arguments w/o order limitations
//
// NB: optional positional keywords are ignored if supplied and value is zero.
//
// keyword valided against "official" names in "dwarf::dwarf_defns.inc"
// NB: processing is liberal: no order is enforced. positional can be passed as keyword.

struct bsd_loc : opc_dw_line 
{
    using base_t = opc_dw_line;
        
    // name positional arguments
    // NB: names must match values in "dwarf::dwarf_defns.inc"
    static constexpr const char * positional_args[] =
        {
              "file"
            , "line"
            , "column"
            , "basic_block"
            , "prologue_end"
            , "epilogue_begin"
        };

    template <typename...Ts>
    void proc_args(data_t& data, bsd_args&& args, Ts&&...)
    {
        // first two positional args are required.
        // pick absurdly large number for max, so as to limit fixed buffer size
        static constexpr auto MIN_LOC_ARGS = 2; 
        static constexpr auto MAX_LOC_ARGS = 2 * dwarf::NUM_DWARF_LINE_STATES;

        if (auto err = opcode::validate_min_max(args, MIN_LOC_ARGS, MAX_LOC_ARGS))
            return make_error(data, err);
        // create array of "key/value" pairs for output
        dl_data::dl_pair values[MAX_LOC_ARGS];

        auto out    = std::begin(values);
        auto in     = args.begin();
        auto end    = args.end();
        
        int  reqd_cnt = MIN_LOC_ARGS;
        auto name_p   = positional_args;

        for (; in != end; ++in)
        {
            auto& arg = *in;
                
            // test for "symbol" first so as not to turn token into symbol
            auto key_p = arg.get_p<core::symbol_ref>();
                
            // looking for positional args?
            if (name_p && !key_p) {
                // if argument has numeric value, process as positional
                if (auto p = arg.get_fixed_p())
                {
                    // if required value (file/line), always emit
                    if (reqd_cnt)
                    {
                        --reqd_cnt;
                        *out++ = {0, *p};
                    } else {
                        // optional arg: ignore if zero
                        if (*p)
                            *out++ = { dl_data::lookup(*name_p), *p };
                    }

                    // check for end of positional args
                    if (++name_p == std::end(positional_args))
                        name_p = {};
                    continue;
                }

                // FALLSTHRU & treat as keyword argument
            }
    
            // test if required positionals found
            if (reqd_cnt)
                return make_error(data, "fixed value required", *in);
            else
                name_p = {};

            // lookup key
            if (!key_p)
                return make_error(data, "expected a dwarf line keyword", *in);

            auto key = dl_data::lookup(key_p->get().name().c_str());

            if (!key)
                return make_error(data, "unknown dwarf line keyword", *in);

            if (in == std::prev(end))
                return make_error(data, "keyword requires value", *in);

            auto& value = *++in;
            if (auto p = value.get_fixed_p()) 
                *out++ = { key, *p };
            else
                return make_error(data, "fixed value required", *in);
        }

        // calculate values generated
        auto cnt = out - values;

        // pass first two values (file, line) as fixed arguments
        base_t::proc_args(data, values[0].second, values[1].second, &values[2], cnt-2);
    }
};

}

#endif
