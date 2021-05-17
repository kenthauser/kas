// NB: this file included several times, so no header guard


// Declare features which control assembly

// NOP features not requested
#ifndef CPU_FEATURE
#define CPU_FEATURE(name)
#endif

#ifndef FPU_FEATURE
#define FPU_FEATURE(name)
#endif

#ifndef MMU_FEATURE
#define MMU_FEATURE(name)
#endif

// declare m68k/coldfire features
// nominate features which describe supported addressing modes (by CPU class)
// and nominate features which appear in two unrelated architectures
// eg: cpu32 & coldfire support `table` instructions

CPU_FEATURE (limit_3w)      // coldfire insns limited to 48 bits
CPU_FEATURE (math)          // coldfire requires special test for math insns
CPU_FEATURE (branch_long)   // support for 32-bit branches
CPU_FEATURE (read_ccr)      // read ccr (all but original '000)
CPU_FEATURE (callm)         // enable for 68020 only
CPU_FEATURE (mult64)        // '020, cpu32, and coldfire
CPU_FEATURE (divide)        // all but some early coldfire isa_a & isa_b
CPU_FEATURE (index_full)    // '020 index modes
CPU_FEATURE (index_word)    // coldfire doesn't support word index reg
CPU_FEATURE (index_scale)   // '000 & '010 doesn't support scaled index
CPU_FEATURE (index_scale_8) // coldfire doesn't support *8 scale (w/o FPU)
CPU_FEATURE (movep)         // deleted from '060
CPU_FEATURE (table)         // cpu32 & coldfire
CPU_FEATURE (mac)           // coldfire mac
CPU_FEATURE (bmac)          // coldfire *original* mac
CPU_FEATURE (emac)          // coldfire enhanaced mac
CPU_FEATURE (emac_b)        // coldfire enahanced mac_v2


// decalre co-processor support keys 
CPU_FEATURE (fpu)
CPU_FEATURE (mmu)
CPU_FEATURE (div)
CPU_FEATURE (usp)

// declare fpu features

// All FPU instructions fall into one of four categories:
//FPU_FEATURE (fpu_insn)
FPU_FEATURE (fpu_intrz)
FPU_FEATURE (fpu_basic)
FPU_FEATURE (fpu_m68k)
FPU_FEATURE (fpu_trig)

// support feature with rounding
FPU_FEATURE (fpu_basic_rnd)
FPU_FEATURE (fpu_m68k_rnd)
FPU_FEATURE (fpu_trig_rnd)

// Some FPU instructions support rounding
FPU_FEATURE (fpu_rounding)

// different FPUs allow different addess modes:
FPU_FEATURE (fpu_p_addr)
FPU_FEATURE (fpu_x_addr)

// coldfire CPUs with FPUs allow index scale 8
FPU_FEATURE (f_index_scale_8)


// declare mmu features
MMU_FEATURE (mmu_insn)
MMU_FEATURE (mmu_bcc)
MMU_FEATURE (mmu_040)
MMU_FEATURE (mmu_060)
 
// clear for next run
#undef CPU_FEATURE
#undef FPU_FEATURE
#undef MMU_FEATURE
