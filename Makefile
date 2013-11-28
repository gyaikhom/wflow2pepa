#*********************************************************************
#
#  THE ENHANCE PROJECT
#  School of Informatics,
#  University of Edinburgh,
#  Edinburgh - EH9 3JZ
#  United Kingdom
#
#  Written by: Gagarine Yaikhom
#
#*********************************************************************

CC      = gcc
LEX     = flex
YACC    = bison
CFLAGS  = -g
LDFLAGS = -lfl -lm

wflow2pepa: lexer.o parser.o pepa.o
	${CC} ${CFLAGS} -o wflow2pepa lexer.o parser.o pepa.o ${LDFLAGS}

pepa.o: pepa.c
	${CC} ${CFLAGS} -c pepa.c

lexer.o: lexer.c parser.c
	${CC} ${CFLAGS} -c lexer.c

parser.o: parser.c
	${CC} ${CFLAGS} -c parser.c

lexer.c: lexer.l parser.c
	${LEX} -o lexer.c lexer.l

parser.c: parser.y
	${YACC} -d -o parser.c parser.y

clean:
	rm -f *.o *~
