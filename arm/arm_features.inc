// NB: this file included several times, so no header guard


// Declare features which control assembly

// NOP features not requested
#ifndef CPU_FEATURE
#define CPU_FEATURE(name)
#endif

#ifndef THB_FEATURE
#define THB_FEATURE(name)
#endif

#ifndef FPU_FEATURE
#define FPU_FEATURE(name)
#endif

#ifndef MMU_FEATURE
#define MMU_FEATURE(name)
#endif

// declare arm features
// nominate features which describe supported addressing modes (by CPU class)
// and nominate features which appear in two unrelated architectures
CPU_FEATURE(fpu)
CPU_FEATURE(mmu)
CPU_FEATURE(thumb)
CPU_FEATURE(jazelle)
CPU_FEATURE(thumb_ee)

// Thumb instruction set variants
THB_FEATURE(thumb_v4)   // original thumb instructions
THB_FEATURE(thumb_v5)   // added at ARMv5 (includes CLZ & BKPT)
THB_FEATURE(thumb_v5j)  // added at ARMv5TEJ (includes BXJ & other Jazelle)
THB_FEATURE(thumb2_v6)  // added at ARMv6T2: added similar to ARM functionality 

// Floating point variants (extensions)
FPU_FEATURE(VFPv2)      // floating point extension V5-V6
FPU_FEATURE(VFPv3)      // floating point extension V7-A/R
FPU_FEATURE(VFPv3U)     // V3 with exception trapping
FPU_FEATURE(VFPv3H)     // V3 with half-precision extension
FPU_FEATURE(VFPv4)      // V7-A/R extension
FPU_FEATURE(VFPv4U)     // V4 with exception trapping

FPU_FEATURE(ASIMDv1)    // V7-A/R extension
FPU_FEATURE(ASIMDV1H)   // V1 with half-precision extension
FPU_FEATURE(ASIMDv2)    // V7-A/R extension (extends V1H)
 
// declare fpu features

// declare mmu features

// clear for next run
#undef CPU_FEATURE
#undef THB_FEATURE
#undef FPU_FEATURE
#undef MMU_FEATURE
