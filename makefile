CFLAGS = -DYYDEBUG=1
all:isql clean

#isql: isql.l isql.y
#	bison -d isql.y
#	flex -o isql.lex.c isql.l
#	cc -o $@ isql.tab.c isql.lex.c

isql: isql.tab.o isql.lex.o isql_main.o
	cc -g -o $@ isql.tab.o isql.lex.o isql_main.o -lm

isql.tab.c isql.tab.h: isql.y
	bison -vd isql.y

isql.lex.c: isql.l
	flex -o $@ $<

isql.lex.o: isql.lex.c isql.tab.h

isql_main.o: isql_main.c isql_main.h

clean:
	rm -f isql.lex.o isql.tab.o isql_main.o

cleanall:
	rm -f isql isql.tab.c isql.tab.h isql.lex.c isql.output isql.lex.o isql.tab.o isql_main.o
