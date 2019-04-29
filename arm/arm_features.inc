// NB: this file included several times, so no header guard


// Declare features which control assembly

// NOP features not requested
#ifndef CPU_FEATURE
#define CPU_FEATURE(name)
#endif


// declare arm features
// nominate features which describe supported addressing modes (by CPU class)
// and nominate features which appear in two unrelated architectures
CPU_FEATURE(arm_am)

#undef CPU_FEATURE
