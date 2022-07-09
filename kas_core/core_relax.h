#ifndef KAS_CORE_RELAX_H
#define KAS_CORE_RELAX_H


#include "core_fits.h"

namespace kas::core
{

template <typename C>
struct core_relax
{
    using fuzz_t = typename frag_offset_t::value_t;
    static constexpr fuzz_t initial_fuzz = std::numeric_limits<int16_t>::max();
    
    // tuning function
    fuzz_t new_fuzz(frag_offset_t const& segment_size, fuzz_t prior = initial_fuzz)
    {
        if (trace)
        {
            *trace << "new_fuzz: size/prior: ";
            *trace << segment_size << ", " << prior;
        }

        fuzz_t delta = segment_size.max - segment_size.min;

        if (delta == 0)
        {
            if (trace)
                *trace << " -> " << 0 << std::endl;
            return 0;
        }

        // tune here
        auto fuzz = std::min(delta, prior) - 1;
        if (trace)
            *trace << " -> " << fuzz << std::endl;

        return fuzz;
    }

    auto relax_fn(fuzz_t fuzz)
    {
        core_fits fits(c.get_insn_dot(), fuzz);
    
        return [fits](auto& insn, core_expr_dot const&)
            {
                if (!insn.is_relaxed())
                {
                    if (trace)
                    {
                        *trace << "relax: " << insn.raw() << std::endl;
                        *trace << "relax: " << insn.fmt()
                               << " from "  << insn.size();
                    }

                    insn.calc_size(fits);

                    if (trace)
                        *trace << " -> " << insn.size() << std::endl;
                }
            };
    }

    // execute "relax"    
    void operator()()
    {
        if (trace)
            *trace << "\nBegin relaxing... (fuzz = INITIAL)" << std::endl;


        // perform initial pass: result: symbol "sections" resolved
        c.proc_all_frags(relax_fn(-1));

        // now relax sections
        core_section::for_each([this](auto& section)
            {
                if (trace)
                    *trace << "Relax section: " << section << std::endl;
                for (auto& segment : section)
                    relax_segment(*segment.second);
            });

        if (trace)
            *trace << "Relax complete." << std::endl;
    }

    void relax_segment(core_segment &segment)
    {
        if (trace)
            *trace << "Relaxing segment: " << segment << std::endl;

        auto fuzz = new_fuzz(segment.size());
        while(!segment.size().is_relaxed())
        {

            if (trace)
                *trace << "relax_seg:fuzz = " << fuzz << std::endl;
            
            for (auto fp = segment.initial_relax(); fp; fp = fp->next_p())
                relax_frag(*fp, fuzz);
            
            // all good things must end...
            if (fuzz == 0)
                break;

            fuzz = new_fuzz(segment.size(), fuzz);
        }

        if (trace)
            *trace << "Relax: segment " << segment << " done." << std::endl;
    }

    void relax_frag(core_fragment& frag, fuzz_t fuzz)
    {
        if (trace)
        {
            *trace << "Relaxing frag: " << frag << " @ " << frag.base_addr();
            *trace << std::hex << " (0x" << frag.base_addr().min << ")" << std::dec;
        }
        
        if (frag.is_relaxed())
        {       
            if (trace)
                *trace << " [relaxed] = " << frag.size() << std::endl;
        } 
        else if (frag.size().is_relaxed()) 
        {
            frag.set_size();
            if (trace)
                *trace << " [constant] -> " << frag.size() << std::endl;
        }
        else 
        {
            c.proc_frag(frag, relax_fn(fuzz));
            if (trace)
                *trace << " -> " << frag.size() << std::endl;
        }
    }


    C& c;
    static inline std::ostream *trace{};
};

template <typename C>
void do_relax(C& c, std::ostream *trace = {})
{
    core_relax<C>::trace = trace;
    core_relax<C>{c}();
}

}


#endif
