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

include ../config/external_versions.txt

ifndef buildDir
buildDir =	$(CURDIR)/..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

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
		$(svrCoreObjDir)/rodsConnect.o \
		$(svrCoreObjDir)/procLog.o \
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
		$(svrCoreObjDir)/irods_resource_plugin_impostor.o  \
		$(svrCoreObjDir)/readServerConfig.o \
		$(svrCoreObjDir)/irods_server_control_plane.o \
		$(svrCoreObjDir)/irods_server_state.o

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
INCLUDES +=	-I$(POSTGRES_HOME)/include
endif

ifdef ORAICAT
CFLAGS +=	-DORA_ICAT
INCLUDES +=	-I$(ORACLE_HOME)/rdbms/public
endif
endif

ifdef MYICAT
INCLUDES +=	-I$(UNIXODBC_HOME)/include
CFLAGS += -DUNIXODBC_DATASOURCE=\"$(UNIXODBC_DATASOURCE)\" -DMY_ICAT=1
endif

ifdef DBMS_LIB
LDFLAGS +=	$(DBMS_LIB)
endif

ifdef RELEASE_FLAG
CFLAGS +=       -DRELEASE_FLAG
endif
ifdef IRODS_SYSLOG
CFLAGS +=		-DIRODS_SYSLOG
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
		$(svrReObjDir)/irods_ms_plugin.o \
		$(svrReObjDir)/irods_operation_rule_execution_manager.o

INCLUDES +=	-I$(svrReIncDir)
SERVER_BINS +=	$(serverBinDir)/irodsReServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsReServer.o

# authentication support
SERVER_BINS += $(serverBinDir)/PamAuthCheck
SERVER_OBJS += $(svrAuthObjDir)/PamAuthCheck.o
AUTH_CFLAGS +=
AUTH_LDFLAGS += -lpam

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


# Compilation and link flags
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(LIB_INCLUDES)

LDFLAGS +=	$(LIBRARY) $(LDADD)

#
# Princpal Targets
#
.PHONY:	all clean config server servers bin test tests print_cflags print_ldflags
vpath %cpp %c 	$(svrCoreSrcDir) $(svrDriversSrcDir) $(svrApiSrcDir) \
		$(svrIcatSrcDir) $(svrReSrcDir)





# Build everything
all:	server


# Build the servers
servers: server

bin:	server

server:	config print_cflags print_ldflags $(OBJS) $(SERVER_BINS)


# Server configuration
config:	$(CONFIG)


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

retest:	test

test:	print_cflags print_ldflags $(TEST_OBJS) $(TEST_BINS)



# temp include jansson for zonereport
CFLAGS += -I../../external/$(JANSSONVERSION)/src/
AG_LDADD += ../../external/$(JANSSONVERSION)/src/.libs/libjansson.a
LDFLAGS += ../../external/$(JANSSONVERSION)/src/.libs/libjansson.a

# temp include for control plane
INCLUDES +=	-I$(buildDir)/../external/$(ZEROMQVERSION)/include/
LDFLAGS += $(buildDir)/../external/$(ZEROMQVERSION)/src/.libs/libzmq.a

INCLUDES +=	-I$(buildDir)/../external/$(AVROVERSION)
LDFLAGS += $(buildDir)/../external/$(AVROVERSION)/build/libavrocpp_s.a -lz

INCLUDES += -I/usr/include/proc

FILE_TO_CHECK=/usr/lib/x86_64-linux-gnu/libprocps.so
ifneq ("$(wildcard $(FILE_TO_CHECK))","")
LDFLAGS += -lprocps
endif

FILE_TO_CHECK=/usr/lib/libproc.so
ifneq ("$(wildcard $(FILE_TO_CHECK))","")
LDFLAGS += -lproc
endif

FILE_TO_CHECK=/lib64/libprocps.so
ifneq ("$(wildcard $(FILE_TO_CHECK))","")
LDFLAGS += -lprocps
endif

FILE_TO_CHECK=/lib64/libproc.so
ifneq ("$(wildcard $(FILE_TO_CHECK))","")
LDFLAGS += -lproc
endif


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
             $(BOOST_DIR)/stage/lib/libboost_thread.a \
             $(BOOST_DIR)/stage/lib/libboost_chrono.a

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
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server ICAT
$(SVR_ICAT_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server DBR
$(SVR_DBR_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp $(BOOST_LIBS)
	@echo "Compile dbr `basename $@`..."
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

$(serverBinDir)/irodsAgent: db_plugin_with_re $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Link agent server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS)  $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS) $(AG_LDADD)

# Rule engine
$(svrCoreObjDir)/irodsReServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile rule engine server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsReServer: db_plugin_with_re $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Link rule engine server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS)

# RODS
$(svrCoreObjDir)/rodsServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Compile rods server `basename $@`..."
	$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsServer: db_plugin_with_re $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Link rods server `basename $@`..."
	@$(LDR) -o $@ $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS)

# xmsg server
$(svrCoreObjDir)/irodsXmsgServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile xmsg server `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/irodsXmsgServer: db_plugin_with_re $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(BOOST_LIBS)
	@echo "Link xmsg server `basename $@`..."
	@echo "LDFLAGS $(LDFLAGS) $@"
	@$(LDR) -o $@ $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(BOOST_LIBS) $(LDFLAGS)


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
	@$(CC) -c $(CFLAGS) -o $@ $<

$(serverBinDir)/ruleAdmin: $(svrCoreObjDir)/ruleAdmin.o $(LIBRARY) $(OBJS)
	@$(LDR) -o $@ $^ $(LDFLAGS)
