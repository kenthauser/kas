/* create machine register enum */

#define __REG(sym)		REG_ ## sym,

enum machine_reg {
    REG_NONE = 0,
#include "z80-registers.def"
};

#undef __REG



struct opcode {
    union {
	char           *name;
	struct opcode  *next;
    }               u;
    char           *arg_list;
    const unsigned  value;
    char            num_bytes, num_args, name_length;
};
/*
struct z80_incant {
	struct z80_incant *m_next;
	char           *m_operands;
	unsigned        m_opcode;
	short           m_opnum;
	short           m_codenum;
};
*/

typedef struct {
	EXPR           *expr;
	char            indirect;
}               AARG;


#define TM_GETOPT

#define	M_Z80	M_68020
#define	EXEC_INIT	{ 0, 0, M_68020, OMAGIC }

#define	X_LITTLE_ENDIAN
#define ADDR_BYTES 2		/* 16-bit addresses */

#define	FRAG_MD	frag_branch
