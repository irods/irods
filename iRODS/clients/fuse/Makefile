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
fuseDir =	$(buildDir)/clients/fuse

objDir =	$(fuseDir)/obj
binDir =	$(fuseDir)/bin
srcDir =	$(fuseDir)/src
incDir =	$(fuseDir)/include





#
# Source files
#
OBJS =		\
		$(objDir)/irodsFs.o

BINS =		\
		$(binDir)/irodsFs \

LIB_OBJS =	\
		$(objDir)/iFuseOper.o \
		$(objDir)/iFuseLib.o

INCLUDES +=	-I$(incDir)





#
# Compile and link flags
#

CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)
ifdef IRODS_FS
CFLAGS_OPTIONS += -D_REENTRANT
INCLUDES +=	-I$(fuseHomeDir)/include
endif

CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(LIB_INCLUDES) $(SVR_INCLUDES) $(MODULE_CFLAGS)

ifeq ($(OS_platform), linux_platform)
LDFLAGS +=	$(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib -lfuse -ldl -lrt
else
LDFLAGS +=	$(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib -lfuse -ldl
endif

# for checking memleak
# LDFLAGS +=	-L/data/mwan/rods/ccmalloc/ccmalloc-0.4.0/lib

LDFLAGS +=	$(LIBRARY)


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
LIB_KRB_AUTH = -L$(KRB_LOC)/lib -lgssapi_krb5
ifdef GSI_AUTH
LIB_KRB_AUTH += -ldl
endif
LDFLAGS += $(LIB_KRB_AUTH)
endif


#
# Principal Targets
#
.PHONY: all fuse clients clean print_cflags print_ldflags
all:	fuse
	@true

clients: fuse
	@true

fuse:	print_cflags print_ldflags $(LIB_OBJS) $(OBJS) $(BINS)
	@true

$(objDir)/%.o:	$(srcDir)/%.c $(LIBRARY)
	@echo "Compile fuse `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o $(LIB_OBJS)
	@echo "Link fuse `basename $@`..."
	@$(LDR) -o $@ $< $(LIB_OBJS) $(LDFLAGS)





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

