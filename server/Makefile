# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-Wall -g -lpthread -pthread -std=gnu99 -I../
LDFLAGS=-lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs

tecnicofs: fs/state.o fs/operations.o main.o circularqueue/circularqueue.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs fs/state.o fs/operations.o circularqueue/circularqueue.o main.o 

fs/state.o: fs/state.c fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/operations.o -c fs/operations.c

circularqueue/circularqueue.o: circularqueue/circularqueue.c circularqueue/circularqueue.h
	$(CC) $(CFLAGS) -o circularqueue/circularqueue.o -c circularqueue/circularqueue.c

main.o: main.c fs/operations.h fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o main.o -c main.c

clean:
	@echo Cleaning...
	rm -f fs/*.o *.o circularqueue/*.o tecnicofs

run: tecnicofs
	./tecnicofs
