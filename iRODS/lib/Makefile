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
include $(buildDir)/../packaging/irods_externals_locations.mk

CFLAGS += `../../packaging/compiler_check.sh 4 2`

#
# Source files
#	core	utility functions
#	api 	client API functions (LIB_API_OBJS is set by api.mk)
#   hasher  Utility classes for generating hash digests
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
		$(libCoreObjDir)/osauth.o \
		$(libCoreObjDir)/sslSockComm.o \
		$(libCoreObjDir)/irods_serialization.o \
		$(libCoreObjDir)/irods_auth_factory.o \
		$(libCoreObjDir)/irods_string_tokenize.o \
		$(libCoreObjDir)/irods_error.o  \
		$(libCoreObjDir)/irods_log.o \
		$(libCoreObjDir)/irods_children_parser.o \
		$(libCoreObjDir)/irods_hierarchy_parser.o \
		$(libCoreObjDir)/irods_stacktrace.o \
		$(libCoreObjDir)/irods_signal.o \
		$(libCoreObjDir)/irods_plugin_name_generator.o \
		$(libCoreObjDir)/irods_client_negotiation.o \
		$(libCoreObjDir)/irods_network_object.o \
		$(libCoreObjDir)/irods_ssl_object.o \
		$(libCoreObjDir)/irods_tcp_object.o \
		$(libCoreObjDir)/irods_network_factory.o \
		$(libCoreObjDir)/irods_network_manager.o \
		$(libCoreObjDir)/irods_buffer_encryption.o \
		$(libCoreObjDir)/irods_auth_object.o \
		$(libCoreObjDir)/irods_gsi_object.o \
		$(libCoreObjDir)/irods_krb_object.o \
		$(libCoreObjDir)/irods_auth_manager.o \
		$(libCoreObjDir)/irods_virtual_path.o \
		$(libCoreObjDir)/irods_native_auth_object.o \
		$(libCoreObjDir)/irods_pam_auth_object.o \
		$(libCoreObjDir)/irods_osauth_auth_object.o \
		$(libCoreObjDir)/irods_pluggable_auth_scheme.o \
		$(libCoreObjDir)/irods_kvp_string_parser.o \
		$(libCoreObjDir)/irods_client_api_table.o \
		$(libCoreObjDir)/irods_pack_table.o \
		$(libCoreObjDir)/irods_get_full_path_for_config_file.o \
		$(libCoreObjDir)/irods_configuration_parser.o \
		$(libCoreObjDir)/irods_server_properties.o \
		$(libCoreObjDir)/irods_environment_properties.o \
		$(libCoreObjDir)/irods_parse_command_line_options.o \
		$(libCoreObjDir)/list.o \
		$(libCoreObjDir)/hashtable.o \
		$(libCoreObjDir)/region.o \
		$(libCoreObjDir)/irods_exception.o  \
		$(libCoreObjDir)/irods_random.o  \
		$(libCoreObjDir)/apiHandler.o

INCLUDES +=	-I$(libCoreIncDir)


ifdef RELEASE_FLAG
CFLAGS +=       -DRELEASE_FLAG
endif

# client_exec
LIB_CLIENT_EXEC_OBJS = \
		$(libClientExecObjDir)/irods_client_rule_execution_manager_factory.o
INCLUDES +=	-I$(libClientExecIncDir)


# Hasher
LIB_HASHER_OBJS = \
		$(libHasherObjDir)/Hasher.o \
		$(libHasherObjDir)/checksum.o \
		$(libHasherObjDir)/MD5Strategy.o \
		$(libHasherObjDir)/SHA256Strategy.o \
		$(libHasherObjDir)/irods_hasher_factory.o
INCLUDES +=	-I$(libHasherIncDir)

# rbudp
LIB_RBUDP_OBJS =  \
		$(libRbudpObjDir)/QUANTAnet_rbudpBase_c.o \
		$(libRbudpObjDir)/QUANTAnet_rbudpSender_c.o \
		$(libRbudpObjDir)/QUANTAnet_rbudpReceiver_c.o

# API (see api.mk, which is automatically generated)
include $(buildDir)/config/api.mk
INCLUDES +=	-I$(libApiIncDir)

# All objects
OBJS =  $(LIB_CORE_OBJS) \
		$(LIB_API_OBJS) \
		$(LIB_RBUDP_OBJS) \
		$(LIB_HASHER_OBJS)

BOOST_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/include 
JANSSON_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/include 

# Server configuration
CONFIG =	$(svrConfigDir)/irodsHost


# Compilation flags
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS = $(CFLAGS_OPTIONS) $(INCLUDES) $(SVR_INCLUDES) $(BOOST_INCLUDES) $(JANSSON_INCLUDES)

ifdef SYSLOG_FACILITY_CODE
CFLAGS_OPTIONS += -D SYSLOG_FACILITY_CODE=$(SYSLOG_FACILITY_CODE)
endif

#
# Principal Targets
#
.PHONY:	all lib libs clean print_cflags
vpath %c	$(libCoreSrcDir) $(libApiSrcDir) \
		$(libRbudpSrcDir) $(libHasherSrcDir)

# Build everything
all:	libs
	@true

# Build the library
lib:	libs
	@true

libs:	$(CONFIG) $(LIBRARY) $(LIB_CLIENT_EXEC_OBJS)
	@true

$(LIBRARY): $(OBJS)
#	@rm -f $(LIBRARY)
#	$(CC) -D${OS_platform} -fPIC "-Wl,-E" -shared -o $(LIBRARY) $(OBJS)
	@echo "Building lib$(LIBRARY_NAME).a"
	@rm -f $(LIBRARY)
	$(V_at)$(AR) $(AROPT) $(LIBRARY) $^
	$(V_at)$(AR) -s $(LIBRARY)

# Server configuration
$(svrConfigDir)/irodsHost:
	@$(MAKE) --no-print-directory -C .. config

# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"

# Clean
clean:
	@echo "Cleaning library..."
	@rm -f $(OBJS)
	@rm -f $(LIBRARY)

#
# Compilation Targets
#
# Build core
$(LIB_CORE_OBJS):  $(libCoreObjDir)/%.o: $(libCoreSrcDir)/%.cpp $(DEPEND)
	@echo "Compile core `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

# Build API
$(LIB_API_OBJS):  $(libApiObjDir)/%.o: $(libApiSrcDir)/%.cpp $(DEPEND)
	@echo "Compile api `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

# Build Hasher
$(LIB_HASHER_OBJS): $(libHasherObjDir)/%.o: $(libHasherSrcDir)/%.cpp $(configMk) $(platformMk)
	@echo "Compile Hasher `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<

$(LIB_CLIENT_EXEC_OBJS): $(libClientExecObjDir)/%.o: $(libClientExecSrcDir)/%.cpp $(configMk) $(platformMk)
	@echo "Compile client_exec `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<


$(LIB_RBUDP_OBJS):  $(libRbudpObjDir)/%.o: $(libRbudpSrcDir)/%.cpp $(configMk) $(platformMk)
	@echo "Compile rbudp `basename $@`..."
	$(V_at)$(CC) -c $(CFLAGS) -o $@ $<
