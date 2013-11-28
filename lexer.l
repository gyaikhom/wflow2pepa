%{

   /*********************************************************************

    THE ENHANCE PROJECT
    School of Informatics,
    University of Edinburgh,
    Edinburgh - EH9 3JZ
    United Kingdom


    DESCRIPTION:

    This file contains all the description of the lexical analyser.

    Written by: Gagarine Yaikhom

    *********************************************************************/

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "parser.h"

    #define MAX_TASK_NAME 64
    char buffer[MAX_TASK_NAME];

    #ifdef DEBUG
    #define PRINT(X,...) printf(X,...);
    #else
    #define PRINT(X,...)
    #endif
%}

%option stack
%x COMMENT
%x STRINGS

DIGIT    [0-9]

%%

{DIGIT}+ { yylval.ival = atoi(yytext); PRINT("%d", yylval.ival); return TINTG; }
{DIGIT}+"."{DIGIT}* { yylval.dval = atof(yytext); PRINT("%f", yylval.dval); return TDOUB; }

 /* Patterns. */
pipe { PRINT("pipe"); return TPIPE; }
xdeal { PRINT("xdeal"); return TXDEAL; }
deal { PRINT("deal"); return TDEAL; }
xfarm { PRINT("xfarm"); return TXFARM; }
farm { PRINT("farm"); return TFARM; }
task { PRINT("task"); return TTASK; }

 /* Comments. */
"/*"           { yy_push_state(COMMENT); PRINT("starting comment\n"); }
<COMMENT>.     { PRINT("%s", yytext); }
<COMMENT>"*/"  { yy_pop_state(); PRINT("ending comment\n"); }

 /* Strings. */
"\""           { yy_push_state(STRINGS); PRINT("<"); strcpy(buffer, ""); }
<STRINGS>[^"]* { PRINT("%s", yytext); strcat(buffer, yytext); }
<STRINGS>"\"" { yy_pop_state(); PRINT(">"); yylval.sptr = buffer; return TSTRG; }

 /* Whitespace. */
" "   { PRINT(" "); }
"\t"  { PRINT("\t"); }
"\n"+ { PRINT("\n"); }

 /* Operators */
"("   { PRINT("("); return TLPAR; }
")"   { PRINT(")"); return TRPAR; }
","   { PRINT(","); return TCOMMA; }
";"   { PRINT(";"); return TSEMI; }
"+"   { PRINT("+"); return TPLUS; }
"-"   { PRINT("-"); return TMINUS; }
"*"   { PRINT("*"); return TTIMES; }
"/"   { PRINT("/"); return TDIVIDE; }
"^"   { PRINT("^"); return TEXPO; }

 /* Everything else. */
.     { PRINT("%s", yytext); }

%%

int init_lex(char *fname) {
    if ((yyin = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "[ cannot open %s ]\n", fname);
        exit(1);
    }
    return 0;
}

int final_lex(void) {
    fclose(yyin);
    /* yylex_destroy(); */
    return 0;
}
