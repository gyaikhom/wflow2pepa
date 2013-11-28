/*********************************************************************

  THE ENHANCE PROJECT
  School of Informatics,
  University of Edinburgh,
  Edinburgh - EH9 3JZ
  United Kingdom


  DESCRIPTION:

  This file contains the grammar for the hierarchical description
  language which is used to generate a parser. The remaining part
  consists of the code for checking the execution options supplied by
  the programmer. The functions that are invoked here are declared in
  the file "pepa.h"; which are defined in "pepa.c".

  Written by: Gagarine Yaikhom

*********************************************************************/
%{
    #include <getopt.h>
    #include <stdio.h>
    #include "pepa.h"
    void yyerror(char const *s);
%}

%union {
    int ival;
    double dval;
    char *sptr;
}

/* Bison declarations. */
%token <ival> TINTG
%token <dval> TDOUB
%token <sptr> TSTRG
%token TPIPE
%token TDEAL
%token TXDEAL
%token TFARM
%token TXFARM
%token TTASK
%token TSEMI
%token TLPAR
%token TRPAR
%token TCOMMA

%type <dval> exp

%left TMINUS TPLUS
%left TTIMES TDIVIDE
%left TNEG       /* negation--unary minus */
%right TEXPO     /* exponentiation */

%% /* The grammar follows.  */
input:    /* empty */
        | input line
    ;

line:     TSEMI
        | stmt TSEMI
    ;

stmt:     TPIPE TLPAR TINTG TRPAR { pipe($3); }
        | TDEAL TLPAR TINTG TCOMMA TSTRG TCOMMA exp TRPAR { deal($3, $5, $7); }
        | TXDEAL TLPAR TINTG TRPAR { xdeal($3); }
        | TFARM TLPAR TINTG TCOMMA TSTRG TCOMMA exp TRPAR { farm($3, $5, $7); }
        | TXFARM TLPAR TINTG TRPAR { xfarm($3); }
        | TTASK TLPAR TSTRG TCOMMA exp TRPAR { task($3, $5); }
    ;

exp:      TDOUB                 { $$ = $1;          } 
        | TINTG                 { $$ = $1;          }
        | exp TPLUS exp         { $$ = $1 + $3;     }
        | exp TMINUS exp        { $$ = $1 - $3;     }
        | exp TTIMES exp        { $$ = $1 * $3;     }
        | exp TDIVIDE exp       { $$ = $1 / $3;     }
        | TMINUS exp %prec TNEG { $$ = -$2;         }
        | exp TEXPO exp         { $$ = pow($1, $3); }
        | TLPAR exp TRPAR        { $$ = $2;          }
    ;
%%

/* Called by yyparse on error.  */
void yyerror (char const *s) {
    printf("%s\n", s);
}

int main(int argc, char *argv[]) {
    int c;

    while(1) {
        c = getopt(argc, argv, "aghlo");
        if (c == -1)
            break;

        switch(c) {
        case 'g':
            graph = 1;
            break;
        case 'h':
            fprintf(stderr,
                    "Usage: wflow2pepa [OPTIONS] <file>\n\n"
                    "Options:\n"
                    "  -a  Generate complete (graph, latex, ps etc.).\n"
                    "  -g  Generate dot graph.\n"
                    "  -h  Show this help message.\n"
                    "  -l  Generate LaTeX source file.\n"
                    "  -o  Output into a file.\n\n"
                    "Copyright 2006 Enhance Project\n"
                    "University of Edinburgh, United Kingdom\n"
                    "Contact: Gagarine Yaikhom (g.yaikhom@inf.ed.ac.uk)\n");
            exit(0);
        case 'l':
            latex = 1;
            break;
        case 'o':
            output = 1;
            break;
        case 'a':
            graph = 1;
            latex = 1;
            output = 1;
            complete = 1;
            break;
        case '?':
            break;
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);

        }
    }

    if (optind >= argc) {
        printf("ERROR: No input file.\n");
        exit(1);
    }
    fname = strdup(argv[optind]);
    *(strrchr(fname, '.') + 1) = '\0';
    init_lex(argv[optind]);
    yyparse();
    final_lex();
    generate();
    free(fname);
}
