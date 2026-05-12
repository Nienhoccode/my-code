CC = gcc
CFLAGS = -Wall -g
INPUT = data.txt

TARGETS = fcfs sjf rr

all: $(TARGETS) run

fcfs: driver.o fcfs.o
	$(CC) $(CFLAGS) -o fcfs driver.o fcfs.o -lpthread

sjf: driver.o sjf.o
	$(CC) $(CFLAGS) -o sjf driver.o sjf.o -lpthread

rr: driver.o rr.o
	$(CC) $(CFLAGS) -o rr driver.o rr.o -lpthread

fcfs.o: fcfs.c
    $(CC) $(CFLAGS) -c fcfs.c -lpthread

sjf.o: sjf.c
    $(CC) $(CFLAGS) -c sjf.c -lpthread

rr.o: rr.c
    $(CC) $(CFLAGS) -c rr.c -lpthread

driver.o: driver.c
    $(CC) $(CFLAGS) -c driver.c -lpthread

run:
	@echo "---------- KET QUA FCFS ----------"
	./fcfs $(INPUT)
	@echo "\n---------- KET QUA SJF ----------"
	./sjf $(INPUT)
	@echo "\n---------- KET QUA RR  ----------"
	./rr $(INPUT)

# clean:
# 	rm -f *.o $(TARGETS)