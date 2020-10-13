#ifndef KAS_EXPR_PRECEDENCE_H
#define KAS_EXPR_PRECEDENCE_H

namespace kas::expression::precedence
{
// Declare operator precedence

using prec_t = std::uint16_t;

// Don't bother with class enums because scope is just this namespace
enum : prec_t {
      PRI_DEL,      // use this precedence to delete operation
      PRI_TERM,     // terminal element (hardcoded in parser)
      PRI_PFX,      // unary prefix operation (hardcoded in parser)
      PRI_SFX,      // unary suffix operation (hardcoded in parser)
      PRI_PRE_MULT, // precedence is above <defn_expr> operations
      PRI_MULT,
      PRI_DIV,
      PRI_MOD,
      PRI_PRE_ADD,  // precedence is between multiplication & addition
      PRI_PLUS,
      PRI_MINUS,
      PRI_LSHFT,
      PRI_RSHFT,
      PRI_PRE_BIT,  // precedence is between addition and bit-wise ops
      PRI_AND,
      PRI_XOR,
      PRI_OR,
      PRI_LOW,      // precedence is after bit-wise ops
      NUM_PRI       // count of types
    };


struct precedence_c
{
    static constexpr prec_t value(prec_t pri)
    {
        switch(pri)
        {
            default:
                return pri;

            // multiply/divide/modulo are same
            case PRI_DIV:
            case PRI_MOD:
                return PRI_MULT;

            // plus & minus are same
            case PRI_MINUS:
                return PRI_PLUS;

            // left & right shift are same
            case PRI_RSHFT:
                return PRI_LSHFT;
        }
    }
};

}


#endif
