#ifndef KAS_TARGET_TGT_MCODE_COND_H
#define KAS_TARGET_TGT_MCODE_COND_H



namespace kas::tgt::opc::traits
{

// declare condition-code trait
template <int value, typename NAME>
struct cc_trait
{
    using name = NAME;
    using code = std::integral_constant<int, value>;
};
}

namespace kas::tgt::opc
{

// forward declare base type
template <typename MCODE_T> struct tgt_mcode_sizes;

template <typename MCODE_T, typename CC_LIST = void>
struct tgt_mcode_cond : tgt_mcode_sizes<MCODE_T>
{
    using base_t = tgt_mcode_sizes<MCODE_T>;
    
    // expose base type ctors
    using base_t::base_t;

    template <typename...SZ, typename CC, typename SHIFT>
    constexpr tgt_mcode_cond(meta::list<meta::list<SZ...>, CC, SHIFT>)
        : index  { meta::find_index<CC_LIST, CC>::value + 1 }
        , shift  { SHIFT::value } 
        , base_t { meta::list<SZ...>() }
        {}

    uint8_t index{};
    uint8_t shift{};
};


// default to BASE_T if no condition-code list
template <typename MCODE_T>
struct tgt_mcode_cond<MCODE_T, void> : tgt_mcode_sizes<MCODE_T> 
{
    // just inherit base_t ctors
    using base_t = tgt_mcode_sizes<MCODE_T>;
    using base_t::base_t;
};

}


#endif

