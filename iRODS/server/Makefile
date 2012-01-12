#
# server/Makefile
#
# Build the iRODS server, agent, and rule engine server.  This Makefile
# is intended to be invoked only through the top-level iRODS Makefile.
#
# Principal targets include:
#
# 	all		build the servers
#	server		build the servers
# 	clean		clean out all object files
#
# To see the gcc command lines as executed, change "@$(CC)" to "$(CC)".

ifndef buildDir
buildDir =	$(CURDIR)/..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

extendedICAT.h = 	$(buildDir)/modules/extendedICAT/extendedICAT.h

#
# Source files
# 	core	shared functions for all servers
# 	api	server API functions (SVR_API_OBJS is set by api.mk)
#	drivers	file system driver functions
#	icat	ICAT server
#	re	rule engine
#
# Core
SVR_CORE_OBJS =	\
		$(svrCoreObjDir)/dataObjOpr.o \
		$(svrCoreObjDir)/fileOpr.o \
		$(svrCoreObjDir)/initServer.o \
		$(svrCoreObjDir)/miscServerFunct.o \
		$(svrCoreObjDir)/objMetaOpr.o \
		$(svrCoreObjDir)/readServerConfig.o \
		$(svrCoreObjDir)/rsApiHandler.o \
		$(svrCoreObjDir)/rsIcatOpr.o \
		$(svrCoreObjDir)/rsLog.o \
		$(svrCoreObjDir)/rsRe.o	\
		$(svrCoreObjDir)/xmsgLib.o \
		$(svrCoreObjDir)/resource.o \
		$(svrCoreObjDir)/collection.o	\
		$(svrCoreObjDir)/objDesc.o	\
		$(svrCoreObjDir)/specColl.o	\
		$(svrCoreObjDir)/reServerLib.o	\
		$(svrCoreObjDir)/physPath.o \
		$(svrCoreObjDir)/fileDriverNoOpFunctions.o

INCLUDES +=	-I$(svrCoreIncDir)


# Servers
CFLAGS +=	-DRODS_SERVER
SERVER_OBJS =	\
		$(svrCoreObjDir)/rodsServer.o \
		$(svrCoreObjDir)/rodsAgent.o
SERVER_BINS =	$(serverBinDir)/irodsServer \
		$(serverBinDir)/irodsAgent


# API (see api.mk, which is automatically generated)
include $(buildDir)/config/api.mk
INCLUDES +=	-I$(svrApiIncDir)

# Drivers
SVR_DRIVERS_OBJS = \
		$(svrDriversObjDir)/structFileDriver.o \
		$(svrDriversObjDir)/fileDriver.o \
		$(svrDriversObjDir)/unixFileDriver.o \
		$(svrDriversObjDir)/msoFileDriver.o \
		$(svrDriversObjDir)/univMSSDriver.o

ifdef HAAW_STRUCT_FILE
SVR_DRIVERS_OBJS+=$(svrDriversObjDir)/haawStructFileDriver.o
endif

ifdef TAR_STRUCT_FILE
SVR_DRIVERS_OBJS+=$(svrDriversObjDir)/tarStructFileDriver.o
endif

ifdef AMAZON_S3
SVR_DRIVERS_OBJS+=$(svrDriversObjDir)/s3FileDriver.o
endif

ifdef DDN_WOS
SVR_DRIVERS_CPP_OBJS+=$(svrDriversObjDir)/wosFunctPP.o
SVR_DRIVERS_OBJS+=$(svrDriversObjDir)/wosFileDriver.o
CPPFLAGS = -g -O0 -c -D$(OS_platform) -I$(libCoreIncDir) -I$(svrDriversIncDir) -I$(WOS_DIR)/include -I$(BOOST_DIR)
endif

ifdef HPSS
SVR_DRIVERS_OBJS+=$(svrDriversObjDir)/hpssFileDriver.o
endif

INCLUDES +=	-I$(svrDriversIncDir)


# XXXXX add svrIcatIncDir for now. needed by non-icat server. mw
INCLUDES +=	-I$(svrIcatIncDir)

# icat
ifdef RODS_CAT
CFLAGS +=	-DRODS_CAT
SVR_ICAT_OBJS =	\
		$(svrIcatObjDir)/icatMidLevelHelpers.o \
		$(svrIcatObjDir)/icatMidLevelRoutines.o \
		$(svrIcatObjDir)/icatHighLevelRoutines.o \
		$(svrIcatObjDir)/icatGeneralQuery.o \
		$(svrIcatObjDir)/icatGeneralQuerySetup.o \
		$(svrIcatObjDir)/icatGeneralUpdate.o
INCLUDES +=	-I$(svrIcatIncDir)

SERVER_BINS +=  $(serverBinDir)/irodsXmsgServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsXmsgServer.o

ifdef NEW_ODBC
# Use the 'new' database handling
CFLAGS +=	-DNEW_ODBC
endif

# Add new RDA functions (either empty stubs or full functions)
SVR_ICAT_OBJS += $(svrIcatObjDir)/rdaHighLevelRoutines.o

ifdef PSQICAT
# Support Postgres
SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
INCLUDES +=	-I$(POSTGRES_HOME)/include
endif

ifdef ORAICAT
# Support Oracle
CFLAGS +=	-DORA_ICAT
SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOracle.o
INCLUDES +=	-I$(ORACLE_HOME)/rdbms/public
endif
endif

ifdef MYICAT
# Support MYSQL
SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
INCLUDES +=	-I$(UNIXODBC_HOME)/include
CFLAGS += -DUNIXODBC_DATASOURCE=\"$(UNIXODBC_DATASOURCE)\" -DMY_ICAT=1
endif

ifdef DBR
# Support DBR
# Postgres, Oracle, and MySQL
CFLAGS += -DDBR
ifeq ($(DBR_TYPE), oracle) 
ifndef ORAICAT
SVR_DBR_OBJS += $(svrIcatObjDir)/icatLowLevelOracle.o
endif
INCLUDES +=	-I$(ORACLE_HOME)/rdbms/public
#CFLAGS +=	-DORA_ICAT
CFLAGS +=	-DORA_DBR
endif
ifeq ($(DBR_TYPE), postgres) 
CFLAGS +=	-DNEW_ODBC
INCLUDES +=	-I$(svrIcatIncDir)
INCLUDES +=	-I$(POSTGRES_HOME)/include
ifndef PSQICAT
#INCLUDES +=	-I$(UNIXODBC_HOME)/include
SVR_DBR_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
endif
endif

ifeq ($(DBR_TYPE), mysql)
CFLAGS +=	-DNEW_ODBC
ifndef MYICAT
SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
endif
INCLUDES +=	-I$(UNIXODBC_HOME)/include
endif

SVR_DBR_OBJS += $(svrIcatObjDir)/dboHighLevelRoutines.o
endif  # end DBR

ifdef DBMS_LIB
LDFLAGS +=	$(DBMS_LIB)
endif

ifdef GSI_AUTH
CFLAGS +=       -DGSI_AUTH
endif
ifdef KRB_AUTH
CFLAGS +=       -DKRB_AUTH
endif
ifdef IRODS_SYSLOG
CFLAGS +=		-DIRODS_SYSLOG
endif
ifdef EXTENDED_ICAT
CFLAGS +=		-DEXTENDED_ICAT
INCLUDES +=	-I$(svrExtIcatIncDir)
endif

# re
ifdef RULE_ENGINE
ifndef RULE_ENGINE_N
CFLAGS +=       -DRULE_ENGINE

SVR_RE_OBJS =	\
		$(svrReObjDir)/extractAvuMS.o \
		$(svrReObjDir)/genQueryMS.o \
		$(svrReObjDir)/icatAdminMS.o \
		$(svrReObjDir)/icatGeneralMS.o \
		$(svrReObjDir)/keyValPairMS.o \
		$(svrReObjDir)/mailMS.o \
		$(svrReObjDir)/miscMS.o \
		$(svrReObjDir)/printMS.o \
		$(svrReObjDir)/reDataObjOpr.o \
		$(svrReObjDir)/msiHelper.o \
		$(svrReObjDir)/reHelpers1.o \
		$(svrReObjDir)/reHelpers2.o \
		$(svrReObjDir)/reIn2p3SysRule.o \
		$(svrReObjDir)/reLib1.o	\
		$(svrReObjDir)/reLib2.o \
		$(svrReObjDir)/reNaraMetaData.o \
		$(svrReObjDir)/reStruct.o \
		$(svrReObjDir)/reSysDataObjOpr.o \
		$(svrReObjDir)/reVariableMap.o \
		$(svrReObjDir)/regExpMatch.o \
		$(svrReObjDir)/ruleAdminMS.o \
		$(svrReObjDir)/systemMS.o \
		$(svrReObjDir)/xmsgMS.o \
		$(svrReObjDir)/reRDA.o \
		$(svrReObjDir)/reDBO.o \
		$(svrReObjDir)/testMS.o \
		$(svrReObjDir)/reAutoReplicateService.o \
		$(svrReObjDir)/sysBackupMS.o
INCLUDES +=	-I$(svrReIncDir)
SERVER_BINS +=	$(serverBinDir)/irodsReServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsReServer.o
endif
endif

ifdef RULE_ENGINE_N
CFLAGS +=       -DRULE_ENGINE -DRULE_ENGINE_N -DCACHE_ENABLE

ifeq ($(OS_platform), osx_platform)
# osx_platform does not support the POSIX realtime extension, but the shared memory functions are supported 
else
LDADD +=		-lrt
endif

SVR_RE_OBJS =	\
		$(svrReObjDir)/extractAvuMS.o \
		$(svrReObjDir)/genQueryMS.o \
		$(svrReObjDir)/icatAdminMS.o \
		$(svrReObjDir)/icatGeneralMS.o \
		$(svrReObjDir)/keyValPairMS.o \
		$(svrReObjDir)/mailMS.o \
		$(svrReObjDir)/miscMS.o \
		$(svrReObjDir)/printMS.o \
		$(svrReObjDir)/reDataObjOpr.o \
		$(svrReObjDir)/msiHelper.o \
		$(svrReObjDir)/nre.reHelpers1.o \
		$(svrReObjDir)/nre.reHelpers2.o \
		$(svrReObjDir)/arithmetics.o \
		$(svrReObjDir)/rules.o \
		$(svrReObjDir)/parser.o \
		$(svrReObjDir)/conversion.o \
		$(svrReObjDir)/index.o \
		$(svrReObjDir)/region.o \
		$(svrReObjDir)/datetime.o \
		$(svrReObjDir)/hashtable.o \
		$(svrReObjDir)/functions.o \
		$(svrReObjDir)/typing.o \
		$(svrReObjDir)/utils.o \
		$(svrReObjDir)/locks.o \
		$(svrReObjDir)/restructs.o \
		$(svrReObjDir)/sharedmemory.o \
		$(svrReObjDir)/filesystem.o \
		$(svrReObjDir)/cache.o \
		$(svrReObjDir)/reIn2p3SysRule.o \
		$(svrReObjDir)/nre.reLib1.o	\
		$(svrReObjDir)/nre.reLib2.o \
		$(svrReObjDir)/reNaraMetaData.o \
		$(svrReObjDir)/reStruct.o \
		$(svrReObjDir)/reSysDataObjOpr.o \
		$(svrReObjDir)/reVariableMap.o \
		$(svrReObjDir)/regExpMatch.o \
		$(svrReObjDir)/ruleAdminMS.o \
		$(svrReObjDir)/nre.systemMS.o \
		$(svrReObjDir)/xmsgMS.o \
		$(svrReObjDir)/reDBO.o \
		$(svrReObjDir)/reRDA.o \
		$(svrReObjDir)/testMS.o \
		$(svrReObjDir)/configuration.o \
		$(svrReObjDir)/reAutoReplicateService.o \
		$(svrReObjDir)/sysBackupMS.o

INCLUDES +=	-I$(svrReIncDir)
SERVER_BINS +=	$(serverBinDir)/irodsReServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsReServer.o
endif

# icat Tests
ifdef RODS_CAT
TEST_OBJS =	\
		$(svrTestObjDir)/test_chl.o \
		$(svrTestObjDir)/test_cll.o \
		$(svrTestObjDir)/test_genq.o \
		$(svrTestObjDir)/test_genu.o \
		$(svrTestObjDir)/test_rda.o

TEST_BINS =	\
		$(svrTestBinDir)/test_chl \
		$(svrTestBinDir)/test_cll \
		$(svrTestBinDir)/test_genq \
		$(svrTestBinDir)/test_genu \
		$(svrTestBinDir)/test_rda
endif

# reTest only works on Solaris
#TEST_OBJS +=	$(svrTestObjDir)/reTest.o
#TEST_BINS +=	$(svrTestBinDir)/reTest




# All objects (except tests)
OBJS =		$(SVR_CORE_OBJS) $(SVR_DRIVERS_OBJS) $(SVR_API_OBJS) \
		$(SVR_ICAT_OBJS) $(SVR_RE_OBJS) $(SVR_DBR_OBJS)
ifdef DDN_WOS
OBJS+=$(SVR_DRIVERS_CPP_OBJS)
endif

# Server configuration
CONFIG +=	$(svrConfigDir)/server.config $(svrConfigDir)/irodsHost


# Compilation and link flags
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(LIB_INCLUDES) $(MODULE_CFLAGS)

# Previously, MODULE_LDFLAGS was added into the LDFLAGS below, but now
# we use the new MODULE_OBJS for some dependencies so servers will be
# relinked when module object files are updated.  The MODULE_LDFLAGS
# is a list of the object files for modules plus the flags (-L -l
# options) and MODULE_OBJS is just the object files from the
# MODULE_LDFLAGS string.
LDFLAGS +=	$(LDADD) $(LIBRARY)


ifdef GSI_AUTH

# GSI_SSL is set to ssl to use the system's SSL library, else use
# regular Globus version.
ifndef GSI_SSL
GSI_SSL = ssl_$(GSI_INSTALL_TYPE)
endif

# GSI_CRYPTO is set to crypto to use the system's Crypto library, else
# use regular Globus version.
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

LDFLAGS += $(LIB_KRB_AUTH)
endif

#
# Princpal Targets
#
.PHONY:	all clean config server servers bin test tests print_cflags print_ldflags
vpath %cpp %c 	$(svrCoreSrcDir) $(svrDriversSrcDir) $(svrApiSrcDir) \
		$(svrIcatSrcDir) $(svrReSrcDir) 





# Build everything
all:	server
	@true





# Build the servers
servers: server
	@true

bin:	server
	@true

server:	config print_cflags print_ldflags $(OBJS) $(SERVER_BINS)
	@true





# Server configuration
config:	$(CONFIG)
	@true

$(svrConfigDir)/server.config: $(svrConfigDir)/server.config.in
	@echo "Warning, using default server.config file"
	@touch $(svrConfigDir)/server.config
	@mv $(svrConfigDir)/server.config $(svrConfigDir)/server.config.sav
	@cp $(svrConfigDir)/server.config.in $(svrConfigDir)/server.config
	@chmod 600 $(svrConfigDir)/server.config

$(svrConfigDir)/irodsHost: $(svrConfigDir)/irodsHost.in
	@echo "Note, making irodsHost file from irodsHost.in"
	@touch $(svrConfigDir)/irodsHost
	@mv $(svrConfigDir)/irodsHost $(svrConfigDir)/irodsHost.sav
	@cp $(svrConfigDir)/irodsHost.in $(svrConfigDir)/irodsHost
	@chmod 600 $(svrConfigDir)/irodsHost




# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"

# Show link flags
print_ldflags:
	@echo "Link flags:"
	@echo "    $(LDFLAGS)"





# Tests
tests:	test
	@true

retest:	test
	@true

test:	print_cflags print_ldflags $(TEST_OBJS) $(TEST_BINS)
	@true





# Clean
clean:
	@echo "Cleaning server..."
	@rm -f $(OBJS)
	@rm -f $(SERVER_OBJS) $(SERVER_BINS)
	@rm -f $(TEST_OBJS) $(TEST_BINS)
	@rm -f $(svrCoreObjDir)/ruleAdmin.o $(serverBinDir)/ruleAdmin


# Not needed
# LDFLAGS += /home/jasonc/dev/lib/boost/boost_1_45_0/stage/lib/libboost_thread.a

#
# Compilation Targets
#
# Build server API
$(SVR_API_OBJS):	$(svrApiObjDir)/%.o: $(svrApiSrcDir)/%.c $(DEPEND)
	@echo "Compile api `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server core
$(SVR_CORE_OBJS):	$(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.c $(DEPEND)
	@echo "Compile core `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(svrDriversObjDir)/wosFunctPP.o:	$(svrDriversSrcDir)/wosFunctPP.cpp
	g++ $(CPPFLAGS) -o $(svrDriversObjDir)/wosFunctPP.o $(svrDriversSrcDir)/wosFunctPP.cpp

# Build server drivers
$(SVR_DRIVERS_OBJS):	$(svrDriversObjDir)/%.o: $(svrDriversSrcDir)/%.c $(DEPEND)
	@echo "Compile drivers `basename $@`..."
	$(CC) -c $(CFLAGS) -o $@ $<

# Build server ICAT
$(SVR_ICAT_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.c $(extendedICAT.h) $(DEPEND)
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(extendedICAT.h): 
	@touch $(extendedICAT.h)

# Build server ICAT
$(SVR_DBR_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.c
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Builder server rule engine
$(SVR_RE_OBJS):		$(svrReObjDir)/%.o: $(svrReSrcDir)/%.c $(DEPEND)
	@echo "Compile re `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<





#
# Servers
#
# Agent
$(svrCoreObjDir)/rodsAgent.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.c $(LIBRARY) $(OBJS)
	@echo "Compile agent server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsAgent: $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS) $(MODULE_OBJS)
	@echo "Link agent server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(MODULE_LDFLAGS) $(OBJS) $(LDFLAGS) $(AG_LDADD)

# Rule engine
$(svrCoreObjDir)/irodsReServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.c $(LIBRARY) $(OBJS)
	@echo "Compile rule engine server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsReServer: $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS)
	@echo "Link rule engine server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(MODULE_LDFLAGS) $(OBJS) $(LDFLAGS)

# RODS
$(svrCoreObjDir)/rodsServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.c $(LIBRARY) $(OBJS)
	@echo "Compile rods server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsServer: $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS)
	@echo "Link rods server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(MODULE_LDFLAGS) $(LDFLAGS)


# xmsg server
$(svrCoreObjDir)/irodsXmsgServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.c $(LIBRARY) $(OBJS)
	@echo "Compile xmsg server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsXmsgServer: $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS)
	@echo "Link xmsg server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(MODULE_LDFLAGS) $(LDFLAGS)

#
# Tests
#
$(TEST_OBJS):	$(svrTestObjDir)/%.o: $(svrTestSrcDir)/%.c $(DEPEND)
	@echo "Compile server test `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# reTest
$(svrTestBinDir)/reTest: $(svrTestObjDir)/reTest.o $(OBJS)
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)

# cll and chl
$(svrTestBinDir)/test_cll: $(svrTestObjDir)/test_cll.o $(SVR_ICAT_OBJS)
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)

$(svrTestBinDir)/test_chl: $(svrTestObjDir)/test_chl.o $(SVR_ICAT_OBJS) $(svrCoreObjDir)/readServerConfig.o
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)

# genq and genu
$(svrTestBinDir)/test_genq: $(svrTestObjDir)/test_genq.o $(SVR_ICAT_OBJS) $(svrCoreObjDir)/readServerConfig.o
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)

$(svrTestBinDir)/test_genu: $(svrTestObjDir)/test_genu.o $(SVR_ICAT_OBJS) $(svrCoreObjDir)/readServerConfig.o
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)

$(svrTestBinDir)/test_rda: $(svrTestObjDir)/test_rda.o $(SVR_ICAT_OBJS)
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS)



#
# Rule Admin Targets
#
ruleadmin:: $(serverBinDir)/ruleAdmin 
	@cp $(serverBinDir)/ruleAdmin /misc/www/projects/srb-secure/cgi-bin/ruleAdminRaja.cgi

$(svrCoreObjDir)/ruleAdmin.o: $(svrCoreObjDir)/%.o: %.c $(LIBRARY) $(OBJS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/ruleAdmin: $(svrCoreObjDir)/ruleAdmin.o $(LIBRARY) $(OBJS)
	$(LDR) -o $@ $^ $(LDFLAGS)

