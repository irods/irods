#
# clients/icommands/Makefile
#
# Build the iRODS command-line tools
#
# The principal targets include:
#
#	all		build all of the commands
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
include $(buildDir)/../packaging/irods_externals_locations.mk

CFLAGS += `../../../packaging/compiler_check.sh 4 2`
#
# Directories
#
icommandsDir =	$(buildDir)/clients/icommands

objDir =	$(icommandsDir)/obj
binDir =	$(icommandsDir)/bin
srcDir =	$(icommandsDir)/src





#
# Source files
#
OBJS =		\
		$(objDir)/iadmin.o \
		$(objDir)/icd.o \
		$(objDir)/ichksum.o   \
		$(objDir)/ichmod.o \
		$(objDir)/icp.o \
		$(objDir)/ienv.o \
		$(objDir)/iexecmd.o \
		$(objDir)/iexit.o \
		$(objDir)/iget.o \
		$(objDir)/iinit.o \
		$(objDir)/ils.o \
		$(objDir)/ilsresc.o \
		$(objDir)/imeta.o \
		$(objDir)/imiscsvrinfo.o \
		$(objDir)/imkdir.o \
		$(objDir)/imv.o \
		$(objDir)/iphymv.o \
		$(objDir)/iput.o \
		$(objDir)/ipwd.o  \
		$(objDir)/iqdel.o \
		$(objDir)/iqmod.o \
		$(objDir)/iqstat.o \
		$(objDir)/iquest.o \
		$(objDir)/ireg.o \
		$(objDir)/irepl.o \
		$(objDir)/irm.o \
		$(objDir)/irmtrash.o \
		$(objDir)/irsync.o \
		$(objDir)/irule.o \
		$(objDir)/isysmeta.o \
		$(objDir)/itrim.o \
		$(objDir)/iuserinfo.o \
		$(objDir)/imcoll.o \
		$(objDir)/ierror.o \
		$(objDir)/ipasswd.o	\
		$(objDir)/ibun.o	\
		$(objDir)/iphybun.o    \
		$(objDir)/ihelp.o \
		$(objDir)/iquota.o \
		$(objDir)/iscan.o \
		$(objDir)/ixmsg.o \
		$(objDir)/idbug.o \
		$(objDir)/ips.o  \
		$(objDir)/ifsck.o \
		$(objDir)/genOSAuth.o \
		$(objDir)/iticket.o \
		$(objDir)/igroupadmin.o \
		$(objDir)/iapitest.o \
		$(objDir)/izonereport.o \
		$(objDir)/irods-grid.o 


BINS =		\
		$(binDir)/iadmin \
		$(binDir)/icd \
		$(binDir)/ichksum \
		$(binDir)/ichmod \
		$(binDir)/icp \
		$(binDir)/ienv \
		$(binDir)/iexecmd \
		$(binDir)/iexit \
		$(binDir)/iget \
		$(binDir)/iinit \
		$(binDir)/ils \
		$(binDir)/ilsresc \
		$(binDir)/imeta \
		$(binDir)/imiscsvrinfo \
		$(binDir)/imkdir \
		$(binDir)/imv \
		$(binDir)/iphymv \
		$(binDir)/iput \
		$(binDir)/ipwd \
		$(binDir)/iqdel \
		$(binDir)/iqmod \
		$(binDir)/iqstat \
		$(binDir)/iquest \
		$(binDir)/ireg \
		$(binDir)/irepl \
		$(binDir)/irm \
		$(binDir)/irmtrash \
		$(binDir)/irsync \
		$(binDir)/irule \
		$(binDir)/isysmeta \
		$(binDir)/itrim \
		$(binDir)/iuserinfo \
		$(binDir)/imcoll \
		$(binDir)/ierror \
		$(binDir)/ipasswd	\
		$(binDir)/ibun	\
		$(binDir)/iphybun \
		$(binDir)/ihelp \
		$(binDir)/iquota \
		$(binDir)/iscan \
		$(binDir)/ixmsg  \
		$(binDir)/idbug  \
		$(binDir)/ips   \
		$(binDir)/ifsck \
		$(binDir)/genOSAuth \
		$(binDir)/iticket \
		$(binDir)/igroupadmin  \
		$(binDir)/iapitest  \
		$(binDir)/izonereport  \
		$(binDir)/irods-grid

# Compile and link flags

CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)

CFLAGS =	$(CFLAGS_OPTIONS) $(LIB_INCLUDES) $(SVR_INCLUDES) $(MODULE_CFLAGS)

LDFLAGS +=	$(LIBRARY) $(LIB_CLIENT_EXEC) $(MODULE_LDFLAGS) $(CL_LDADD) 

BOOST_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/include 
BOOST_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/lib \
    -lboost_system \
	-lboost_filesystem \
	-lboost_regex \
	-lboost_thread \
	-lboost_chrono \
	-lboost_program_options

JANSSON_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/include 
JANSSON_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/lib -ljansson

AVRO_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_AVRO)/include 
AVRO_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_AVRO)/lib  -lavrocpp

ZMQ_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_ZMQ)/include 
ZMQ_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_ZMQ)/lib  -lzmq

CPPZMQ_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_CPPZMQ)/include 

CFLAGS += $(BOOST_INCLUDES) $(JANSSON_INCLUDES) $(AVRO_INCLUDES) $(ZMQ_INCLUDES) $(CPPZMQ_INCLUDES)
LDFLAGS += $(BOOST_LIBS) $(JANSSON_LIBS) $(AVRO_LIBS) $(ZMQ_LIBS)

RPATH = "-Wl,-rpath,$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/lib" \
        "-Wl,-rpath,$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/lib" \
        "-Wl,-rpath,$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_AVRO)/lib" \
        "-Wl,-rpath,$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_ZMQ)/lib" \

LDFLAGS += -lz $(RPATH)

ifneq (${OS_platform}, osx_platform)
LDFLAGS += -lrt
endif

#
# Principal Targets
#
.PHONY: all icommands clients clean print_cflags print_ldflags
all:	icommands
	@true

clients: icommands
	@true

icommands:	$(OBJS) $(BINS)
	@true

$(objDir)/%.o:	$(srcDir)/%.cpp $(LIBRARY)
	@echo "Compile icommand `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o
	@echo "Link icommand `basename $@`..."
	$(V_at)$(LDR) -o $@ $< $(LDFLAGS)

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
	@echo "Cleaning icommands..."
	@rm -f $(BINS) $(OBJS)

