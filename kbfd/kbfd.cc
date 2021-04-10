#include "kbfd.h"

// include implementation files
#include "kbfd_reloc_ops_impl.h"
#include "kbfd_object_impl.h"
#include "kbfd_target_format_impl.h"
#include "kbfd_section_impl.h"
#include "kbfd_section_sym_impl.h"

#include "kbfd_format_elf_write.h"

// include all supported target formats
#include "target/m68k_elf.h"
#include "target/z80_aout.h"


namespace kbfd
{
#if 1
kbfd_target_format const *get_obj_format(const char *target, const char *format)
{
    //static m68k::m68k_elf obj;
    static z80::z80_aout obj;
    return &obj;
}
#endif
}
