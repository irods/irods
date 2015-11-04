#
# clients/fuse/Makefile
#
# Build the iRODS fuse interface.
#
# The principal targets include:
#
#	all		build all of the tools
#	clean		clean out all object files
#

include ../../config/external_versions.txt

ifndef buildDir
buildDir =	$(CURDIR)/../..
endif

include $(buildDir)/config/directories.mk
include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/common.mk

#
# Directories
#
fuseDir =	$(buildDir)/clients/fuse

objDir =	$(fuseDir)/obj
binDir =	$(fuseDir)/bin
srcDir =	$(fuseDir)/src
incDir =	$(fuseDir)/include

reSrcDir =	$(buildDir)/server/re/src
reObjDir =	$(buildDir)/server/re/obj
reIncDir =	$(buildDir)/server/re/include







#
# Source files
#
OBJS =		\
		$(objDir)/irodsFs.o 

BINS =		\
		$(binDir)/irodsFs \

LIB_OBJS =	\
		$(objDir)/iFuseCmdLineOpt.o \
		$(objDir)/iFuseOper.o \
		$(objDir)/iFuse.Preload.o \
		$(objDir)/iFuse.BufferedFS.o \
		$(objDir)/iFuse.Lib.o \
		$(objDir)/iFuse.Lib.Conn.o \
		$(objDir)/iFuse.Lib.Fd.o \
		$(objDir)/iFuse.Lib.Util.o \
		$(objDir)/iFuse.Lib.RodsClientAPI.o \
		$(objDir)/iFuse.FS.o

INCLUDES +=	-I$(incDir) -I$(reIncDir) 





#
# Compile and link flags
#

CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)
ifdef IRODS_FS
CFLAGS_OPTIONS += -D_REENTRANT
CFLAGS_OPTIONS += -D_MYSQL_ICAT_DRIVER_PATCH_
    ifeq (${OS_platform}, osx_platform)
    INCLUDES +=    -I$(fuseHomeDir)/include/osxfuse
    else
    INCLUDES +=	-I$(fuseHomeDir)/include
    endif
endif




CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(LIB_INCLUDES) $(SVR_INCLUDES) $(MODULE_CFLAGS)

ifeq ($(OS_platform), linux_platform)
LDFLAGS +=	$(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib -lfuse
else
    LDFLAGS +=     $(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib
    ifeq ($(OS_platform), osx_platform)
    LDFLAGS +=     -losxfuse
    else
    LDFLAGS +=     -lfuse
    endif
endif

ifeq ($(OS_platform), linux_platform)
    LDRTFLAGS = -lrt
endif

# for checking memleak
# LDFLAGS +=	-L/data/mwan/rods/ccmalloc/ccmalloc-0.4.0/lib

LDFLAGS +=	$(LIBRARY)  $(LIB_CLIENT_EXEC)  -ldl

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a \
             $(BOOST_DIR)/stage/lib/libboost_chrono.a

ifdef USE_SSL
LDFLAGS        += -lssl -lcrypto
endif

CFLAGS:=$(filter-out -DUSE_BOOST,$(CFLAGS))

# include jansson for zonereport
CFLAGS += -I../../../external/$(JANSSONVERSION)/src/
LDFLAGS += ../../../external/$(JANSSONVERSION)/src/.libs/libjansson.a


#
# Principal Targets
#
.PHONY: all fuse clients clean print_cflags print_ldflags
all:	fuse
	@true

clients: fuse
	@true

fuse:	$(LIB_OBJS) $(OBJS) $(BINS)
	@true

$(objDir)/%.o:	$(srcDir)/%.cpp $(LIBRARY)
	@echo "Compile fuse `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

$(reObjDir)/%.o:	$(reSrcDir)/%.c $(LIBRARY)
	@echo "Compile fuse `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o $(LIB_OBJS)
	@echo "Link fuse `basename $@`..."
	$(V_at)$(LDR) -o $@ $< $(LIB_OBJS) $(SVR_OBJS) $(LDFLAGS) $(BOOST_LIBS) $(LDRTFLAGS) -lcrypto -lssl





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"

# Show link flags
print_ldflags:
	@echo "Link flags:"
	@echo "    $(LDFLAGS)"





# Clean
clean:
	@echo "Cleaning fuse..."
	@rm -f $(BINS) $(OBJS) $(LIB_OBJS)

