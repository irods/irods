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
		$(objDir)/iapitest.o


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
		$(binDir)/iapitest 

ifdef NETCDF_CLIENT
OBJS += $(objDir)/incattr.o $(objDir)/inc.o $(objDir)/incarch.o
BINS += $(binDir)/incattr $(binDir)/inc $(binDir)/incarch
endif

# Compile and link flags

CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)
ifdef GSI_AUTH
CFLAGS_OPTIONS += -D GSI_AUTH
endif
ifdef KRB_AUTH
CFLAGS_OPTIONS += -D KRB_AUTH
endif

CFLAGS =	$(CFLAGS_OPTIONS) $(LIB_INCLUDES) $(SVR_INCLUDES) $(MODULE_CFLAGS)

LDFLAGS +=	$(LIBRARY) $(MODULE_LDFLAGS) $(CL_LDADD) 


ifdef GSI_AUTH

ifeq ($(GSI_INSTALL_TYPE),)
GSI_INSTALL_TYPE2=
else
GSI_INSTALL_TYPE2=_$(GSI_INSTALL_TYPE)
endif

# GSI_SSL is set to ssl to use the system's SSL library, else use
# regular Globus version.
ifndef GSI_SSL
GSI_SSL = ssl$(GSI_INSTALL_TYPE2)
endif

# GSI_CRYPTO is set to crypto to use the system's Crypto library, else use
# regular Globus version.
ifndef GSI_CRYPTO
GSI_CRYPTO = crypto$(GSI_INSTALL_TYPE2)
endif

LIB_GSI_AUTH = \
      -L$(GLOBUS_LOCATION)/lib64 \
      -L$(GLOBUS_LOCATION)/lib \
      -lglobus_gss_assist$(GSI_INSTALL_TYPE2) \
      -lglobus_gssapi_gsi$(GSI_INSTALL_TYPE2) \
      -lglobus_gsi_credential$(GSI_INSTALL_TYPE2) \
      -lglobus_gsi_proxy_core$(GSI_INSTALL_TYPE2) \
      -lglobus_gsi_callback$(GSI_INSTALL_TYPE2) \
      -lglobus_oldgaa$(GSI_INSTALL_TYPE2) \
      -lglobus_gsi_sysconfig$(GSI_INSTALL_TYPE2) \
      -lglobus_gsi_cert_utils$(GSI_INSTALL_TYPE2) \
      -lglobus_openssl_error$(GSI_INSTALL_TYPE2) \
      -lglobus_openssl$(GSI_INSTALL_TYPE2) \
      -lglobus_proxy_ssl$(GSI_INSTALL_TYPE2) \
      -l$(GSI_SSL) \
      -l$(GSI_CRYPTO) \
      -lglobus_common$(GSI_INSTALL_TYPE2) \
      -lglobus_callout$(GSI_INSTALL_TYPE2) \
      -lltdl$(GSI_INSTALL_TYPE2)
LDFLAGS += $(LIB_GSI_AUTH)
endif

ifdef KRB_AUTH
LIB_KRB_AUTH = \
        -L$(KRB_LOC)/lib \
        -lgssapi_krb5
ifdef GSI_AUTH
LIB_KRB_AUTH += -ldl
endif
LDFLAGS += $(LIB_KRB_AUTH)
endif

# for checking memleak
ifdef CCMALLOC
LDFLAGS += -L$(CCMALLOC_DIR)/lib
endif

LDFLAGS += -lrt
BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a
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
	@$(LDR) -o $@ $< $(LDFLAGS) $(BOOST_LIBS) -lrt

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

