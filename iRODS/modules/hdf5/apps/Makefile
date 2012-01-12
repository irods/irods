# Makefile - This is the makefile for building standalone HDF5/iRODS 
# client application. 
SHELL=/bin/csh

# name of the application program
PROG = islice

# iRODS home direcotry 
IRODSDIR = /home/srb/iRODS

# only need if you need jpeg image output file
JPEGDIR = /home/srb/ext_lib/jpeg

#################################################################
#                 do not change below this line                 #
#################################################################

include $(IRODSDIR)/config/config.mk
include $(IRODSDIR)/config/platform.mk
include $(IRODSDIR)/config/directories.mk
include $(IRODSDIR)/config/common.mk

# these are derived parameters
irodsLibDir = $(IRODSDIR)/lib/core/obj
hdf5LibIncDir = $(IRODSDIR)/modules/hdf5/lib/include
hdf5LibObjDir = $(IRODSDIR)/modules/hdf5/lib/obj
hdf5LibSrcDir = $(IRODSDIR)/modules/hdf5/lib/src

PROG_OBJECTS = $(PROG).o
PROG_SRC = $(PROG).c
HDF_LIB_OBJECTS=$(hdf5LibObjDir)/h5ClHandler.o 	\
    $(hdf5LibObjDir)/clH5Dataset.o         \
    $(hdf5LibObjDir)/clH5File.o $(hdf5LibObjDir)/clH5Group.o	\
    $(hdf5LibObjDir)/h5Ctor.o $(hdf5LibObjDir)/h5Dtor.o

#   $(hdf5LibObjDir)/h5LocalHandler.o	\

INCLUDES = -I$(hdf5LibIncDir)
INCLUDES +=     -I$(IRODSDIR)/lib/core/include -I$(IRODSDIR)/lib/api/include
INCLUDES +=     -I$(IRODSDIR)/lib/md5/include
INCLUDES +=     -I$(IRODSDIR)/server/core/include
INCLUDES +=     -I$(IRODSDIR)/server/icat/include
INCLUDES +=     -I$(IRODSDIR)/server/drivers/include
INCLUDES +=     -I$(IRODSDIR)/server/re/include
INCLUDES +=     -I$(JPEGDIR)/include

CFLAGS = $(MY_CFLAG) $(INCLUDES)

LDFLAGS =      -L$(irodsLibDir) -lRodsAPIs -lpthread -lm
LDFLAGS +=     -L$(JPEGDIR)/lib -ljpeg

all:: $(PROG) 

clean:
	rm -f $(HDF_LIB_OBJECTS) $(PROG) $(PROG_OBJECTS)

$(HDF_LIB_OBJECTS): $(hdf5LibObjDir)/%.o: $(hdf5LibSrcDir)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROG): $(HDF_LIB_OBJECTS) $(PROG_OBJECTS)
	$(CC) $(PROG_OBJECTS) $(HDF_LIB_OBJECTS) $(LDFLAGS) -o $(PROG)

$(PROG_OBJECTS): $(PROG_SRC)
	$(CC) -c $(CFLAGS) -o $@ $<

