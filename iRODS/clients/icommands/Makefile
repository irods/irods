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

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

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

LDFLAGS +=	$(LIBRARY) $(MODULE_LDFLAGS) $(CL_LDADD) 


LDFLAGS += $(BOOST_LIBS)

# include jansson for zonereport
CFLAGS += -I../../../external/$(JANSSONVERSION)/src/
LDFLAGS += ../../../external/$(JANSSONVERSION)/src/.libs/libjansson.a

# include for control plane
CFLAGS += -I../../../external/$(ZEROMQVERSION)/include/
LDFLAGS += ../../../external/$(ZEROMQVERSION)/src/.libs/libzmq.a

CFLAGS += -I../../../external/$(AVROVERSION)
LDFLAGS += ../../../external/$(AVROVERSION)/build/libavrocpp_s.a

LDFLAGS += -lz

ifneq (${OS_platform}, osx_platform)
LDFLAGS += -lrt
endif

# for checking memleak
ifdef CCMALLOC
LDFLAGS += -L$(CCMALLOC_DIR)/lib
endif

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a \
             $(BOOST_DIR)/stage/lib/libboost_chrono.a \
             $(BOOST_DIR)/stage/lib/libboost_program_options.a
#
# Principal Targets
#
.PHONY: all icommands clients clean print_cflags print_ldflags
all:	icommands
	@true

clients: icommands
	@true

icommands:	print_cflags print_ldflags $(OBJS) $(BINS) $(BOOST_LIBS)
	@true

$(objDir)/%.o:	$(srcDir)/%.cpp $(LIBRARY) $(BOOST_LIBS)
	@echo "Compile icommand `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o $(BOOST_LIBS)
	@echo "Link icommand `basename $@`..."
	@$(LDR) -o $@ $< $(LDFLAGS)

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

