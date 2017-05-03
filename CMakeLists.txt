cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME
include(${CMAKE_SOURCE_DIR}/cmake/RequireOutOfSourceBuild.cmake)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'. This is the correct setting for normal builds.")
endif()

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
  set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
  message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'. This is the correct setting for normal builds.")
endif()

macro(IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH DEPENDENCY_NAME DEPENDENCY_SUBDIRECTORY)
  if (IS_DIRECTORY ${IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME}})
    message(STATUS "Using user-specified value for IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME}: ${IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME}}")
  else()
    if (NOT IS_DIRECTORY ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
      message(FATAL_ERROR "${DEPENDENCY_NAME} not found at ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY}")
    endif()
    set(IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME} ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
  endif()
endmacro()

macro(IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST DEPENDENCY_NAME DEPENDENCY_SUBDIRECTORY)
  IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(${DEPENDENCY_NAME} ${DEPENDENCY_SUBDIRECTORY})
  list(APPEND IRODS_PACKAGE_DEPENDENCIES_LIST irods-externals-${DEPENDENCY_SUBDIRECTORY})
endmacro()

IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(CLANG clang3.8-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(CPPZMQ cppzmq4.1-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(ARCHIVE libarchive3.1.2-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(AVRO avro1.7.7-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(BOOST boost1.60.0-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(CLANG_RUNTIME clang-runtime3.8-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(JANSSON jansson2.7-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(ZMQ zeromq4-14.1.3-0)

string(REPLACE ";" ", " IRODS_PACKAGE_DEPENDENCIES_STRING "${IRODS_PACKAGE_DEPENDENCIES_LIST}")

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)

project(irods C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if (NOT CPACK_PACKAGING_INSTALL_PREFIX)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/" CACHE STRING "Package root path. \"/\" is correct for normal package builds.." FORCE)
  message(STATUS "Setting unspecified CPACK_PACKAGING_INSTALL_PREFIX to '${CPACK_PACKAGING_INSTALL_PREFIX}'. This is the correct setting for normal builds.")
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -Wl,--export-dynamic") # export-dynamic so stacktrace entries from executables have function names
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++ -Wl,-z,defs")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -Wl,-z,defs")
add_compile_options(-nostdinc++ -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter)
link_libraries(c++abi)
include_directories(${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1)

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
set(IRODS_VERSION_MINOR "3")
set(IRODS_VERSION_PATCH "0")
set(IRODS_VERSION "${IRODS_VERSION_MAJOR}.${IRODS_VERSION_MINOR}.${IRODS_VERSION_PATCH}")

set(IRODS_CXX_STANDARD 14)

set(IRODS_PLATFORM_STRING linux_platform)
set(IRODS_COMPILE_DEFINITIONS ${IRODS_PLATFORM_STRING} _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64 _LARGE_FILES _LARGEFILE64_SOURCE BOOST_SYSTEM_NO_DEPRECATED)

if (NOT IRODS_LINUX_DISTRIBUTION_NAME)
  execute_process(
    COMMAND "python" "-c" "from __future__ import print_function; import platform; print(platform.linux_distribution()[0].split()[0].strip().lower(), end='')"
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

if(IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
  if (NOT IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
    execute_process(
      COMMAND "lsb_release" "-s" "-c"
      RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME
      OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME
      )
    string(STRIP ${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME} IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
    if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME} STREQUAL "0")
      message(FATAL_ERROR "Linux lsb_release shortname detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME}")
    endif()
    set(IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME ${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME} CACHE STRING "Linux distribution version codename, e.g. {precise, wheezy, trusty, jessie, ...}." FORCE)
    message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME to '${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}'")
  endif()
endif()

if (NOT CPACK_GENERATOR)
  if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "arch")
    set(CPACK_GENERATOR TGZ CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  elseif(IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
    set(CPACK_GENERATOR DEB CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  elseif(IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "fedora")
    set(CPACK_GENERATOR RPM CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  else()
    message(FATAL_ERROR "Failed to set CPACK_GENERATOR for unknown Linux Distribution: ${IRODS_LINUX_DISTRIBUTION_NAME}")
  endif()
endif()
set(IRODS_CPACK_GENERATOR ${CPACK_GENERATOR})

if (CPACK_GENERATOR STREQUAL "DEB")
  if (NOT CPACK_DEBIAN_PACKAGE_VERSION)
    set(CPACK_DEBIAN_PACKAGE_VERSION ${IRODS_VERSION} CACHE STRING "Version string used by .deb packages." FORCE)
    message(STATUS "Setting unspecified CPACK_DEBIAN_PACKAGE_VERSION to '${CPACK_DEBIAN_PACKAGE_VERSION}'. This is the correct setting for normal builds.")
  endif()
endif()

# CPACK names component package files as "${CPACK_PACKAGE_FILE_NAME}-${COMPONENT_NAME}"
# CPackDeb TO_UPPERs all the component names when checking the ${CPACK_DEBIAN_<COMPONENT>...} variables
set(CPACK_PACKAGE_FILE_NAME "irods")
if (CPACK_GENERATOR STREQUAL "RPM")
  set(IRODS_PACKAGE_FILE_NAME_SUFFIX "-${IRODS_VERSION}-1.${CMAKE_SYSTEM_PROCESSOR}")
elseif(CPACK_GENERATOR STREQUAL "DEB")
  if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR "amd64")
  endif()
  set(IRODS_PACKAGE_FILE_NAME_SUFFIX "_${CPACK_DEBIAN_PACKAGE_VERSION}~${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}_${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(IRODS_PACKAGE_FILE_NAME_SUFFIX "-${IRODS_VERSION}-${IRODS_LINUX_DISTRIBUTION_NAME}${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(IRODS_PACKAGE_COMPONENT_SERVER_NAME "server${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_SERVER_NAME} IRODS_PACKAGE_COMPONENT_SERVER_NAME_UPPERCASE)
if (CPACK_GENERATOR STREQUAL "RPM")
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "devel${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
elseif(CPACK_GENERATOR STREQUAL "DEB")
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "dev${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
else()
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "dev${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
endif()

string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_RUNTIME_NAME "runtime${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME} IRODS_PACKAGE_COMPONENT_RUNTIME_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_POSTGRES_NAME "database-plugin-postgres${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME} IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_MYSQL_NAME "database-plugin-mysql${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_MYSQL_NAME} IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_ORACLE_NAME "database-plugin-oracle${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_ORACLE_NAME} IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE)

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json")
  message(FATAL_ERROR "Missing file: ${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
Are your git submodules initialized? If not, try running the following in the source directory:
git submodule update --init")
endif()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp
  COMMAND ${IRODS_EXTERNALS_FULLPATH_AVRO}/bin/avrogencpp -n irods -o ${CMAKE_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp -i ${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  )

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_control_plane.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp
  )

configure_file(
  ${CMAKE_SOURCE_DIR}/lib/core/include/rodsVersion.h.in
  ${CMAKE_BINARY_DIR}/lib/core/include/rodsVersion.h
  @ONLY
  )

configure_file(
  ${CMAKE_SOURCE_DIR}/lib/core/include/irods_version.h.in
  ${CMAKE_BINARY_DIR}/lib/core/include/irods_version.h
  @ONLY
  )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
get_git_head_revision(IRODS_GIT_REFSPEC IRODS_GIT_SHA1)
configure_file(
  ${CMAKE_SOURCE_DIR}/VERSION.json.dist.in
  ${CMAKE_BINARY_DIR}/VERSION.json.dist
  @ONLY
  )

set(
  IRODS_LIBIRODS_CLIENT_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcClientHints.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecRuleExpression.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFilePut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcIESClientHints.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcObjStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcProcStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRmColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcServerReport.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSslStart.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/bunUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/chksumUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/clientLogin.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/cpUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/fsckUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/getUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_c_api.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_client_api_table.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_client_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/lsUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mcollUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/miscUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mkdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mvUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/phybunUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/phymvUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/procApiRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/putUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcConnect.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/regUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/replUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmtrashUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rsyncUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/scanUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/sockComm.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/sslSockComm.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/trimUtil.cpp
  )
add_library(
  irods_client
  SHARED
  ${IRODS_LIBIRODS_CLIENT_SOURCES}
  )
target_link_libraries(
  irods_client
  PRIVATE
  irods_plugin_dependencies
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  )
target_include_directories(
  irods_client
  PRIVATE
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_client PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
set_property(TARGET irods_client PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_client PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_options(irods_client PRIVATE -Wno-write-strings)
target_compile_definitions(irods_client PRIVATE ${IRODS_COMPILE_DEFINITIONS})


set(
  IRODS_LIBIRODS_SERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/server/api/src/rsAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsClientHints.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataGet.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsDataPut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsExecRuleExpression.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileClose.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileGet.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFilePut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileRead.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileRename.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsIESClientHints.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsModColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsObjStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsProcStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRegColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRmColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsServerReport.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSetRoundRobinContext.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSslStart.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/server/api/src/rsZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/collection.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/dataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/fileOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/initServer.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_api_calling_functions.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_collection_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_data_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_database_factory.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_database_manager.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_database_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_database_plugin.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_file_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_get_l1desc.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_report_plugins_in_json.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_mysql_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_oracle_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_physical_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_postgres_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_resource_backport.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_resource_manager.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_resource_plugin.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_resource_plugin_impostor.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_resource_redirect.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_api_table.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_globals.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_state.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_signal.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_structured_object.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/miscServerFunct.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/objDesc.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/objMetaOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/physPath.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/procLog.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/rodsAgent.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/rodsConnect.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/rsApiHandler.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/rsIcatOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/rsLog.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/specColl.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/xmsgLib.cpp
  ${CMAKE_SOURCE_DIR}/server/drivers/src/fileDriver.cpp
  ${CMAKE_SOURCE_DIR}/server/icat/src/icatHighLevelRoutines.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/extractAvuMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/genQueryMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/icatAdminMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/icatGeneralMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_ms_plugin.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_re_plugin.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_re_structs.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/reAction.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/nre.systemMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_re_namespaceshelper.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_re_ruleexistshelper.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/irods_re_serialization.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/keyValPairMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/locks.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/mailMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/miscMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/printMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/reDataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/reIn2p3SysRule.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/reNaraMetaData.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/reSysDataObjOpr.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/sharedmemory.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/testMS.cpp
  ${CMAKE_SOURCE_DIR}/server/re/src/xmsgMS.cpp
  )
add_library(
  irods_server
  SHARED
  ${IRODS_LIBIRODS_SERVER_SOURCES}
  ${IRODS_LIBIRODS_CLIENT_SOURCES}
  )
target_link_libraries(
  irods_server
  PRIVATE
  irods_common
  irods_plugin_dependencies
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${CMAKE_DL_LIBS}
  rt
  ${CMAKE_THREAD_LIBS_INIT}
  )
target_include_directories(
  irods_server
  PRIVATE
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/server/api/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irods_server PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
set_property(TARGET irods_server PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_server PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_server PRIVATE ENABLE_RE RODS_CLERVER ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(irods_server PRIVATE -Wno-write-strings)


set(
  IRODS_LIBIRODS_COMMON_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/core/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/getRodsEnv.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/hashtable.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_children_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_default_paths.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_error.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_exception.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_hostname.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_log.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pack_table.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_random.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_serialization.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_server_properties.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_socket_information.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/list.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/msParam.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/obf.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/osauth.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/packStruct.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/parseCommandLine.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcGlobal.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcMisc.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/region.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rodsLog.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rodsPath.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/stringOpr.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/Hasher.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/checksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/irods_hasher_factory.cpp
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  )
add_library(
  irods_common
  SHARED
  ${IRODS_LIBIRODS_COMMON_SOURCES}
  )
target_link_libraries(
  irods_common
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_random.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${CMAKE_DL_LIBS}
  )
target_include_directories(
  irods_common
  PRIVATE
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  )
set_property(TARGET irods_common PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
set_property(TARGET irods_common PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_common PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_common PRIVATE ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(irods_common PRIVATE -Wno-write-strings)


set(
  IRODS_LIBIRODS_PLUGIN_DEPENDENCIES_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/core/src/apiHandler.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_factory.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_krb_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_factory.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_manager.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_tcp_object.cpp
  )
add_library(
  irods_plugin_dependencies
  SHARED
  ${IRODS_LIBIRODS_PLUGIN_DEPENDENCIES_SOURCES}
  )
target_link_libraries(
  irods_plugin_dependencies
  PRIVATE
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_random.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${CMAKE_DL_LIBS}
  )
target_include_directories(
  irods_plugin_dependencies
  PRIVATE
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  )
set_property(TARGET irods_plugin_dependencies PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
set_property(TARGET irods_plugin_dependencies PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_plugin_dependencies PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_plugin_dependencies PRIVATE ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(irods_plugin_dependencies PRIVATE -Wno-write-strings)


set(
  IRODS_LIB_API_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetMiscSvrInfo.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileChmod.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileGetFsFreeSpace.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRepl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFilePut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataCopy.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCopy.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSimpleQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCollCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileChksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcChkNVPathPerm.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGenQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthResponse.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthCheck.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegDataObj.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUnregDataObj.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRegReplica.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModDataObjMeta.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModAVUMetadata.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModAccessControl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecSubmit.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecRuleExpression.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecDel.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRuleExecMod.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecMyRule.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcOprComplete.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjRsync.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjChksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPhyPathReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjPhymv.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjTrim.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcObjStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcExecCmd.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileCreate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileOpen.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileWrite.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileUnlink.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileLseek.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRename.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcQuerySpecColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetTempPassword.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcModColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileMkdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileRmdir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileOpendir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileReaddir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileClosedir.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralUpdate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetXmsgTicket.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSendXmsg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRcvXmsg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFilePut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSyncMountedColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileSync.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcOpenCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcReadCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCloseCollection.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcCollRepl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcRmColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileExtract.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileExtAndReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStructFileBundle.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcChkObjPermAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUserAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetRemoteZoneResc.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjOpenAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcL3FileGetSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcL3FilePutSingleBuf.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjCreateAndStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileStageToCache.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileSyncToArch.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralRowInsert.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGeneralRowPurge.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPhyBundleColl.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcUnbunAndRegPhyBunfile.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHostForPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetRescQuota.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcBulkDataObjReg.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcBulkDataObjPut.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcEndTransaction.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcProcStat.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStreamRead.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSpecificQuery.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcStreamClose.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHostForGet.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjLock.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcTicketAdmin.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetTempPasswordForOther.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcPamAuthRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSslStart.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSslEnd.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcDataObjTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcSubStructFileTruncate.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcAuthPluginRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetLimitedPassword.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHierarchyForResc.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcServerReport.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcZoneReport.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcClientHints.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcGetHierFromLeafId.cpp
  ${CMAKE_SOURCE_DIR}/lib/api/src/rcIESClientHints.cpp
  )

set(
  IRODS_LIB_CORE_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/core/src/apiHandler.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/bunUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/chksumUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/clientLogin.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/cpUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/fsckUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/getRodsEnv.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/getUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/hashtable.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_factory.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_children_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_c_api.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_client_api_table.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_client_negotiation.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_default_paths.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_error.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_exception.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_krb_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_log.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_factory.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_manager.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_network_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pack_table.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_random.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_serialization.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_server_properties.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_socket_information.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_tcp_object.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/list.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/lsUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mcollUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/miscUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mkdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/msParam.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/mvUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/obf.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/osauth.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/packStruct.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/parseCommandLine.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/phybunUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/phymvUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/procApiRequest.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/putUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcConnect.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcGlobal.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcMisc.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/regUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/region.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/replUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmdirUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rmtrashUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rodsLog.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rodsPath.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/rsyncUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/scanUtil.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/sockComm.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/sslSockComm.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/stringOpr.cpp
  ${CMAKE_SOURCE_DIR}/lib/core/src/trimUtil.cpp
  )

set(
  IRODS_LIB_HASHER_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/Hasher.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/checksum.cpp
  ${CMAKE_SOURCE_DIR}/lib/hasher/src/irods_hasher_factory.cpp
  )

set(
  IRODS_LIB_RBUDP_SOURCES
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  ${CMAKE_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  )

set(
  IRODS_LIB_CLIENT_EXEC_SOURCES
  )

add_library(
  RodsAPIs
  STATIC
  ${IRODS_LIB_CORE_SOURCES}
  ${IRODS_LIB_API_SOURCES}
  ${IRODS_LIB_RBUDP_SOURCES}
  ${IRODS_LIB_HASHER_SOURCES}
  ${IRODS_LIB_CLIENT_EXEC_SOURCES}
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
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET RodsAPIs PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(RodsAPIs PRIVATE ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(RodsAPIs PRIVATE -Wno-write-strings)

set(
  IRODS_MAIN_EXECUTABLE_IRODSSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/server/core/src/rodsServer.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/irods_server_control_plane.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_IRODSXMSGSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/server/core/src/irodsXmsgServer.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_HOSTNAME_RESOLVES_TO_LOCAL_ADDRESS_SOURCES
  ${CMAKE_SOURCE_DIR}/server/core/src/hostname_resolves_to_local_address.cpp
  )

set(
  IRODS_MAIN_EXECUTABLES
#  irodsAgent
  irodsServer
  irodsXmsgServer
  hostname_resolves_to_local_address
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
    irods_common
    irods_plugin_dependencies
    irods_server
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
    ${CMAKE_SOURCE_DIR}/server/re/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/drivers/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/hasher/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/lib/rbudp/include
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
    ENABLE_RE
    ${IRODS_COMPILE_DEFINITIONS}
    )
  target_compile_options(${EXECUTABLE} PRIVATE -Wno-write-strings)
endforeach()

add_executable(
  irodsReServer
  ${CMAKE_SOURCE_DIR}/server/core/src/irodsReServer.cpp
  ${CMAKE_SOURCE_DIR}/server/core/src/reServerLib.cpp
  )
target_link_libraries(
  irodsReServer
  PRIVATE
  irods_client
  irods_common
  irods_plugin_dependencies
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
  irodsReServer
  PRIVATE
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/hasher/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/rbudp/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
  ${OPENSSL_INCLUDE_DIR}
  )
set_property(TARGET irodsReServer PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(irodsReServer PRIVATE ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(irodsReServer PRIVATE -Wno-write-strings)

add_executable(
  irodsPamAuthCheck
  ${CMAKE_SOURCE_DIR}/server/auth/src/irodsPamAuthCheck.cpp
  )
target_link_libraries(
  irodsPamAuthCheck
  PRIVATE
  ${PAM_LIBRARY}
  )
set_property(TARGET irodsPamAuthCheck PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})

add_executable(
  genOSAuth
  ${CMAKE_SOURCE_DIR}/server/auth/src/genOSAuth.cpp
  )
target_include_directories(
  genOSAuth
  PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_BINARY_DIR}/lib/core/include
  )
target_link_libraries(
  genOSAuth
  PRIVATE
  irods_client
  irods_plugin_dependencies
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  )
set_property(TARGET genOSAuth PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
target_compile_definitions(genOSAuth PRIVATE ${IRODS_COMPILE_DEFINITIONS})
target_compile_options(genOSAuth PRIVATE -Wno-write-strings)

set(IRODS_HOME_DIRECTORY var/lib/irods)
set(IRODS_PLUGINS_DIRECTORY usr/lib/irods/plugins)

add_subdirectory(plugins/api)
add_subdirectory(plugins/auth)
add_subdirectory(plugins/database)
add_subdirectory(plugins/microservices)
add_subdirectory(plugins/network)
add_subdirectory(plugins/resources)
add_subdirectory(plugins/rule_engines)
add_subdirectory(test/c_api_test)

include(${CMAKE_SOURCE_DIR}/cmake/development_library.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/runtime_library.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/server.cmake)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${IRODS_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${IRODS_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${IRODS_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")
set(CPACK_PACKAGE_VENDOR "Renaissance Computing Institute <info@irods.org>")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_VENDOR "iRODS Consortium")
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/sbin")

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)


### TODO better place for these
## database plugins
# postgres
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-postgres")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}), unixodbc, odbc-postgresql, postgresql-client, super, libc6")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_NAME "irods-database-plugin-postgres")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6" OR IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, unixODBC, postgresql, authd, postgresql-odbc")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, unixODBC, postgresql, psqlODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm")

# mysql
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-mysql")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" AND IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "16")
  set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}), unixodbc, mysql-client, libc6")
else()
  set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}), unixodbc, libmyodbc, mysql-client, libc6")
endif()
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_NAME "irods-database-plugin-mysql")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6" OR IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, mysql, unixODBC, mysql-connector-odbc")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, mysql-client, unixODBC, MyODBC-unixODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm")

# oracle
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-oracle")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}), unixodbc, libc6")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_NAME "irods-database-plugin-oracle")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, unixODBC")
  elseif (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, unixODBC")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, unixODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/plugins/database/packaging/postrm")


# Install configuration schemas with updated id field
set(
  IRODS_CONFIGURATION_SCHEMA_VERSION
  3
  )
set(
  IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY
  ${CMAKE_BINARY_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}
  )
set(
  IRODS_CONFIGURATION_SCHEMA_FILES
  VERSION.json
  client_environment.json
  client_hints.json
  configuration_directory.json
  database_config.json
  grid_status.json
  host_access_control_config.json
  hosts_config.json
  plugin.json
  resource.json
  rule_engine.json
  server.json
  server_config.json
  server_status.json
  service_account_environment.json
  zone_bundle.json
  )
foreach(SCHEMA_FILE ${IRODS_CONFIGURATION_SCHEMA_FILES})
  configure_file(
    ${CMAKE_SOURCE_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}/${SCHEMA_FILE}
    ${IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY}/${SCHEMA_FILE}
    COPYONLY
    )
  install(
    FILES
    ${IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY}/${SCHEMA_FILE}
    DESTINATION ${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
    )
endforeach()
install(
  CODE
  "execute_process(COMMAND python ${CMAKE_SOURCE_DIR}/configuration_schemas/update_schema_ids_for_cmake.py ${IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY} ${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION})"
  COMPONENT
  ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )


set(IRODS_INCLUDE_DIRS usr/include/irods)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/IRODSConfig.cmake.not_yet_installed.in
  ${CMAKE_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed # suffix prevents cmake's find_package() from using this copy of the file
  INSTALL_DESTINATION usr/lib/irods/cmake
  PATH_VARS IRODS_INCLUDE_DIRS
  )

install(
  FILES
  ${CMAKE_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed
  DESTINATION usr/lib/irods/cmake
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  RENAME IRODSConfig.cmake
  )

write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/IRODSConfigVersion.cmake
  VERSION ${IRODS_VERSION}
  COMPATIBILITY ExactVersion
  )

install(
  FILES
  ${CMAKE_BINARY_DIR}/IRODSConfigVersion.cmake
  DESTINATION usr/lib/irods/cmake
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  )

foreach(DATABASE_PLUGIN postgres oracle mysql)
  string(TOUPPER ${DATABASE_PLUGIN} DATABASE_PLUGIN_UPPER)
  add_custom_target(
    non-package-install-${DATABASE_PLUGIN}
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_SERVER_NAME} -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME} -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_${DATABASE_PLUGIN_UPPER}_NAME} -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    python ${CMAKE_SOURCE_DIR}/configuration_schemas/update_schema_ids_for_cmake.py ${CMAKE_INSTALL_PREFIX}/${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION} ${CMAKE_INSTALL_PREFIX}/${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}
    DEPENDS
    irodsServer irodsReServer irodsXmsgServer hostname_resolves_to_local_address irodsPamAuthCheck irods_client irods_server irods_common irods_plugin_dependencies RodsAPIs helloworld_server helloworld_client msisync_to_archive msi_update_unixfilesystem_resource_free_space compound deferred load_balanced mockarchive nonblocking passthru random replication roundrobin structfile univmss unixfilesystem irods_rule_engine_plugin-irods_rule_language irods_rule_engine_plugin-cpp_default_policy native_client native_server osauth_client osauth_server pam_client pam_server ssl_client ssl_server tcp_client tcp_server genOSAuth mytest
    ${DATABASE_PLUGIN} IRODS_PHONY_TARGET_icatSysTables_${DATABASE_PLUGIN}.sql
    )
endforeach()

include(CPack)
