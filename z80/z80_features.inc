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

// declare z80-features
// nominate features which describe supported addressing modes (by CPU class)
// and nominate features which appear in two unrelated architectures
// clear for next run
#undef CPU_FEATURE
#undef FPU_FEATURE
#undef MMU_FEATURE
