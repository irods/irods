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
# To see the gcc command lines as executed, use "make VERBOSE=1" or run
# build.sh with "--verbose".

include ../config/external_versions.txt

ifndef buildDir
buildDir =	$(CURDIR)/..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk
include $(buildDir)/../packaging/irods_externals_locations.mk

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
		$(svrCoreObjDir)/irods_resource_plugin.o \
		$(svrCoreObjDir)/irods_resource_plugin_impostor.o  \
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

ICAT_HLR_SRC = $(buildDir)/server/icat/src/icatHighLevelRoutines.cpp
ICAT_HLR_OBJ_WITH_NO_RE = $(svrCoreObjDir)/icatHighLevelRoutines_with_no_re.o



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

SVR_ICAT_OBJS =	\
		$(svrIcatObjDir)/icatHighLevelRoutines.o

SVR_ICAT_OBJS_NO_RE =	\
		$(svrIcatObjDir)/icatHighLevelRoutines_with_no_re.o

SVR_ICAT_OBJS_NO_RE_SRC =	\
		$(svrIcatSrcDir)/icatHighLevelRoutines.cpp
INCLUDES +=	-I$(svrIcatIncDir)

SERVER_BINS +=  $(serverBinDir)/irodsXmsgServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsXmsgServer.o

ifdef RELEASE_FLAG
CFLAGS += -DRELEASE_FLAG
endif

ifdef SYSLOG_FACILITY_CODE
CFLAGSS += -D SYSLOG_FACILITY_CODE=$(SYSLOG_FACILITY_CODE)
endif

# re
CFLAGS += -DCACHE_ENABLE

ifeq ($(OS_platform), osx_platform)
# osx_platform does not support the POSIX realtime extension, but the shared memory functions are supported
LDADD += -rdynamic -force_flat_namespace
else
LDADD += -lrt
endif

SVR_RE_OBJS =	\
		$(svrReObjDir)/irods_re_plugin.o \
		$(svrReObjDir)/irods_operation_rule_execution_manager.o \
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
		$(svrReObjDir)/irods_ms_plugin.o

INCLUDES +=	-I$(svrReIncDir)
SERVER_BINS +=	$(serverBinDir)/irodsReServer
SERVER_OBJS +=	$(svrCoreObjDir)/irodsReServer.o

# authentication support
SERVER_BINS += $(serverBinDir)/PamAuthCheck
SERVER_OBJS += $(svrAuthObjDir)/PamAuthCheck.o
AUTH_CFLAGS +=
AUTH_LDFLAGS += -lpam

# icat Tests
TEST_OBJS =	\
		$(svrTestObjDir)/test_chl.o

TEST_BINS =	\
		$(svrTestBinDir)/test_chl

# All objects (except tests)
OBJS =		$(SVR_RE_OBJS) $(SVR_CORE_OBJS) $(SVR_DRIVERS_OBJS) $(SVR_API_OBJS) \
		$(SVR_ICAT_OBJS) $(SVR_DBR_OBJS)


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

server:	config $(OBJS) $(SERVER_BINS)


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

test:	$(TEST_OBJS) $(TEST_BINS)



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



BOOST_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/include
BOOST_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/lib \
             -lboost_system \
             -lboost_filesystem \
             -lboost_regex \
             -lboost_thread \
             -lboost_chrono

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

db_plugin_with_re: $(DB_PLUGIN_SRC)
	@echo "Compile With RE `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE $(DB_PLUGIN_SRC) -o $(DB_PLUGIN_OBJ)

db_plugin_with_no_re: $(DB_PLUGIN_SRC) $(ICAT_HLR_SRC)
	@echo "Compile Without RE `basename $@`..."
	$(V_at)$(CC) -DLINK_NO_OP_RE_MGR $(CFLAGS) -c $(DB_PLUGIN_SRC) -o $(DB_PLUGIN_OBJ_WITH_NO_RE)
	$(V_at)$(CC) -DLINK_NO_OP_RE_MGR $(CFLAGS) -c $(ICAT_HLR_SRC) -o $(ICAT_HLR_OBJ_WITH_NO_RE)

# Compilation Targets
#
# Build server API
$(SVR_API_OBJS):	$(svrApiObjDir)/%.o: $(svrApiSrcDir)/%.cpp $(DEPEND)
	@echo "Compile api `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Build server core
$(SVR_CORE_OBJS):	$(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(DEPEND)
	@echo "Compile core `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Build server drivers
$(SVR_DRIVERS_OBJS):	$(svrDriversObjDir)/%.o: $(svrDriversSrcDir)/%.cpp $(DEPEND) 
	@echo "Compile drivers `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Build server ICAT
$(SVR_ICAT_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp $(DEPEND)
	@echo "Compile icat `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Build server ICAT
$(SVR_ICAT_OBJS_NO_RE):	$(SVR_ICAT_OBJS_NO_RE_SRC) $(DEPEND) $(BOOST_LIBS)
	@echo "Compile icat no re `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build server DBR
$(SVR_DBR_OBJS):	$(svrIcatObjDir)/%.o: $(svrIcatSrcDir)/%.cpp
	@echo "Compile dbr `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Builder server rule engine
$(SVR_RE_OBJS):		$(svrReObjDir)/%.o: $(svrReSrcDir)/%.cpp $(DEPEND)
	@echo "Compile re `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

# Servers
#
# Agent
$(svrCoreObjDir)/rodsAgent.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile agent server `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/irodsAgent: db_plugin_with_re $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS)
	@echo "Link agent server `basename $@`..."
	$(V_at)$(LDR) -o $@ $(svrCoreObjDir)/rodsAgent.o $(LIBRARY) $(OBJS)  $(DB_PLUGIN_OBJ) $(LDFLAGS)

# Rule engine
$(svrCoreObjDir)/irodsReServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile rule engine server `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/irodsReServer: db_plugin_with_re $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS)
	@echo "Link rule engine server `basename $@`..."
	$(V_at)$(LDR) -o $@ $(svrCoreObjDir)/irodsReServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(LDFLAGS)

# RODS
$(svrCoreObjDir)/rodsServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile rods server `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/irodsServer: db_plugin_with_re $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS)
	@echo "Link rods server `basename $@`..."
	$(V_at)$(LDR) -o $@ $(svrCoreObjDir)/rodsServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(LDFLAGS) 

# xmsg server
$(svrCoreObjDir)/irodsXmsgServer.o: $(svrCoreObjDir)/%.o: $(svrCoreSrcDir)/%.cpp $(LIBRARY) $(OBJS)
	@echo "Compile xmsg server `basename $@`..."
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/irodsXmsgServer: db_plugin_with_re $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS)
	@echo "Link xmsg server `basename $@`..."
	$(V_at)$(LDR) -o $@ $(svrCoreObjDir)/irodsXmsgServer.o $(LIBRARY) $(OBJS) $(DB_PLUGIN_OBJ) $(LDFLAGS)


#
# Authentication support
#
$(svrAuthObjDir)/PamAuthCheck.o: $(svrAuthSrcDir)/PamAuthCheck.cpp
	@echo "Compile PamAuthCheck utility `basename $@`..."
	@$(CC) -c $(AUTH_CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/PamAuthCheck: $(svrAuthObjDir)/PamAuthCheck.o
	@echo "Link PamAuthCheck utility `basename $@`..."
	$(V_at)$(CC) -o $@ $(svrAuthObjDir)/PamAuthCheck.o $(AUTH_LDFLAGS)

#
# Tests
#
$(TEST_OBJS):	$(svrTestObjDir)/%.o: $(svrTestSrcDir)/%.cpp $(DEPEND)
	@echo "Compile server test `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -DLINK_NO_OP_RE_MGR -o $@ $<

# reTest
$(svrTestBinDir)/reTest: $(svrTestObjDir)/reTest.o $(OBJS)
	@echo "Link server test `basename $@`..."
	$(V_at)$(LDR) -o $@ $^ $(LDFLAGS)

# chl
$(svrTestBinDir)/test_chl: db_plugin_with_no_re $(svrTestObjDir)/test_chl.o $(SVR_ICAT_OBJS) $(LIBRARY) $(DB_IFACE_OBJS)
	@echo "Link server test `basename $@`..."
	$(V_at)$(LDR) -o $@ $(svrTestObjDir)/test_chl.o $(LIBRARY) $(LIB_CLIENT_EXEC) $(ICAT_HLR_OBJ_WITH_NO_RE) $(DB_PLUGIN_OBJ_WITH_NO_RE) $(DB_IFACE_OBJS) $(LDFLAGS)

#
# Rule Admin Targets
#
ruleadmin:: $(serverBinDir)/ruleAdmin
	@cp $(serverBinDir)/ruleAdmin /misc/www/projects/srb-secure/cgi-bin/ruleAdminRaja.cgi

$(svrCoreObjDir)/ruleAdmin.o: $(svrCoreObjDir)/%.o: %.cpp $(LIBRARY) $(OBJS)
	@$(CC) -c $(CFLAGS) -DENABLE_RE -o $@ $<

$(serverBinDir)/ruleAdmin: $(svrCoreObjDir)/ruleAdmin.o $(LIBRARY) $(OBJS)
	$(V_at)$(LDR) -o $@ $^ $(LDFLAGS)
