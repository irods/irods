# irodsBuildDir is the directory where the iRODS server is built

# buildDir is the directory where the irods lib is located
ifndef buildDir
buildDir = $(CURDIR)/../../..
endif

hdfLibDir = $(irodsBuildDir)/module/hdf5/obj

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

# Compiler flags go here.
CFLAGS = -g -I$(irodsBuildDir)/proxy/hdf5/include
CFLAGS+= $(MY_CFLAG) -I$(irodsBuildDir)/src/include -I$(irodsBuildDir)/src/catalog/include -I$(buildDir)/tape/include

# Linker flags go here. Currently there aren't any, but if we'll switch to
# code optimization, we might add "-s" here to strip debug info and symbols.
LDFLAGS+= -lSrbClient -L $(buildDir) -lpthread -lm

# use this command to erase files.
RM = /bin/rm -f

# list of generated object files.
OBJS = $(hdfLibDir)/h5Ctor.o \
$(hdfLibDir)/h5Dtor.o $(hdfLibDir)/h5ClHandler.o 	\
$(hdfLibDir)/clH5File.o $(hdfLibDir)/clH5Dataset.o	\
$(hdfLibDir)/clH5Group.o


# program executable file name.
PROG = t5 

# top-level rule, to compile everything.
all: $(PROG)

# rule to link the program
$(PROG): test_h5File.o
	$(CC) $(OBJS) test_h5File.o $(LDFLAGS) -o $(PROG)

# rule for file "test_h5File.o".
test_h5File.o: test_h5File.c
	$(CC) $(CFLAGS) -c test_h5File.c -o test_h5File.o

# rule for test
test:
	$(PROG) test.h5

# rule for cleaning re-compilable files.
clean:
	$(RM) $(PROG) test_h5File.o
