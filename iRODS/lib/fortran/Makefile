INCLUDES = -I ./include
II_INCLUDES =  -I ../core/include -I ../api/include -I ../md5/include -I../rbudp/include -I../../server/core/include -I ../../server/icat/include -I ../lib/api/include -I../../server/drivers/include -I../../server/re/include
CCFLAGS = -g
all:: test1 test2 test3

test1: test1.f90 fortran_io.o
	f95 fortran_io.o -L../core/obj -l RodsAPIs -lpthread  -o test1 test1.f90

test2: test2.f90 fortran_io.o
	f95 fortran_io.o -L../core/obj -l RodsAPIs -lpthread  -o test2 test2.f90

test3: test3.f90 fortran_io.o
	f95 fortran_io.o -L../core/obj -l RodsAPIs -lpthread  -o test3 test3.f90

fortran_io.o: src/fortran_io.c
	$(CC) $(CCFLAGS) $(II_INCLUDES) -c src/fortran_io.c 

clean:
	rm fortran_io.o test1 test2 test3
