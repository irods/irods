#
# lib/Makefile
#
# Build the iRODS API and utility library.
#
# The principal targets include:
#
# 	all		build the library objects and .a archive
#	lib		build the library objects and .a archive
# 	clean		clean out all object files
#

ifndef buildDir
buildDir =	$(CURDIR)/..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

extendedICAT.h =	$(buildDir)/modules/extendedICAT/extendedICAT.h



#
# Source files
#	core	utility functions
#	md5	MD5 hash generation functions
#	api	client API functions (LIB_API_OBJS is set by api.mk)
#
# Core
LIB_CORE_OBJS =	\
		$(libCoreObjDir)/base64.o \
		$(libCoreObjDir)/chksumUtil.o \
		$(libCoreObjDir)/clientLogin.o \
		$(libCoreObjDir)/cpUtil.o \
		$(libCoreObjDir)/getRodsEnv.o \
		$(libCoreObjDir)/getUtil.o \
		$(libCoreObjDir)/lsUtil.o \
		$(libCoreObjDir)/miscUtil.o \
		$(libCoreObjDir)/mkdirUtil.o \
		$(libCoreObjDir)/msParam.o \
		$(libCoreObjDir)/mvUtil.o \
		$(libCoreObjDir)/obf.o \
		$(libCoreObjDir)/packStruct.o \
		$(libCoreObjDir)/parseCommandLine.o \
		$(libCoreObjDir)/phymvUtil.o \
		$(libCoreObjDir)/procApiRequest.o \
		$(libCoreObjDir)/putUtil.o \
		$(libCoreObjDir)/rcConnect.o \
		$(libCoreObjDir)/rcMisc.o \
		$(libCoreObjDir)/rcPortalOpr.o \
		$(libCoreObjDir)/regUtil.o \
		$(libCoreObjDir)/replUtil.o \
		$(libCoreObjDir)/rmUtil.o \
		$(libCoreObjDir)/rmtrashUtil.o \
		$(libCoreObjDir)/rodsLog.o \
		$(libCoreObjDir)/rodsPath.o \
		$(libCoreObjDir)/rsyncUtil.o \
		$(libCoreObjDir)/sockComm.o \
		$(libCoreObjDir)/stringOpr.o \
		$(libCoreObjDir)/trimUtil.o	\
		$(libCoreObjDir)/mcollUtil.o	\
		$(libCoreObjDir)/bunUtil.o	\
		$(libCoreObjDir)/phybunUtil.o \
		$(libCoreObjDir)/scanUtil.o \
		$(libCoreObjDir)/fsckUtil.o \
		$(libCoreObjDir)/osauth.o

ifdef USE_BOOST_ASIO
LIB_CORE_OBJS += $(libCoreObjDir)/socket_wrapper.o 
endif

ifdef GSI_AUTH
LIB_CORE_OBJS += $(libCoreObjDir)/igsi.o
endif
ifdef KRB_AUTH
LIB_CORE_OBJS += $(libCoreObjDir)/ikrb.o
ifdef GSI_AUTH
# when using both KRB and GSI, include the KRB wrapper
LIB_CORE_OBJS += $(libCoreObjDir)/ikrbGSSAPIWrapper.o
endif
endif
INCLUDES +=	-I$(libCoreIncDir)

ifdef EXTENDED_ICAT
CFLAGS +=	-DEXTENDED_ICAT
INCLUDES +=	-I$(svrExtIcatIncDir)
endif

# Logging (automatically generated source)
LOG_SRC =	$(libCoreSrcDir)/rodsLog.c


# MD5
LIB_MD5_OBJS =	\
		$(libMd5ObjDir)/md5c.o \
		$(libMd5ObjDir)/md5Checksum.o
INCLUDES +=	-I$(libMd5IncDir)

# rbudp
ifdef RBUDP_TRANSFER
LIB_RBUDP_OBJS =  \
		$(libRbudpObjDir)/QUANTAnet_rbudpBase_c.o \
		$(libRbudpObjDir)/QUANTAnet_rbudpSender_c.o 	\
		$(libRbudpObjDir)/QUANTAnet_rbudpReceiver_c.o
endif

# GSI
ifdef GSI_AUTH
INCLUDES += 	-I$(GLOBUS_LOCATION)/include/$(GSI_INSTALL_TYPE)
endif

# Kerberos
ifdef KRB_AUTH
INCLUDES += 	-I$(KRB_LOC)/include/gssapi -I$(KRB_LOC)/include
endif

# API (see api.mk, which is automatically generated)
include $(buildDir)/config/api.mk
INCLUDES +=	-I$(libApiIncDir)


# All objects
OBJS =		$(LIB_CORE_OBJS) $(LIB_API_OBJS) $(LIB_MD5_OBJS)	\
		$(LIB_RBUDP_OBJS)


# Server configuration
CONFIG =	$(svrConfigDir)/server.config $(svrConfigDir)/irodsHost


# Compilation flags
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(SVR_INCLUDES)

MD5_CFLAGS_OPTIONS := $(CFLAGS_OPTIONS)
MD5_CFLAGS =	$(CFLAGS)
ifeq ($(CC_IS_GCC), 1)
MD5_CFLAGS_OPTIONS += -O3
MD5_CFLAGS += -O3
endif
ifdef GSI_AUTH
CFLAGS_OPTIONS += -D GSI_AUTH
endif
ifdef KRB_AUTH
CFLAGS_OPTIONS += -D KRB_AUTH
endif
ifdef IRODS_SYSLOG
CFLAGS_OPTIONS += -D IRODS_SYSLOG
endif


#
# Princpal Targets
#
.PHONY:	all lib libs clean print_cflags
vpath %c	$(libCoreSrcDir) $(libApiSrcDir) $(libMd5SrcDir) 	\
		$(libRbudpSrcDir)





# Build everything
all:	libs
	@true





# Build the library
lib:	libs
	@true

libs:	print_cflags $(CONFIG) $(LOG_SRC) $(LIBRARY)
	@true

$(LIBRARY): $(OBJS)
	@echo "Building lib$(LIBRARY_NAME).a"
	@rm -f $(LIBRARY)
	@$(AR) $(AROPT) $(LIBRARY) $^
	@$(RANLIB) $(LIBRARY)





# Server configuration
$(svrConfigDir)/irodsHost:
	@$(MAKE) --no-print-directory -C .. config

$(svrConfigDir)/server.config:
	@$(MAKE) --no-print-directory -C .. config





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"
	@echo "MD5-only compile flags:"
	@echo "    $(MD5_CFLAGS_OPTIONS)"





# Clean
clean:
	@echo "Cleaning library..."
	@rm -f $(OBJS)
	@rm -f $(LIBRARY)




#
# Compilation Targets
#
# Build core
$(LIB_CORE_OBJS):  $(libCoreObjDir)/%.o: $(libCoreSrcDir)/%.c $(extendedICAT.h) $(DEPEND)
	@echo "Compile core `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(extendedICAT.h): 
	@touch $(extendedICAT.h)

# Build API
$(LIB_API_OBJS):  $(libApiObjDir)/%.o: $(libApiSrcDir)/%.c $(DEPEND)
	@echo "Compile api `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build MD5
#	Requires -O3 for optimization
$(LIB_MD5_OBJS):  $(libMd5ObjDir)/%.o: $(libMd5SrcDir)/%.c $(configMk) $(platformMk)
	@echo "Compile md5 `basename $@`..."
	@$(CC) -c $(MD5_CFLAGS) -o $@ $<

ifdef RBUDP_TRANSFER
$(LIB_RBUDP_OBJS):  $(libRbudpObjDir)/%.o: $(libRbudpSrcDir)/%.c $(configMk) $(platformMk)
	@echo "Compile rbudp `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
endif

# Build logging source
rodslog:  $(libCoreSrcDir)/rodsLog.c
	@true

$(libCoreSrcDir)/rodsLog.c:  $(libCoreIncDir)/rodsErrorTable.h
	@$(PERL) $(perlScriptsDir)/updateRodsLog.pl
