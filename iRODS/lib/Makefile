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

CFLAGS += `../../packaging/compiler_check.sh 4 2`

# jansson support
CFLAGS += -I../../external/jansson-2.7/src/
AG_LDADD += ../../external/jansson-2.7/src/.libs/libjansson.a
LDFLAGS += ../../external/jansson-2.7/src/.libs/libjansson.a

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
		$(libCoreObjDir)/irods_operation_wrapper.o \
		$(libCoreObjDir)/irods_string_tokenize.o \
		$(libCoreObjDir)/irods_plugin_base.o \
		$(libCoreObjDir)/irods_resource_plugin.o \
		$(libCoreObjDir)/irods_error.o  \
		$(libCoreObjDir)/irods_log.o \
		$(libCoreObjDir)/irods_children_parser.o \
		$(libCoreObjDir)/irods_hierarchy_parser.o \
		$(libCoreObjDir)/irods_stacktrace.o \
		$(libCoreObjDir)/irods_signal.o \
		$(libCoreObjDir)/irods_plugin_name_generator.o \
		$(libCoreObjDir)/irods_client_negotiation.o \
		$(libCoreObjDir)/irods_network_object.o \
		$(libCoreObjDir)/irods_network_plugin.o \
		$(libCoreObjDir)/irods_ssl_object.o \
		$(libCoreObjDir)/irods_tcp_object.o \
		$(libCoreObjDir)/irods_network_factory.o \
		$(libCoreObjDir)/irods_network_manager.o \
		$(libCoreObjDir)/irods_buffer_encryption.o \
		$(libCoreObjDir)/irods_auth_object.o \
		$(libCoreObjDir)/irods_gsi_object.o \
		$(libCoreObjDir)/irods_krb_object.o \
		$(libCoreObjDir)/irods_auth_plugin.o \
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
		$(libCoreObjDir)/apiHandler.o

INCLUDES +=	-I$(libCoreIncDir)


ifdef RELEASE_FLAG
CFLAGS +=       -DRELEASE_FLAG
endif

ifdef SHA256_FILE_HASH
CFLAGS += -DSHA256_FILE_HASH
endif

# Logging (automatically generated source)
LOG_SRC =	$(libCoreSrcDir)/rodsLog.cpp

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

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a \
             $(BOOST_DIR)/stage/lib/libboost_chrono.a

# Server configuration
CONFIG =	$(svrConfigDir)/irodsHost


# Compilation flags
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(SVR_INCLUDES)

ifdef IRODS_SYSLOG
CFLAGS_OPTIONS += -D IRODS_SYSLOG
endif
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

libs:	$(CONFIG) $(LOG_SRC) $(LIBRARY)
	@true

$(LIBRARY): $(OBJS) $(BOOST_LIBS)
#	@rm -f $(LIBRARY)
#	$(CC) -D${OS_platform} -fPIC "-Wl,-E" -shared -o $(LIBRARY) $(OBJS) 
	@echo "Building lib$(LIBRARY_NAME).a"
	@rm -f $(LIBRARY)
	@$(AR) $(AROPT) $(LIBRARY) $^
	@$(RANLIB) $(LIBRARY)

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
	@rm -f $(LOG_SRC)

#
# Compilation Targets
#
# Build core
$(LIB_CORE_OBJS):  $(libCoreObjDir)/%.o: $(libCoreSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile core `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build API
$(LIB_API_OBJS):  $(libApiObjDir)/%.o: $(libApiSrcDir)/%.cpp $(DEPEND) $(BOOST_LIBS)
	@echo "Compile api `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build Hasher
$(LIB_HASHER_OBJS): $(libHasherObjDir)/%.o: $(libHasherSrcDir)/%.cpp $(configMk) $(platformMk)
	@echo "Compile Hasher `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(LIB_RBUDP_OBJS):  $(libRbudpObjDir)/%.o: $(libRbudpSrcDir)/%.cpp $(configMk) $(platformMk)
	@echo "Compile rbudp `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

# Build logging source
rodslog:  $(LOG_SRC)
	@true

$(LOG_SRC):  $(libCoreIncDir)/rodsErrorTable.hpp
	@$(PERL) $(perlScriptsDir)/updateRodsLog.pl
