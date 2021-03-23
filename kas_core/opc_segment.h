#ifndef KAS_CORE_OPC_SEGMENT_H
#define KAS_CORE_OPC_SEGMENT_H

// opc_segment: change `segment`

// implementation: store segment index in fixed area

#include "opcode.h"
#include "core_segment.h"

namespace kas::core::opc
{


struct opc_segment : opcode
{
    OPC_INDEX();
    const char *name() const override { return "SEG"; }

    opc_segment() = default;

    void operator()(data_t& data, core_segment::index_t index) const
    {
        data.fixed = index;
    }
    
    void operator()(data_t& data, core_segment const& seg) const
    {
        (*this)(data, seg.index());
    }

    void proc_args(data_t& data, core_segment::index_t index) const
    {
        data.fixed = index;
    }

    void fmt(data_t const& data, std::ostream& os) const override
    {
        auto index = data.fixed.fixed;
        os << index << ' ';
        os << core_segment::get(index);
    }

    void emit(data_t const& data
            , emit_base& base
            , core_expr_dot const *dot_p) const override
    {
        auto& seg = core_segment::get(data.fixed.fixed); 
        base.set_segment(seg);
    }
};

}
#endif
