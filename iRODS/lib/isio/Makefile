INCLUDES = -I ./include
II_INCLUDES =  -I ../core/include -I ../api/include -I ../md5/include -I../rbudp/include -I../../server/core/include -I ../../server/icat/include -I ../lib/api/include -I../../server/drivers/include -I../../server/re/include
CCFLAGS = -g
all:: test1 test2 test3

test1: test1.c isio.o
	$(CC) $(CCFLAGS) $(INCLUDES) isio.o -L../core/obj -l RodsAPIs -lpthread  -o test1 test1.c

test2: test2.c isio.o
	$(CC) $(CCFLAGS) $(INCLUDES) isio.o -L../core/obj -l RodsAPIs -lpthread  -o test2 test2.c

test3: test3.c isio.o
	$(CC) $(CCFLAGS) $(INCLUDES) isio.o -L../core/obj -l RodsAPIs -lpthread  -o test3 test3.c

isio.o: src/isio.c
	$(CC) $(CCFLAGS) $(II_INCLUDES) -c src/isio.c 

clean:
	rm isio.o test1 test2 test3
