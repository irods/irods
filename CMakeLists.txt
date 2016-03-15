cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'")
endif()

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
  set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
  message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'")
endif()

macro(IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH DEPENDENCY_NAME DEPENDENCY_SUBDIRECTORY)
  if (NOT IS_DIRECTORY ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
    message(FATAL_ERROR "${DEPENDENCY_NAME} not found at ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY}")
  endif()
  set(IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME} ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
endmacro()

IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(CLANG clang3.7-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(AVRO avro1.7.7-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(BOOST boost1.60.0-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(JANSSON jansson2.7-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(ZMQ zeromq4-14.1.3-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(CPPZMQ cppzmq4.1-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(ARCHIVE libarchive3.1.2-0)
IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH(EPM epm4.2-0)

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)

project(irods C CXX)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_PREFIX /usr)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
if (NOT PAM_LIBRARY)
  find_library(PAM_LIBRARY pam)
  if (PAM_LIBRARY)
    message(STATUS "Found pam: ${PAM_LIBRARY}")
  else()
    message(FATAL_ERROR "pam library not found")
  endif()
endif()

set(IRODS_VERSION_MAJOR "4")
set(IRODS_VERSION_MINOR "2")
set(IRODS_VERSION_PATCH "0")
set(IRODS_VERSION "${IRODS_VERSION_MAJOR}.${IRODS_VERSION_MINOR}.${IRODS_VERSION_PATCH}")

set(IRODS_PACKAGE_COMPONENT_SERVER_NAME IRODS_COMPONENT_SERVER)
set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME IRODS_COMPONENT_DEVELOPMENT)
set(IRODS_PACKAGE_COMPONENT_RUNTIME_NAME IRODS_COMPONENT_RUNTIME)
set(IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME IRODS_COMPONENT_ICOMMANDS)
set(IRODS_PACKAGE_COMPONENT_POSTGRES_NAME IRODS_COMPONENT_POSTGRES)
set(IRODS_PACKAGE_COMPONENT_MYSQL_NAME IRODS_COMPONENT_MYSQL)

set(IRODS_CXX_STANDARD 11)

set(IRODS_PLATFORM_STRING linux_platform)
set(IRODS_COMPILE_DEFINITIONS ${IRODS_PLATFORM_STRING} _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64 _LARGE_FILES _LARGEFILE64_SOURCE)

if (NOT IRODS_LINUX_DISTRIBUTION_NAME)
  execute_process(
    COMMAND "python" "-c" "from __future__ import print_function; import platform; print(platform.linux_distribution()[0].strip().lower(), end='')"
    RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME
    OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_NAME
    )
  if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME} STREQUAL "0")
    message(FATAL_ERROR "Linux platform name detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME}")
  endif()
  set(IRODS_LINUX_DISTRIBUTION_NAME ${IRODS_LINUX_DISTRIBUTION_NAME} CACHE STRING "Linux distribution name, e.g. {ubuntu, centos, ...}." FORCE)
  message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_NAME to '${IRODS_LINUX_DISTRIBUTION_NAME}'")
endif()

if (NOT IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR)
  execute_process(
    COMMAND "python" "-c" "from __future__ import print_function; import platform; print(platform.linux_distribution()[1].strip().lower().split('.')[0], end='')"
    RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR
    OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR
    )
  if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR} STREQUAL "0")
    message(FATAL_ERROR "Linux platform name detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
  set(IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR} CACHE STRING "Linux distribution name, e.g. {ubuntu, centos, ...}." FORCE)
  message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR to '${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}'")
endif()


# C++14 settings # TODO after rebuild externals with 14 and libc++ and clang std headers
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
#set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
##set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -stdlib=libc++")
#add_compile_options(-nostdinc++)
#link_libraries(c++abi)
#include_directories(${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/server_control_plane_command.hpp
  COMMAND ${IRODS_EXTERNALS_FULLPATH_AVRO}/bin/avrogencpp -n irods -o ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/server_control_plane_command.hpp -i ${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  )

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_server_control_plane.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/server_control_plane_command.hpp
  )

configure_file(
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include/rodsVersion.h.in
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/rodsVersion.h
  @ONLY
  )

configure_file(
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include/irods_version.h.in
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/irods_version.h
  @ONLY
  )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
get_git_head_revision(IRODS_GIT_REFSPEC IRODS_GIT_SHA1)
configure_file(
  ${CMAKE_SOURCE_DIR}/VERSION.json.in
  ${CMAKE_BINARY_DIR}/VERSION.json
  @ONLY
  )

set(IRODS_HOME_DIRECTORY /var/lib/irods)
configure_file(
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include/irods_home_directory.hpp.in
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/irods_home_directory.hpp
  @ONLY
  )

set(IRODS_PLUGIN_HOME_DIRECTORY /var/lib/irods/plugins/)
configure_file(
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include/irods_plugin_home_directory.hpp.in
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/irods_plugin_home_directory.hpp
  @ONLY
  )

set(
  IRODS_LIBIRODS_CLIENT_CORE_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcMisc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/trimUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/cpUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_serialization.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mkdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/list.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_server_properties.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/putUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/scanUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rmtrashUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/fsckUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/bunUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/hashtable.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_signal.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/obf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rodsPath.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/parseCommandLine.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mcollUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_log.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/apiHandler.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/getUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/miscUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/phymvUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rodsLog.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/sslSockComm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/stringOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rsyncUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/regUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/msParam.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/replUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/region.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/sockComm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_exception.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_error.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcConnect.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/getRodsEnv.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/chksumUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/clientLogin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/packStruct.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/phybunUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/lsUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_client_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mvUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_children_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rmUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/procApiRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/osauth.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/checksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/irods_hasher_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/Hasher.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/client_exec/src/irods_client_rule_execution_manager_factory.cpp
  )

add_library(
  irods_client_core
  SHARED
  ${IRODS_LIBIRODS_CLIENT_CORE_SOURCES}
  )
target_link_libraries(
  irods_client_core
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  ${OPENSSL_SSL_LIBRARY}
  )
target_include_directories(
  irods_client_core
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_client_core PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_options(irods_client_core PRIVATE -Wno-write-strings)
target_compile_definitions(
  irods_client_core
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )

#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,defs" ) # circular dependency in client libs prevents this
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,defs" )

set(
  IRODS_LIBIRODS_CLIENT_API_TABLE_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcProcStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcClientHints.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRmColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcIESClientHints.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSslStart.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcObjStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcServerReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pack_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_client_api_table.cpp
  )

add_library(
  irods_client_api_table
  SHARED
  ${IRODS_LIBIRODS_CLIENT_API_TABLE_SOURCES}
  )
target_include_directories(
  irods_client_api_table
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_client_api_table PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(
  irods_client_api_table
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )
target_compile_options(irods_client_api_table PRIVATE -Wno-write-strings)

set(
  IRODS_LIBIRODS_CLIENT_PLUGINS_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/client_exec/src/irods_client_rule_execution_manager_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_krb_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_tcp_object.cpp
  )
add_library(
  irods_client_plugins
  SHARED
  ${IRODS_LIBIRODS_CLIENT_PLUGINS_SOURCES}
  )
target_link_libraries(
  irods_client_plugins
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  )
target_include_directories(
  irods_client_plugins
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_client_plugins PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(
  irods_client_plugins
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )
target_compile_options(irods_client_plugins PRIVATE -Wno-write-strings)


set(
  IRODS_LIBIRODS_CLIENT_API_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pack_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_client_api_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_krb_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_tcp_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/client_exec/src/irods_client_rule_execution_manager_factory.cpp
  )

add_library(
  irods_client_api
  SHARED
  ${IRODS_LIBIRODS_CLIENT_API_SOURCES}
  )
target_include_directories(
  irods_client_api
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_client_api PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(
  irods_client_api
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )
target_compile_options(irods_client_api PRIVATE -Wno-write-strings)


set(
  IRODS_LIBIRODS_SERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsClientHints.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsDataPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsIESClientHints.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsModColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsObjStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsProcStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRegColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRmColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsServerReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSetRoundRobinContext.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSslStart.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/api/src/rsZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/collection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/dataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/fileOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/initServer.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_api_calling_functions.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_collection_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_data_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_database_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_database_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_database_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_database_plugin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_file_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_get_l1desc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_globals.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_mysql_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_oracle_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_physical_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_postgres_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_resource_backport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_resource_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_resource_plugin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_resource_plugin_impostor.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_resource_redirect.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_server_api_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_server_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_server_state.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_structured_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/miscServerFunct.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/objDesc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/objMetaOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/physPath.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/procLog.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/reServerLib.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/readServerConfig.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rodsConnect.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rsApiHandler.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rsIcatOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rsLog.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rsRe.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/specColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/xmsgLib.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/src/fileDriver.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/src/icatHighLevelRoutines.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/arithmetics.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/cache.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/configuration.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/conversion.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/datetime.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/extractAvuMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/filesystem.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/functions.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/genQueryMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/icatAdminMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/icatGeneralMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/index.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_ms_plugin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_re_plugin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_re_serialization.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/keyValPairMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/locks.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/mailMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/miscMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/msiHelper.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/nre.reHelpers1.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/nre.reHelpers2.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/nre.reLib1.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/nre.ruleAdminMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/nre.systemMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/printMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reDataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reIn2p3SysRule.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reNaraMetaData.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reStruct.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reSysDataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reVariableMap.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/reVariableMap.gen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/restructs.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/rules.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/sharedmemory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/testMS.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/typing.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/utils.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/xmsgMS.cpp
  )

add_library(
  irods_server
  SHARED
  ${IRODS_LIBIRODS_SERVER_SOURCES}
  )
target_link_libraries(
  irods_server
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  )
target_include_directories(
  irods_server
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_server PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(
  irods_server
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  RODS_SERVER
  ${IRODS_COMPILE_DEFINITIONS}
  )
target_compile_options(irods_server PRIVATE -Wno-write-strings)


set(
  IRODS_LIB_API_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcObjStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcModColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcRmColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcProcStat.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSslStart.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcServerReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcClientHints.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/src/rcIESClientHints.cpp
  )

set(
  IRODS_LIB_CORE_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/chksumUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/clientLogin.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/cpUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/getRodsEnv.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/getUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/lsUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/miscUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mkdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/msParam.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mvUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/obf.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/packStruct.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/parseCommandLine.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/phymvUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/procApiRequest.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/putUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcConnect.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcMisc.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/regUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/replUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rmUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rmtrashUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rodsLog.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rodsPath.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/rsyncUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/sockComm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/stringOpr.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/trimUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/mcollUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/bunUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/phybunUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/scanUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/fsckUtil.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/osauth.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/sslSockComm.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_serialization.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_error.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_log.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_children_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_signal.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_client_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_tcp_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_factory.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_network_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_krb_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_client_api_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_pack_table.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_server_properties.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/list.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/hashtable.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/region.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/irods_exception.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/src/apiHandler.cpp
  )

set(
  IRODS_LIB_HASHER_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/Hasher.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/checksum.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/src/irods_hasher_factory.cpp
  )

set(
  IRODS_LIB_RBUDP_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  )

add_library(
  RodsAPIs
  STATIC
  ${IRODS_LIB_CORE_SOURCES}
  ${IRODS_LIB_API_SOURCES}
  ${IRODS_LIB_RBUDP_SOURCES}
  ${IRODS_LIB_HASHER_SOURCES}
  )
target_link_libraries(
  RodsAPIs
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${CMAKE_DL_LIBS}
  )
target_include_directories(
  RodsAPIs
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET RodsAPIs PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(
  RodsAPIs
  PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )
target_compile_options(RodsAPIs PRIVATE -Wno-write-strings)


add_library(
  irods_clerver
  SHARED
  ${IRODS_LIBIRODS_CLIENT_CORE_SOURCES}
  ${IRODS_LIBIRODS_CLIENT_API_TABLE_SOURCES}
  ${IRODS_LIBIRODS_CLIENT_PLUGINS_SOURCES}
  )
target_link_libraries(
  irods_clerver
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${OPENSSL_SSL_LIBRARY}
  )
target_include_directories(
  irods_clerver
  PRIVATE
  ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_clerver PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_options(irods_clerver PRIVATE -Wno-write-strings)
target_compile_definitions(
  irods_clerver
  PRIVATE
  ENABLE_RE
  RODS_CLERVER
  BOOST_SYSTEM_NO_DEPRECATED
  ${IRODS_COMPILE_DEFINITIONS}
  )


set(
  IRODS_MAIN_EXECUTABLE_IRODSAGENT_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rodsAgent.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_operation_rule_execution_manager.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_IRODSSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/rodsServer.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_operation_rule_execution_manager.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irods_server_control_plane.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_IRODSRESERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irodsReServer.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_operation_rule_execution_manager.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_IRODSXMSGSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/core/src/irodsXmsgServer.cpp
  ${CMAKE_SOURCE_DIR}/iRODS/server/re/src/irods_operation_rule_execution_manager.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_TEST_CHL_SOURCES
  ${CMAKE_SOURCE_DIR}/iRODS/server/test/src/test_chl.cpp
  )

set(
  IRODS_MAIN_EXECUTABLES
  irodsAgent
  irodsServer
  irodsReServer
  irodsXmsgServer
  test_chl
  )

foreach(EXECUTABLE ${IRODS_MAIN_EXECUTABLES})
  string(TOUPPER ${EXECUTABLE} EXECUTABLE_UPPERCASE)
  add_executable(
    ${EXECUTABLE}
    ${IRODS_MAIN_EXECUTABLE_${EXECUTABLE_UPPERCASE}_SOURCES}
    )
  target_link_libraries(
    ${EXECUTABLE}
    PRIVATE
    irods_server
    irods_clerver
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    rt
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    m
    )
  target_include_directories(
    ${EXECUTABLE}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
    ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/rbudp/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
    ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
    ${OPENSSL_INCLUDE_DIR}
    )
  set_property(TARGET ${EXECUTABLE} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
  target_compile_definitions(
    ${EXECUTABLE}
    PRIVATE
    RODS_SERVER
    ENABLE_RE
    CACHE_ENABLE
    ${IRODS_COMPILE_DEFINITIONS}
    BOOST_SYSTEM_NO_DEPRECATED
    )
  target_compile_options(${EXECUTABLE} PRIVATE -Wno-write-strings)
endforeach()


add_executable(
  PamAuthCheck
  ${CMAKE_SOURCE_DIR}/iRODS/server/auth/src/PamAuthCheck.cpp
  )
target_link_libraries(
  PamAuthCheck
  PRIVATE
  ${PAM_LIBRARY}
  )


add_subdirectory(iRODS/clients)
add_subdirectory(plugins/api)
add_subdirectory(plugins/auth)
add_subdirectory(plugins/database)
add_subdirectory(plugins/microservices)
add_subdirectory(plugins/network)
add_subdirectory(plugins/resources)
add_subdirectory(plugins/rule_engines)

include(${CMAKE_SOURCE_DIR}/cmake/development_library.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/runtime_library.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/server.cmake)

set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${IRODS_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${IRODS_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${IRODS_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

# TODO better place for these
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}_PACKAGE_NAME "irods-icommands")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}_PACKAGE_DEPENDS "irods-runtime (= ${IRODS_VERSION}), libc6, libssl1.0.0")

set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_NAME "irods-database-plugin-postgres")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_DEPENDS "irods-server, unixodbc, odbc-postgresql, postgresql-client, super, libc6, perl")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_NAME "irods-database-plugin-mysql")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_DEPENDS "irods-server, unixodbc, libmyodbc, mysql-client, libc6, perl")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm;")


##install(EXPORT myproj NAMESPACE mp_ DESTINATION lib/myproj) # TODO

set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_GENERATOR DEB)

include(CPack)
