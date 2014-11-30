CC     = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall

HWK6= /c/cs323/Hwk6
HWK4= /c/cs323/Hwk4

Bash: $(HWK6)/mainBash.o $(HWK4)/getLine.o $(HWK6)/parse.o process.o
	${CC} ${CFLAGS} -o Bash $(HWK6)/mainBash.o $(HWK4)/getLine.o $(HWK6)/parse.o process.o

process.o : process.c
