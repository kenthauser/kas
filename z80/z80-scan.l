/* flex spec for Zilog z80 assembly language
 *
 * $Id: z80-scan.l,v 1.2 1997/12/01 23:26:13 kent Exp $
 *
 * Copyright (c) 1989, Kent Hauser
 * 
 * This file is part of AS, a retargetable assembler.
 * 
 * AS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, Inc., Version 1.
 */

%{
#ifndef lint
static char *rcsid = "$Id: z80-scan.l,v 1.2 1997/12/01 23:26:13 kent Exp $";
#endif

#include	<stdio.h>
#include	<strings.h>
#include	"as.h"
#include	"parse.tab.h"	/* token names from bison */

YYSTYPE yylval;

#undef yywrap			/* for flex */
%}

comment_chars	[;]
char		\'([^'\n]|\\['\n])*\'
string		\"([^"\n]|\\["\n])*\"
alpha		[a-zA-Z_]
alphanum	[a-zA-Z0-9_\$\.]

/* bsd pseudo ops */
%%
".ascii"		return ASCII;
".asciz"		return ASCIZ;
".byte"			return BYTE;
".bytez"		return BYTEZ;
".word"			return WORD;
".long"			return LONG;
".text"			return TEXT;
".data"			return DATA;
".data1"		return DATA1;
".data2"		return DATA2;
".bss"			return BSS;
".globl"		return GLOBL;
".comm"			return COMM;
".lcomm"		return LCOMM;
".skip"			return SKIP;
".align"		return ALIGN;
".even"			return EVEN;
".xor."			return '^';
".or."			return '|';
".and."			return '&';

{alpha}{alphanum}*	return lookup (yytext, yyleng);
[0-9]+"$"               return llbl_lookup (yytext, yyleng);
[0-9][bf]		return nlbl_lookup (yytext[0] - '0', yytext[1] == 'f');

[0-9][0-9a-fA-F]*[hH]	|
0[xX][0-9a-fA-F]*	{ yylval.i = strtol (yytext, (char **)NULL, 16);
				  return NUMBER;}
0[0-7]+			{ yylval.i = strtol (yytext, (char **)NULL, 8);
				  return NUMBER;}

[0-9]+			{ int i = 0; short j = yyleng;
			  char *p = yytext;
		  /* the following causes gcc to generate amazing code */
			  while (j--){
			    i *= 10;
			    i += *(unsigned char *)p++ - '0';
			    }
			  yylval.i = i;
			  return NUMBER;
			}
			    

{char}			{ int i, j = 0;
			  char *p = &yytext[1];
			  yytext[yyleng-1] = '\0';  /* trailing quote */

			  while ((i = decode_string (&p)) != -1) {
			    j <<= 8;
			    j += i;
			  }
			  yylval.i = j;
			  return NUMBER;
			}			    

{string}		{ yytext[yyleng-1] = '\0';
			  yylval.cp = &yytext[1];
     			  return STRING;
			}


\.			return DOT;

^#.*\n			set_lineno ();

{comment_chars}.*	; /* comment delimiters */
[ \t\f]+		; /* eat whitespace */
.			return yytext[0];
\n			return yytext[0];
%%

static char *source_file;	/* this is the main sorce file */

yywrap ()
{
    if (!pass2)
	end_pass1 ();
    if (!lflag || pass2)
	return 1;
    
    pass2 = 1;	
    last_lbl = 0;
    init_input (source_file);
    return 0;
}

init_input (ifile)
     char *ifile;
{
    source_file = filename = ifile;
    if (freopen (ifile, "r", stdin) == 0)
	fatal ("Opening input file \"%s\": %s",
	       ifile, strerror (errno));
    lineno = 1;
    stab_lineno = 0;
    stab_filename = 0;
    new_dot ();
}

set_lineno ()
{
    char fname_buf[200], *p;
    int n;
    
    ++lineno;
    if ((sscanf (yytext, "# %d %s", &lineno, fname_buf) == 2) ||
	(sscanf (yytext, "# line %d %s", &lineno, fname_buf) == 2)) {
	n = strlen (fname_buf);
	if (fname_buf[0] == '"') {
	    p = &fname_buf[1];
	    fname_buf[--n] = '\0';
	}
	else
	    p = fname_buf;
	
	if (strcmp (filename, p))				
	    filename = strcpy (xmalloc (n + 1), p);
    }
}

/* This function converts strings to bytes.
 * It understands about `\' style escapes of all flavors.
 * It is the translator used for both character & string constants.
 *
 * called with pointer to string, it returns next byte or -1 on
 * end of string.
 *
 * See example in {char} above for usage.
 */

decode_string (s)
     char **s;
     
{
    char *p, c, hex;
    int i;
    
    if ((c = *(*s)++) != '\\')
	return c == '\0' ? -1 : c;
    
    switch (c = *(*s)++) {
    default:
	return c;
    case 0:
	return -1;
    case 'n':
	return '\n';
    case 'r':
	return '\r';
    case 'b':
	return '\b';
    case 'f':
	return '\f';
    case 'a':
	return '\a';
    case 't':
	return '\t';
    case 'x':
    case 'X':
	/* change from \[xX] format to \0[xX] format */
	(--*s)[-1] = '0';
	/*FALLSTHRU*/
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
	/* save, then nuke, the last possible char of digit */
	hex = (*s)[0] == 'x' || (*s)[0] == 'X';
	p = &(*s)[2 + hex];
	c = *p;
	*p = '\0';
	
	i = strtol (&(*s)[-1], s, hex ? 16 : 8);
	*p = c;
	return i & 0xff;
    }
    /*NOTREACHED*/
}

/*
 * Local Variables:
 * backup-by-copying-when-linked: t
 * End:
 */
