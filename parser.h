/* A Bison parser, made by GNU Bison 2.2.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TINTG = 258,
     TDOUB = 259,
     TSTRG = 260,
     TPIPE = 261,
     TDEAL = 262,
     TXDEAL = 263,
     TFARM = 264,
     TXFARM = 265,
     TTASK = 266,
     TSEMI = 267,
     TLPAR = 268,
     TRPAR = 269,
     TCOMMA = 270,
     TPLUS = 271,
     TMINUS = 272,
     TDIVIDE = 273,
     TTIMES = 274,
     TNEG = 275,
     TEXPO = 276
   };
#endif
/* Tokens.  */
#define TINTG 258
#define TDOUB 259
#define TSTRG 260
#define TPIPE 261
#define TDEAL 262
#define TXDEAL 263
#define TFARM 264
#define TXFARM 265
#define TTASK 266
#define TSEMI 267
#define TLPAR 268
#define TRPAR 269
#define TCOMMA 270
#define TPLUS 271
#define TMINUS 272
#define TDIVIDE 273
#define TTIMES 274
#define TNEG 275
#define TEXPO 276




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 28 "parser.y"
{
	int ival;
	double dval;
	char *sptr;
}
/* Line 1528 of yacc.c.  */
#line 97 "parser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

