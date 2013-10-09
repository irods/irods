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
		$(objDir)/igroupadmin.o


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
		$(binDir)/igroupadmin 

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a 
#
# Compile and link flags
#
# Is this needed?
#ifndef ADDR_64BITS
#CC =		gcc
#endif

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

# GSI_SSL is set to ssl to use the system's SSL library, else use
# regular Globus version.
ifndef GSI_SSL
GSI_SSL = ssl_$(GSI_INSTALL_TYPE)
endif

# GSI_CRYPTO is set to crypto to use the system's Crypto library, else use
# regular Globus version.
ifndef GSI_CRYPTO
GSI_CRYPTO = crypto_$(GSI_INSTALL_TYPE)
endif

LIB_GSI_AUTH = \
        -L$(GLOBUS_LOCATION)/lib \
        -lglobus_gss_assist_$(GSI_INSTALL_TYPE) \
      -lglobus_gssapi_gsi_$(GSI_INSTALL_TYPE) \
        -lglobus_gsi_credential_$(GSI_INSTALL_TYPE) \
        -lglobus_gsi_proxy_core_$(GSI_INSTALL_TYPE) \
      -lglobus_gsi_callback_$(GSI_INSTALL_TYPE) \
      -lglobus_oldgaa_$(GSI_INSTALL_TYPE) \
      -lglobus_gsi_sysconfig_$(GSI_INSTALL_TYPE) \
      -lglobus_gsi_cert_utils_$(GSI_INSTALL_TYPE) \
      -lglobus_openssl_error_$(GSI_INSTALL_TYPE) \
      -lglobus_openssl_$(GSI_INSTALL_TYPE) \
      -lglobus_proxy_ssl_$(GSI_INSTALL_TYPE) \
      -l$(GSI_SSL) \
      -l$(GSI_CRYPTO) \
      -lglobus_common_$(GSI_INSTALL_TYPE) \
      -lglobus_callout_$(GSI_INSTALL_TYPE) \
      -lltdl_$(GSI_INSTALL_TYPE)
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


# JMC :: BOOST	Not needed MW
# LDFLAGS += -L/home/jasonc/dev/lib/boost/boost_1_45_0/stage/lib -lboost_thread


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

$(objDir)/%.o:	$(srcDir)/%.c $(LIBRARY) $(BOOST_LIBS)
	@echo "Compile icommand `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o $(BOOST_LIBS)
	@echo "Link icommand `basename $@`..."
	@$(LDR) -o $@ $< $(BOOST_LIBS) $(LDFLAGS) $(BOOST_LIBS)

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

