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

// declare PROTO_LC features
// nominate features which describe supported addressing modes (by CPU class)
// and nominate features which appear in two unrelated architectures

// decalre co-processor support keys 
CPU_FEATURE (fpu)
CPU_FEATURE (mmu)

// declare fpu features

// declare mmu features

// clear for next run
#undef CPU_FEATURE
#undef FPU_FEATURE
#undef MMU_FEATURE
