#ifndef KAS_PARSER_DIAG_H
#define KAS_PARSER_DIAG_H

//
// Declare "error" list for assembler
//
// Each error instance has a level, description, and location.
// Create each instance in a std::vector & return an "index"
// to information for later reference.
//
// Convenience constructors: `error`, `warning` and `info`
// are provided.
//
// `errors` are returned as a non-zero index.

#include "parser/parser_types.h"
#include "parser/kas_position.h"
#include "kas_core/ref_loc_t.h"
#include "kas_core/kas_object.h"

#include <utility>
#include <type_traits>
#include <string>
#include <vector>

namespace kas::parser
{
// create a position tagged message w/o creating an actual diagnostic
struct tagged_msg
{
    tagged_msg() = default;
    tagged_msg(const char *msg, kas_position_tagged const& pos)
        : msg(msg), pos_p(&pos) {}

    operator bool() const { return msg; }

    const char *msg{};
    kas_position_tagged const *pos_p{};
};


// ******* assembler diagnostic constructor ************
enum class kas_diag_enum { FATAL, ERROR, WARNING, INFO };

template <typename REF>
struct kas_diag : core::kas_object<kas_diag<REF>, REF>
{
    using base_t = core::kas_object<kas_diag<REF>, REF>;
    using base_t::index;
    using base_t::for_each;
    using base_t::for_each_if;
    
    // enable DUMP
    using NAME = KAS_STRING("kas_diag");
    using base_t::dump;

    // doesn't participate in expression evaluation
    using not_expression_type = void;

    kas_diag(kas_diag_enum level
           , std::string message
           , parser::kas_loc loc = {})
        : level(level), message(std::move(message)), base_t(loc)
        {}

    kas_diag(kas_diag_enum level, tagged_msg const& msg)
        : kas_diag(level, msg.msg, *msg.pos_p) {}

    const char *level_msg() const
    {
        static constexpr const char* _levels[] = {"Fatal Error: ", "Error: ", "Warning: ", "Info: "};
        return _levels[static_cast<unsigned>(level)];
    }

    // allow tagging of "new" errors
    static auto& last()
    {
        return base_t::get(base_t::num_objects());
    }

    // diagnostics have fixed location
    REF ref(parser::kas_loc loc = {}) const
    {
        return get_ref(*this, this->loc());
    }

    template <typename OS> void print(OS& os) const;

    // named constructors
    template <typename...Args>
    static auto& fatal  (Args&&...args)
            { return base_t::add(kas_diag_enum::FATAL, std::forward<Args>(args)...); }
    template <typename...Args>
    static auto& error  (Args&&...args)
            { return base_t::add(kas_diag_enum::ERROR, std::forward<Args>(args)...); }
    template <typename...Args>
    static auto& warning (Args&&...args)
            { return base_t::add(kas_diag_enum::WARNING, std::forward<Args>(args)...); }
    template <typename...Args>
    static auto& info  (Args&&...args)
            { return base_t::add(kas_diag_enum::INFO, std::forward<Args>(args)...); }

    
//private:
    kas_diag_enum level {};
    std::string message;
    static inline core::kas_clear _c{base_t::obj_clear};
};

}

#endif
