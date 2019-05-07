#ifndef KAS_TARGET_TGT_VALIDATE_IMPL_H
#define KAS_TARGET_TGT_VALIDATE_IMPL_H

#include "tgt_validate.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
void tgt_validate_args<MCODE_T>::print(std::ostream& os) const
{
    os << MCODE_T::BASE_NAME::value << "::validate_args:";
    os << " count = " << +arg_count << " indexs = [";
    char delim{};
    for (auto n = 0; n < arg_count; ++n)
    {
        if (delim) os << delim;
        os << +arg_index[n];
        delim = ',';
    }
    os << "] vals = [";
    delim = {};
    for (auto n = 0; n < arg_count; ++n)
    {
        if (delim) os << delim;
        os << names_base[arg_index[n]];
        delim = ',';
    }
    os << "]" << std::endl;
}

}
#endif
