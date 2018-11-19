#ifndef KAS_OPTIONS_PO_HELP_H
#define KAS_OPTIONS_PO_HELP_H


#include <map>
#include <regex>

#include "po_defn.h"

namespace kas::options::detail
{

struct po_help
{
    // XXX needs to support locale
    struct help_ci_compare
    {
        bool operator()(const char *lhs, const char *rhs) const
        {
            while (*lhs && *rhs)
            {
                auto d = std::toupper(*lhs++);
                auto s = std::toupper(*rhs++);
                if (d < s)
                    return true;
                if (d > s)
                    return false;
            }

            // less than if lhs is shorter 
            return *rhs;
        }
    };
    std::multimap<const char *, std::pair<std::string, std::string>, help_ci_compare> help_info;

    template <typename T>
    po_help(T const& groups)
    {
        // XXX support name & epilogue
        add_help_defns(groups.begin(), groups.end());
    }


    template <typename Iter>
    void add_help_defns(Iter it, Iter const& end)
    {
        if (it == end)
            return;

        // groups are in reverse order from definition. Recurse before define.
        auto& group = *it++;
        add_help_defns(std::move(it), end);

        for (auto& defn : group.defns) {
            help_info.emplace(defn.sort_opt()
                            , std::make_pair(defn.help_opt(), defn.help_msg())
                            );
        }
    }

    template <typename OS>
    void dump(OS& os) const
    {
        for (auto& info : help_info) {
            os << "sort: " << info.first;
            os << " opt: " << info.second.first;
            os << " msg: " << info.second.second;
            os << std::endl;
        }

        os << std::endl;
    }

    void print(std::ostream& os) const
    {
        for (auto& info : help_info)
            print_one(os, info.second.first, info.second.second);

        // print epilogue
    }

    // tune as appropriate
    static constexpr auto CONSOLE_WIDTH = 80;
    static constexpr auto INDENT_OPT    = 2;
    static constexpr auto INDENT_MSG    = 25;
    static constexpr auto GAP_OPT_MSG   = 2;

    static void print_one(std::ostream& os, std::string const& opt, std::string const& msg)
    {

        print_opt(os, opt);     // print option string & leave indent at INDENT_MSG

        // tokenize description into vector<> of paragraphs, splitting on newline
        std::regex nl{"\n"};
        std::vector<std::string> paragraphs
            { std::sregex_token_iterator{msg.begin(), msg.end(), nl, -1 }, {}};
          

        auto indent = INDENT_MSG;
        for (auto& par : paragraphs) {
            if (!indent)
                os << std::endl;
            print_paragraph(os, par, indent);
            indent = 0;
        }
    }

    static void print_paragraph(std::ostream& os, std::string const& par, int ident)
    {
        // XXX split into words & wrap as appropriate
        os << par << std::endl;
    }


    static void print_opt(std::ostream& os, std::string const& opt)
    {
        os << std::string(INDENT_OPT, ' ') << opt;

        // space to INDENT_MSG. Start new line if doesn't fit
        int n = INDENT_MSG - INDENT_OPT - opt.size();
        if (n < GAP_OPT_MSG)
            os << '\n' << std::string(INDENT_MSG, ' ');
        else 
            os << std::string(n, ' ');
    }

};
}
#endif

