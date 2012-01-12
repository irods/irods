#
# modules/hdf5/Makefile
#
# Build the iRODS hdf5 module
#

# MAKE_LOCAL_TEST specifies that build the test program using local hdf5
# library routine instead of the irods client library. Normal it is off
# MAKE_LOCAL_TEST = 1

#
# Source files
#
# Set hdf5Dir to the top-level directory of the
# hdf5 installation.
# hdf5Dir is the hdf5 install directory
hdf5Dir = /Users/antoine/workspace/hdf5-1.8.0/hdf5
# szlibDir=/home/srb/ext_lib/szip
zlibDir= /opt/local

ifndef buildDir
buildDir = $(CURDIR)/../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

#
# Directories
#
hdf5MSObjDir =      $(modulesDir)/hdf5/microservices/obj
hdf5MSSrcDir =      $(modulesDir)/hdf5/microservices/src
hdf5MSIncDir =      $(modulesDir)/hdf5/microservices/include

hdf5TestDir =	$(modulesDir)/hdf5/test
hdf5LibIncDir =	$(modulesDir)/hdf5/lib/include
hdf5LibObjDir =	$(modulesDir)/hdf5/lib/obj
hdf5LibSrcDir =	$(modulesDir)/hdf5/lib/src

ifdef hdf5Dir
MS_OBJECTS	=  $(hdf5MSObjDir)/hdf5MS.o
TEST_OBJECT = $(hdf5TestDir)/test_h5File.o
INCLUDE_FLAGS = -I$(hdf5TestDir) -I$(hdf5Dir)/include -I$(hdf5LibIncDir)  \
    -I$(hdf5MSIncDir) 
ifdef zlibDir
INCLUDE_FLAGS+=  -I$(zlibDir)/include
endif
ifdef szlibDir
INCLUDE_FLAGS+= -I$(szlibDir)/include
endif
HDF_LIB_OBJECTS = $(hdf5LibObjDir)/h5Ctor.o $(hdf5LibObjDir)/h5Dtor.o
HDF_LIB_OBJECTS+=$(hdf5LibObjDir)/h5File.o \
$(hdf5LibObjDir)/h5Dataset.o $(hdf5LibObjDir)/h5String.o        \
$(hdf5LibObjDir)/h5Group.o $(hdf5LibObjDir)/h5Object.o  \
$(hdf5LibObjDir)/h5Attribute.o

INCLUDE_FLAGS+=-DHDF5_MODULE
ifdef MAKE_LOCAL_TEST
TEST_PROG = $(hdf5TestDir)/tl5
INCLUDE_FLAGS+=-DHDF5_LOCAL
HDF_LIB_OBJECTS+=$(hdf5LibObjDir)/h5LocalHandler.o
TEST_LDADD=
else
TEST_PROG = $(hdf5TestDir)/t5
HDF_LIB_OBJECTS+=$(hdf5LibObjDir)/h5ClHandler.o
HDF_LIB_OBJECTS+=$(hdf5LibObjDir)/clH5Dataset.o 	\
    $(hdf5LibObjDir)/clH5File.o $(hdf5LibObjDir)/clH5Group.o
endif
HDF5_LD_LIBS = -L$(hdf5Dir)/lib -lhdf5 -lhdf5_hl
ifdef zlibDir
HDF5_LD_LIBS+= -L$(zlibDir)/lib -lz
endif
ifdef szlibDir
HDF5_LD_LIBS+= -L$(szlibDir)/lib -lsz
endif
TEST_LDADD=$(LDADD) $(LIBRARY)
else
MS_OBJECTS =
INCLUDE_FLAGS =
HDF5_LD_LIBS =
endif

#
# Compile and link flags
#
INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =        $(CFLAGS_OPTIONS) $(INCLUDES) $(MODULE_CFLAGS)

.PHONY: all server client microservices clean
.PHONY: server_ldflags client_ldflags server_cflags client_cflags
.PHONY: print_cflags

# Build everything
all:	test
	@true

# List module's objects and needed libs for inclusion in clients
client_ldflags:
	@true
#	@echo $(HDF5_LD_LIBS) $(HDF_LIB_OBJECTS)

# List module's includes for inclusion in the clients
client_cflags:
	@true
#	@echo $(INCLUDE_FLAGS)

# List module's objects and needed libs for inclusion in the server
server_ldflags:
	@echo $(MS_OBJECTS) $(HDF_LIB_OBJECTS) $(HDF5_LD_LIBS)

# List module's includes for inclusion in the server
server_cflags:
	@echo $(INCLUDE_FLAGS) -DRODS_SERVER

# Build microservices
ifdef hdf5Dir
microservices:	print_cflags $(MS_OBJECTS) $(HDF_LIB_OBJECTS)
	@true
test:	$(TEST_PROG)
else
microservices:
	@echo "The hdf5Dir variable is not set in the hdf5 module Makefile."
	@echo "Either set this to the path to ImageMagick, or disable the hdf5"
	@echo "module before building iRODS."
endif

# Build client additions
client:
	@true

# Build server additions
server:
	@true

# Build rules
rules:
	@true

# Clean
ifdef hdf5Dir
clean:
	@echo "Clean hdf5 module..."
	@rm -f $(MS_OBJECTS) $(HDF_LIB_OBJECTS) $(TEST_OBJECT) $(TEST_PROG)
else
clean:
	@echo "Clean hdf5 module..."
endif


# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS)"

#
# Compile targets
#
$(MS_OBJECTS): $(hdf5MSObjDir)/%.o: $(hdf5MSSrcDir)/%.c $(DEPEND)
	@echo "Compile hdf5 module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(TEST_OBJECT): $(hdf5TestDir)/%.o: $(hdf5TestDir)/%.c $(DEPEND)
	@echo "Compile  $(TEST_OBJECT) `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(HDF_LIB_OBJECTS): $(hdf5LibObjDir)/%.o: $(hdf5LibSrcDir)/%.c $(DEPEND)
	@echo "Compile  $(HDF_LIB_OBJECTS) `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# rule to link the program
$(TEST_PROG): $(OBJS) $(TEST_OBJECT) $(HDF_LIB_OBJECTS)
	$(CC) $(TEST_OBJECT) $(HDF_LIB_OBJECTS) $(HDF5_LD_LIBS) $(TEST_LDADD) -o $(TEST_PROG)

