#ifndef KAS_CORE_EMIT_STREAM_IMPL_H
#define KAS_CORE_EMIT_STREAM_IMPL_H

#include "emit_stream.h"
#include "core_emit.h"

#if 1
namespace kas::core
{

// ctor
emit_stream_base::emit_stream_base(kbfd::kbfd_object *kbfd_p)
    : kbfd_p(kbfd_p)
    , base_p(new core_emit(*this, kbfd_p))
    {
        std::cout << "emit_stream_base ctor: kbfd_p = " << kbfd_p << std::endl;
    }

// dtor: tell `kbfd` if provided XXX
emit_stream_base::~emit_stream_base()
{
    std::cout << "emit_stream_base: dtor" << std::endl;
}

// default implementation of `emit`
void emit_stream_base::emit(core::core_insn& insn, core::core_expr_dot const *dot_p)
{
    insn.emit(*base_p, dot_p);
}

// emit memory buffer (with byte swapping)
void emit_stream_base::put_data(e_chan_num num
                   , void const *p
                   , uint8_t     width 
                   , unsigned    count)
{
    auto get_as_width = [width](auto p) -> uint64_t
        {
            // NB: if values properly aligned when put in "buffer"
            // they will be properly aligned when extracted
            switch (width)
            {
                case 8:
                    return *static_cast<uint64_t const *>(p);
                case 4:
                    return *static_cast<uint32_t const *>(p);
                case 2:
                    return *static_cast<uint16_t const *>(p);
                case 1:
                    return *static_cast<uint8_t  const *>(p);
                default:
                    throw std::logic_error("emit_stream_base::put_data: invalid width");
            }
        };
    
    while (count--)
    {
        put_uint(num, width, get_as_width(p));
        p = static_cast<const char *>(p) + width;
    }
}

// trampoline function
void emit_stream_base::set_segment(core_segment const& segment)
{
    base().set_segment(segment);
}

//
// Methods for stream version
//

// principle ctor using ostream
emit_stream::emit_stream(kbfd::kbfd_object& kbfd_obj, std::ostream& out)
    : out(out), emit_stream_base(kbfd_obj) {}
#if 0
// ctor using path: allocate file (ie ofstream object)
emit_stream::emit_stream(kbfd::kbfd_object& kbfd_obj, const char *path)
    : file_p(new std::ofstream(
                    path
                  , std::ios_base::binary | std::ios_base::trunc 
                  ))
    , emit_stream(kbfd_obj, *file_p)
{
    std::cout << "emit_stream_base: allocating \"" << path << "\"" << std::endl;
}

// ctor: allow string for path
emit_stream::emit_stream(kbfd::kbfd_object& kbfd_obj, std::string const& path)
    : emit_stream(kbfd_obj, path.c_str()) {}
#endif

// dtor: close file if allocated
emit_stream::~emit_stream()
{
    delete file_p;
}

    
}
#endif
#endif
