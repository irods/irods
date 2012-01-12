# Makefile - makefile for commandtest files

COMTESTOBJ = writebigfile.o
COMTESTBIN = writebigfile

CC=gcc

TARGETS:= $(COMTESTBIN)

all:: $(TARGETS)

clean:
	rm -f $(TARGETS) $(COMTESTOBJ) 

$(COMTESTOBJ): %.o: %.c
	$(CC) -c -g $(CL_CFLAGS) -o $@ $<

$(COMTESTBIN): %: %.o
	$(CC) -g -o $@ $^ 
