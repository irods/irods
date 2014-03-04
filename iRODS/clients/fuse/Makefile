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
		$(objDir)/iFuseOper.o \
		$(objDir)/iFuseLib.o \
		$(objDir)/iFuseLib.Conn.o \
		$(objDir)/iFuseLib.Desc.o \
		$(objDir)/iFuseLib.FileCache.o \
		$(objDir)/iFuseLib.Lock.o \
		$(objDir)/iFuseLib.PathCache.o \
		$(objDir)/iFuseLib.Utils.o 

SVR_OBJS = \
		$(reObjDir)/list.o \
		$(reObjDir)/hashtable.o \
		$(reObjDir)/region.o 

INCLUDES +=	-I$(incDir) -I$(reIncDir)





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
LDFLAGS +=	$(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib -lfuse -lrt
else
LDFLAGS +=	$(LDADD) $(MODULE_LDFLAGS) -L$(fuseHomeDir)/lib -lfuse 
endif

# for checking memleak
# LDFLAGS +=	-L/data/mwan/rods/ccmalloc/ccmalloc-0.4.0/lib

LDFLAGS +=	$(LIBRARY) -ldl

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a


ifdef GSI_AUTH

ifeq ($(OS_platform), osx_platform)
GSI_LIB_DIR=lib
else
ifeq ($(ADDR_64BITS), 1)
GSI_LIB_DIR=lib64
else
GSI_LIB_DIR=lib
endif
endif

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
      -L$(GLOBUS_LOCATION)/$(GSI_LIB_DIR) \
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
ifdef KRB_LOCATION
LIB_KRB_AUTH = -L$(KRB_LOCATION)/lib -lgssapi_krb5
else
LIB_KRB_AUTH = -lgssapi_krb5
endif
ifdef GSI_AUTH
LIB_KRB_AUTH += -ldl
endif
LDFLAGS += $(LIB_KRB_AUTH)
endif

ifdef USE_SSL
LDFLAGS        += -lssl -lcrypto
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

$(objDir)/%.o:	$(srcDir)/%.cpp $(LIBRARY)
	@echo "Compile fuse `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(reObjDir)/%.o:	$(reSrcDir)/%.c $(LIBRARY)
	@echo "Compile fuse `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(binDir)/%:	$(objDir)/%.o $(LIB_OBJS)
	@echo "Link fuse `basename $@`..."
	@$(LDR) -o $@ $< $(LIB_OBJS) $(SVR_OBJS) $(LDFLAGS) $(BOOST_LIBS) -lcrypto -lssl





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

