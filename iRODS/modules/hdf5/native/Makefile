# Makefile - This is the makefile for libjh5srb.so, jh5srb.class. 
SHELL=/bin/csh

ifndef buildDir
buildDir =      $(CURDIR)/../../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

libCoreObjDir = $(buildDir)/lib/core/obj

hdf5MSObjDir =      $(modulesDir)/hdf5/microservices/obj
hdf5MSSrcDir =      $(modulesDir)/hdf5/microservices/src
hdf5MSIncDir =      $(modulesDir)/hdf5/microservices/include

hdf5TestDir =   $(modulesDir)/hdf5/test
hdf5LibIncDir = $(modulesDir)/hdf5/lib/include
hdf5LibObjDir = $(modulesDir)/hdf5/lib/obj
hdf5LibSrcDir = $(modulesDir)/hdf5/lib/src

LIBDIR = .

SLIB = $(LIBDIR)/libjh5srb.so
OBJ  = $(LIBDIR)/jh5srb.o

INCLUDE_FLAGS = -I$(hdf5TestDir) -I$(hdf5Dir)/src -I$(hdf5LibIncDir) -I$(hdf5MSIncDir)
HDF_LIB_OBJECTS = $(hdf5LibObjDir)/h5Ctor.o $(hdf5LibObjDir)/h5Dtor.o \
	$(hdf5LibObjDir)/h5ClHandler.o $(hdf5LibObjDir)/clH5Dataset.o  \
	$(hdf5LibObjDir)/clH5File.o  $(hdf5LibObjDir)/clH5Group.o

ifeq ($(OS_platform), solaris_platform)
PLATFORM=solaris
endif 

ifeq ($(OS_platform), linux_platform)
PLATFORM=linux
endif

ifeq ($(OS_platform), aix_platform)
PLATFORM=aix
CFLAGS+=-c -M -I.
endif

ifeq ($(OS_platform), sgi_platform)
PLATFORM=irix
endif

CFLAGS = $(MY_CFLAG) $(LIB_INCLUDES) $(SVR_INCLUDES) 	\
	-I$(javaDir)/include -I$(javaDir)/include/$(PLATFORM)	\
	$(MODULE_CFLAGS) $(INCLUDE_FLAGS)

# LDFLAGS +=      $(LDADD) $(LIBRARY) $(MODULE_LDFLAGS)
LDFLAGS +=      $(LIBRARY) $(LDADD) $(MODULE_LDFLAGS)

.SUFFIXES: .c .o .so .java .class .h

all: $(SLIB) 
	@ echo $(SLIB) is to date.

$(SLIB): $(OBJ)
ifeq ($(OS_platform), aix_platform)
	grep " JNICALL " *.c|sed "s/.* JNICALL //g">$@.exp
	$(LDR) -lm -bnoentry -bM:SRE -bE:$@.exp -lc_r -blibpath:/usr/lib/threads:/usr/lib:/lib -L/usr/local/apps/Java/lib/aix/native_threads -o $(SLIB) $(HDF_LIB_OBJECTS) $(OBJ) $(LDFLAGS)
	rm $@.exp
else
ifeq ($(OS_platform), sgi_platform)
	$(LDR) -shared -o $(SLIB) $(HDF_LIB_OBJECTS) $(OBJ) $(LDFLAGS)
else
ifeq ($(OS_platform), linux_platform)
	$(LDR) -shared -lm -lpthread -o $(SLIB) $(HDF_LIB_OBJECTS) $(OBJ) $(LDFLAGS)
else
	$(LDR) -G -o $(SLIB) $(HDF_LIB_OBJECTS) $(OBJ) $(LDFLAGS)
endif
endif
endif

$(LIBDIR)/jh5srb.o: jh5srb.c 
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJ) $(SLIB) $(SLIB).exp

