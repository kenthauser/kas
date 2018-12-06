/* grammar for Z80 assembly language	*/

%{
#include <strings.h>
#include <malloc.h>
#include "as.h"

#define MAX_ARGS 50		/* size of static arrays */
#define MAX_EXPR 200

static AARG aargv[MAX_ARGS];
static EXPR expv[MAX_EXPR];
static int aargc, expc;

/* declare the pseudo op functions */
void s_ident (), s_globl (), s_align (), s_org ();
void s_ascii (), s_skip (), s_comm (), s_lcomm ();
void s_even ();

static EXPR    dot;		/* holds the value of ``.'' */

%}

%union {
  int i;
  char *cp;
  EXPR *ep;
  SYMBOL *sp;
  enum machine_reg r;
}

%token       DOT
%token	<sp> IDENT OPCODE LLBL
%token	<r>  REGISTER
%token  <cp> STRING
%token  <i>  NUMBER
%token	<i>  ASCII ASCIZ BYTE BYTEZ WORD LONG TEXT DATA DATA1 DATA2 BSS
%token	<i>  GLOBL COMM LCOMM SKIP ALIGN EVEN

%type	<cp> s_list
%type 	<ep> expr

%left	'<' '>'
%left	'|'
%left	'^'
%left	'&'
%left	'+' '-'
%left	'*' '/' '%'
%right	UMINUS '~'

%%

program		: /* empty */
  		| program line
			 	{
				if (pass2) print_line ();
				new_dot ();
				++lineno;
				}

  		;

line		: '\n'		{ if (pass2) print_addr (LST_NO_OBJ); }
		| label line	{ if (pass2) print_addr (LST_OK); }
		| statement '\n'
		| statement '!' { new_dot ();} line
		| statement error '\n'
			{ warn ("junk following statement -- ignored");
			  yyerrok;}

		;

statement	: error	args		{ error ("Invalid opcode"); }
		| OPCODE args		{ z80_ip ($1, aargc, aargv); }
		| DOT '=' expr 		{ s_org ($3); }
		| TEXT 	 		{ set_fragp (&textp); }
		| DATA 	 		{ set_fragp (&datap); }
		| DATA1	 		{ set_fragp (&data1p); }
		| DATA2	 		{ set_fragp (&data2p); }
		| BSS	 		{ set_fragp (&bssp); }
		| GLOBL expr_list 	{ s_globl (aargc, aargv); }
		| COMM 	expr_list 	{ s_comm (aargc, aargv); }
		| LCOMM expr_list 	{ s_lcomm (aargc, aargv); }
		| SKIP	expr 		{ s_skip ($2);}
		| ALIGN	expr 		{ s_align ($2); }
		| EVEN	 		{ s_even (); }
		| ASCII s_list 		{ s_ascii ($2); free ($2);}
		| ASCIZ s_list 		{ s_ascii ($2); free ($2); put_byte (0);}
		| IDENT '=' expr	{ if (pass2) {
		  				print_addr (LST_EQU);
						show_expr ($3, 2);
					      }
		  			  s_ident (EQU, $1, $3); }
		| BYTE 	expr_list 
				{
				  AARG *p = &aargv[0];
				  int i;

				  for (i = aargc;i--; p++)
				    if (expr_seg (p->expr) == E_ABS)
				      put_byte (expr_eval (p->expr));
				    else
				      put_expr (expr_cpy (p->expr), 1);
				}
		| BYTEZ	expr_list 	{ s_skip (aargc); }
		| WORD 	expr_list 
				{
				  AARG *p = &aargv[0];
				  int i;

				  for (i = aargc;i--; p++)
				    if (expr_seg (p->expr) == E_ABS)
				      put_word (expr_eval (p->expr));
				    else
				      put_expr (expr_cpy (p->expr), 2);
				}
		| LONG 	expr_list 
				{
				  AARG *p = &aargv[0];
				  int i;

				  for (i = aargc;i--; p++)
				    if (expr_seg (p->expr) == E_ABS)
				      put_long (expr_eval (p->expr));
				    else
				      put_expr (expr_cpy (p->expr), 4);
				}
		;

label		: IDENT ':'	{ s_ident (LABEL, $1); last_lbl = $1->name; }  
		| LLBL ':'	{ s_ident (LOCAL_LBL, $1); }
		| NUMBER ':'	{ s_numeric_label ($1, &dot); }
		;

args		: args ',' arg
		| { expc = aargc = 0; } arg
		| /* empty */	{ aargc = 0; }
		;

arg		: '(' expr ')'
	{ aargv[aargc].indirect = 1; aargv[aargc++].expr = $2; }
		| '#' expr
	{ aargv[aargc].indirect = 0; aargv[aargc++].expr = $2; }
		| expr	
	{ aargv[aargc].indirect = 0; aargv[aargc++].expr = $1; }
		;

s_list		: s_list STRING		{ int i = strlen ($2) + 1;
					  if ($1) {
					      i += strlen ($1);
					      $$ = realloc ($1, i);
					      strcat ($$, $2);
					  } else {
					      $$ = malloc (i);
					      strcpy ($$, $2);
					  }
				      	}
		| /* empty */		{ $$ = 0;}
		;

expr_list	: expr_list ',' expr		{ aargv[aargc++].expr = $3; }
  		| { expc = aargc = 0; } expr 	{ aargv[aargc++].expr = $2; }
		| /* empty */ 			{ expc = aargc = 0; }
		;


expr		: IDENT		{ $$ = &expv[expc++];
				  CLEAR_EXPR ($$);
				  $$->plus = $1;
				}


		| LLBL		{ $$ = &expv[expc++];	/* same as above */
				  CLEAR_EXPR ($$);
				  $$->plus = $1;
				}


		| NUMBER	{ $$ = &expv[expc++];
				  CLEAR_EXPR ($$);
				  $$->offset = $1;
				  $$->seg = E_ABS;
				}
  		| REGISTER	{ $$ = &expv[expc++];
				  CLEAR_EXPR ($$);
				  $$->reg = $1;
				  $$->seg = E_REGISTER;
				}

		| DOT		{ $$ = &expv[expc++];
				  CLEAR_EXPR ($$);
				  $$->plus = dot_symbol (&dot);
				}
  		| '(' expr ')'	{ $$ = $2; }
		| expr '+' expr	{ $$ = expr_op ('+', $1, $3); }
		| expr '-' expr	{ $$ = expr_op ('-', $1, $3); }
		| expr '*' expr	{ $$ = expr_op ('*', $1, $3); }
		| expr '/' expr	{ $$ = expr_op ('/', $1, $3); }
		| expr '%' expr	{ $$ = expr_op ('%', $1, $3); }
		| expr '&' expr	{ $$ = expr_op ('&', $1, $3); }
		| expr '|' expr	{ $$ = expr_op ('|', $1, $3); }
		| expr '^' expr	{ $$ = expr_op ('^', $1, $3); }
		| expr '>' '>' expr { $$ = expr_op ('r', $1, $4); }
		| expr '<' '<' expr { $$ = expr_op ('l', $1, $4); }
		| '~' expr  	{ $$ = expr_op ('~', $2); }
		| '-' expr %prec UMINUS
				{ $$ = expr_op (UMINUS, $2); }

		;
%%

/*ARGSUSED*/
yyerror (s)
char *s;
{}

/* routines to handle `dot' */

EXPR *
get_dot ()
{
  return &dot;
}

void
new_dot ()
{
  dot.frag = fp;
  dot.offset = fp->offset;
  dot.seg = fp->seg;
}
