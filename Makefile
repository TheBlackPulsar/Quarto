CC=gcc
CFLAGS= -g -Wall -Werror #-Wextra
INCLUDE_DIR=./include
OBJ=sysprak-client

vpath %.c src
vpath %.h include

${OBJ}: main.c bibs.h performConnection.o performConnection.h socket.o socket.h  spieldata.o spieldata.h handle_rounds.h handle_rounds.o thinker.o config.o
	${CC} ${CFLAGS} -I${INCLUDE_DIR} performConnection.o socket.o spieldata.o handle_rounds.o thinker.o config.o $< -o ${OBJ}

performConnection.o: performConnection.c bibs.h defs.h performConnection.h spieldata.h
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@

socket.o: socket.c bibs.h defs.h socket.h
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@
	
spieldata.o: spieldata.c bibs.h defs.h spieldata.h
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@

handle_rounds.o: handle_rounds.c handle_rounds.h
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@

thinker.o: thinker.c
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@

config.o: config.c
	${CC} ${CFLAGS} -I${INCLUDE_DIR} $< -c -o $@

.PHONY:clean 

clean:
	rm -rf *.o ${OBJ} 