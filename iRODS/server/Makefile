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

extendedICAT.hpp = 	$(buildDir)/modules/extendedICAT/extendedICAT.hpp

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
		$(svrCoreObjDir)/rsApiHandler.o \
		$(svrCoreObjDir)/rsIcatOpr.o \
		$(svrCoreObjDir)/rsLog.o \
		$(svrCoreObjDir)/rsRe.o	\
		$(svrCoreObjDir)/xmsgLib.o \
		$(svrCoreObjDir)/collection.o	\
		$(svrCoreObjDir)/objDesc.o	\
		$(svrCoreObjDir)/specColl.o	\
		$(svrCoreObjDir)/reServerLib.o	\
		$(svrCoreObjDir)/physPath.o \
		$(svrCoreObjDir)/irods_resource_manager.o \
		$(svrCoreObjDir)/irods_resource_backport.o \
		$(svrCoreObjDir)/irods_resource_redirect.o \
		$(svrCoreObjDir)/irods_server_negotiation.o \
		$(svrCoreObjDir)/irods_collection_object.o \
        $(svrCoreObjDir)/irods_data_object.o \
		$(svrCoreObjDir)/irods_structured_object.o \
		$(svrCoreObjDir)/irods_physical_object.o \
		$(svrCoreObjDir)/irods_file_object.o \
	    $(svrCoreObjDir)/irods_database_factory.o \
   		$(svrCoreObjDir)/irods_database_manager.o \
   		$(svrCoreObjDir)/irods_database_object.o  \
   		$(svrCoreObjDir)/irods_postgres_object.o  \
   		$(svrCoreObjDir)/irods_mysql_object.o  \
   		$(svrCoreObjDir)/irods_oracle_object.o  \
   		$(svrCoreObjDir)/irods_server_api_table.o  \
   		$(svrCoreObjDir)/irods_get_l1desc.o  \
        $(svrCoreObjDir)/readServerConfig.o

DB_IFACE_OBJS = \
	    $(svrCoreObjDir)/irods_database_factory.o \
   		$(svrCoreObjDir)/irods_database_manager.o \
   		$(svrCoreObjDir)/irods_database_object.o  \
   		$(svrCoreObjDir)/irods_postgres_object.o  \
   		$(svrCoreObjDir)/irods_mysql_object.o  \
   		$(svrCoreObjDir)/irods_oracle_object.o  \
        $(svrCoreObjDir)/readServerConfig.o

DB_PLUGIN_OBJ = $(svrCoreObjDir)/irods_database_plugin.o 
DB_PLUGIN_SRC = $(buildDir)/server/core/src/irods_database_plugin.cpp
DB_PLUGIN_OBJ_WITH_NO_RE = $(svrCoreObjDir)/irods_database_plugin_with_no_re.o 
   		

INCLUDES +=	-I$(svrCoreIncDir) -I$(libHasherIncDir)

CFLAGS += `../../packaging/compiler_check.sh 4 2`

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
		$(svrDriversObjDir)/fileDriver.o

INCLUDES +=	-I$(svrDriversIncDir)


# XXXXX add svrIcatIncDir for now. needed by non-icat server. mw
INCLUDES +=	-I$(svrIcatIncDir)

# icat
ifdef RODS_CAT
CFLAGS +=	-DRODS_CAT
SVR_ICAT_OBJS =	\
		$(svrIcatObjDir)/icatHighLevelRoutines.o 

INCLUDES +=	-I$(svrIcatIncDir)

SERVER_BINS +=  $(serverBinDir)/irodsXmsgServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsXmsgServer.o

ifdef PSQICAT
# Support Postgres
#SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
INCLUDES +=	-I$(POSTGRES_HOME)/include
endif

ifdef ORAICAT
# Support Oracle
CFLAGS +=	-DORA_ICAT
#SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOracle.o
INCLUDES +=	-I$(ORACLE_HOME)/rdbms/public
endif
endif

ifdef MYICAT
# Support MYSQL
#SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
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
INCLUDES +=	-I$(svrIcatIncDir)
INCLUDES +=	-I$(POSTGRES_HOME)/include
ifndef PSQICAT
#INCLUDES +=	-I$(UNIXODBC_HOME)/include
SVR_DBR_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
endif
endif

ifeq ($(DBR_TYPE), mysql)
ifndef MYICAT
SVR_ICAT_OBJS += $(svrIcatObjDir)/icatLowLevelOdbc.o
endif
INCLUDES +=	-I$(UNIXODBC_HOME)/include
endif

#SVR_DBR_OBJS += $(svrIcatObjDir)/dboHighLevelRoutines.o
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

ifdef SYSLOG_FACILITY_CODE
CFLAGSS +=     -D SYSLOG_FACILITY_CODE=$(SYSLOG_FACILITY_CODE)
endif

# re
CFLAGS +=       -DCACHE_ENABLE

ifeq ($(OS_platform), osx_platform)
# osx_platform does not support the POSIX realtime extension, but the shared memory functions are supported 
LDADD += -rdynamic -force_flat_namespace
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
		$(svrReObjDir)/datetime.o \
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
		$(svrReObjDir)/reNaraMetaData.o \
		$(svrReObjDir)/reStruct.o \
		$(svrReObjDir)/reSysDataObjOpr.o \
		$(svrReObjDir)/reVariableMap.o \
		$(svrReObjDir)/reVariableMap.gen.o \
		$(svrReObjDir)/nre.ruleAdminMS.o \
		$(svrReObjDir)/nre.systemMS.o \
		$(svrReObjDir)/xmsgMS.o \
		$(svrReObjDir)/testMS.o \
		$(svrReObjDir)/configuration.o \
		$(svrReObjDir)/reAutoReplicateService.o \
		$(svrReObjDir)/sysBackupMS.o \
		$(svrReObjDir)/irods_ms_plugin.o \
		$(svrReObjDir)/irods_operation_rule_execution_manager.o

INCLUDES +=	-I$(svrReIncDir)
SERVER_BINS +=	$(serverBinDir)/irodsReServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsReServer.o

# authentication support
ifdef PAM_AUTH
SERVER_BINS += $(serverBinDir)/PamAuthCheck
SERVER_OBJS += $(svrAuthObjDir)/PamAuthCheck.o
AUTH_CFLAGS +=
AUTH_LDFLAGS += -lpam
endif


# icat Tests
ifdef RODS_CAT
TEST_OBJS =	\
		$(svrTestObjDir)/test_chl.o 

TEST_BINS =	\
		$(svrTestBinDir)/test_chl 
endif

# reTest only works on Solaris
#TEST_OBJS +=	$(svrTestObjDir)/reTest.o
#TEST_BINS +=	$(svrTestBinDir)/reTest




# All objects (except tests)
OBJS =		$(SVR_CORE_OBJS) $(SVR_DRIVERS_OBJS) $(SVR_API_OBJS) \
		$(SVR_ICAT_OBJS) $(SVR_RE_OBJS) $(SVR_DBR_OBJS)

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
LDFLAGS +=	$(LIBRARY) $(LDADD) 

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

# GSI_CRYPTO is set to crypto to use the system's Crypto library, else
# use regular Globus version.
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
#	@true





# Build the servers
servers: server
#	@true

bin:	server
#	@true

server:	config print_cflags print_ldflags $(OBJS) $(SERVER_BINS)
#	@true





# Server configuration
config:	$(CONFIG)
#	@true

$(svrConfigDir)/server.config: $(svrConfigDir)/server.config.in
	@echo "Warning, using default server.config file"
	@cp $(svrConfigDir)/server.config.in $(svrConfigDir)/server.config
	@chmod 600 $(svrConfigDir)/server.config

$(svrConfigDir)/irodsHost: $(svrConfigDir)/irodsHost.in
	@echo "Note, making irodsHost file from irodsHost.in"
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
#	@true

retest:	test
#	@true

test:	print_cflags print_ldflags $(TEST_OBJS) $(TEST_BINS)
#	@true





# Clean
clean:
	@echo "Cleaning server..."
	@rm -f $(OBJS)
	@rm -f $(SERVER_OBJS) $(SERVER_BINS) $(DB_PLUGIN_OBJ)
	@rm -f $(TEST_OBJS) $(TEST_BINS)
	@rm -f $(svrCoreObjDir)/ruleAdmin.o $(serverBinDir)/ruleAdmin
	@rm -f $(DB_PLUGIN_OBJ_WITH_NO_RE)



BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a 

db_plugin_with_re: $(DB_PLUGIN_SRC)
	@echo "Compile With RE `basename $@`..."
	@$(CC) -c $(CFLAGS) $(DB_PLUGIN_SRC) -o $(DB_PLUGIN_OBJ)

db_plugin_with_no_re: $(DB_PLUGIN_SRC)
	@echo "Compile Without RE `basename $@`..."
	@$(CC) -DLINK_NO_OP_RE_MGR $(CFLAGS) -c $(DB_PLUGIN_SRC) -o $(DB_PLUGIN_OBJ_WITH_NO_RE)

# Compilation Targets
#
# Build server API
$(SVR_API_OBJS):	$(svrApiObjDir)/%.o: $(svrApiSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile api `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server core
$(SVR_CORE_OBJS):	$(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile core `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server drivers
$(SVR_DRIVERS_OBJS):	$(svrDriversObjDir)/%.o: $(svrDriversSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile drivers `basename $@`..."
	$(CC) -c $(CFLAGS) -o $@ $<

# Build server ICAT
$(SVR_ICAT_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp $(extendedICAT.hpp) $(DEPEND) $(BOOST_LIBS)
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(extendedICAT.hpp): 
	@touch $(extendedICAT.hpp)

# Build server ICAT
$(SVR_DBR_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp $(BOOST_LIBS)
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Builder server rule engine
$(SVR_RE_OBJS):		$(svrReObjDir)/%.o: $(svrReSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile re `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Servers
#
# Agent
$(svrCoreObjDir)/rodsAgent.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Compile agent server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsAgent: db_plugin_with_re $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS) $(MODULE_OBJS) $(BOOST_LIBS)
	@echo "Link agent server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(MODULE_LDFLAGS) $(OBJS)  $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS) $(AG_LDADD)

# Rule engine
$(svrCoreObjDir)/irodsReServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile rule engine server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsReServer: db_plugin_with_re $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS) $(BOOST_LIBS) 
	@echo "Link rule engine server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(MODULE_LDFLAGS) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS)

# RODS
$(svrCoreObjDir)/rodsServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Compile rods server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsServer: db_plugin_with_re $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS) $(BOOST_LIBS)
	@echo "Link rods server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(MODULE_LDFLAGS) $(LDFLAGS)

# xmsg server
$(svrCoreObjDir)/irodsXmsgServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile xmsg server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsXmsgServer: db_plugin_with_re $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(MODULE_OBJS) $(BOOST_LIBS)
	@echo "Link xmsg server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(MODULE_LDFLAGS) $(LDFLAGS)


#
# Authentication support
#
$(svrAuthObjDir)/PamAuthCheck.o: $(svrAuthSrcDir)/PamAuthCheck.cpp
	@echo "Compile PamAuthCheck utility `basename $@`..."
	@$(CC) -c $(AUTH_CFLAGS) -o $@ $<

$(serverBinDir)/PamAuthCheck: $(svrAuthObjDir)/PamAuthCheck.o
	@echo "Link PamAuthCheck utility `basename $@`..."
	@$(CC) -o $@ $(svrAuthObjDir)/PamAuthCheck.o $(AUTH_LDFLAGS)

#
# Tests
#
$(TEST_OBJS):	$(svrTestObjDir)/%.o: $(svrTestSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS) 
	@echo "Compile server test `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# reTest
$(svrTestBinDir)/reTest: $(svrTestObjDir)/reTest.o $(OBJS) $(BOOST_LIBS) $(BOOST_LIBS)
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $^ $(LDFLAGS) $(BOOST_LIBS) 

# chl
$(svrTestBinDir)/test_chl: db_plugin_with_no_re $(svrTestObjDir)/test_chl.o $(SVR_ICAT_OBJS) $(LIBRARY) $(BOOST_LIBS) $(DB_IFACE_OBJS)
	@echo "Link server test `basename $@`..."
	@$(LDR) -o $@ $(svrTestObjDir)/test_chl.o $(LIBRARY) $(SVR_ICAT_OBJS) $(DB_PLUGIN_OBJ_WITH_NO_RE) $(DB_IFACE_OBJS) $(BOOST_LIBS) $(LDFLAGS)

#
# Rule Admin Targets
#
ruleadmin:: $(serverBinDir)/ruleAdmin 
	@cp $(serverBinDir)/ruleAdmin /misc/www/projects/srb-secure/cgi-bin/ruleAdminRaja.cgi

$(svrCoreObjDir)/ruleAdmin.o: $(svrCoreObjDir)/%.o: %.cpp $(LIBRARY) $(OBJS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/ruleAdmin: $(svrCoreObjDir)/ruleAdmin.o $(LIBRARY) $(OBJS)
	$(LDR) -o $@ $^ $(LDFLAGS)

