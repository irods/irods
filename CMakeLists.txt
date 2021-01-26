cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME
include(${CMAKE_SOURCE_DIR}/cmake/RequireOutOfSourceBuild.cmake)

find_package(IRODS 4.3.0 EXACT REQUIRED CONFIG)

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)

set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

project(icommands
  VERSION "${IRODS_VERSION}"
  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(${IRODS_TARGETS_PATH})

include(GNUInstallDirs)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'. This is the correct setting for normal builds.")
endif()

set(IRODS_BUILD_DOCS_DEFAULT ON)

# Find help2man
if (NOT DEFINED BUILD_DOCS OR BUILD_DOCS)
  if (NOT DEFINED HELP2MAN)
    set(HELP2MAN_SET_BY_CMAKE ON)
  endif()
  find_program(
    HELP2MAN
    NAMES help2man
    DOC "help2man location"
    )
  if (DEFINED HELP2MAN)
    message(STATUS "Found help2man: ${HELP2MAN}")
    if (HELP2MAN_SET_BY_CMAKE)
      # check version number, but only if find_program went fishing
      execute_process(
        COMMAND "${HELP2MAN}" --version
        OUTPUT_VARIABLE HELP2MAN_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
      string(REGEX REPLACE "\n.*" "" HELP2MAN_VERSION "${HELP2MAN_VERSION}")
      string(REGEX REPLACE "[^ ]+ +[^ ]+ +" "" HELP2MAN_VERSION "${HELP2MAN_VERSION}")
      message(STATUS "Found help2man version ${HELP2MAN_VERSION}")
      if (HELP2MAN_VERSION VERSION_LESS "1.37")
        # don't cache unusable help2man
        unset(HELP2MAN CACHE)
        if (NOT DEFINED BUILD_DOCS)
          set(IRODS_BUILD_DOCS_DEFAULT OFF)
        else()
          message(SEND_ERROR "help2man version 1.37 or later required, cannot build docs")
        endif()
      endif()
    endif()
  else()
    if (NOT DEFINED BUILD_DOCS)
      set(IRODS_BUILD_DOCS_DEFAULT OFF)
    else()
      message(SEND_ERROR "help2man not found, cannot build docs")
    endif()
  endif()
endif()

# Find gzip
if (NOT DEFINED BUILD_DOCS OR BUILD_DOCS)
  if (NOT DEFINED GZIP)
    set(GZIP_SET_BY_CMAKE ON)
  endif()
  find_program(
    GZIP
    NAMES gzip pigz
    DOC "gzip location"
    )
  if (DEFINED GZIP)
    message(STATUS "Found gzip: ${GZIP}")
  else()
    if (NOT DEFINED BUILD_DOCS)
      set(IRODS_BUILD_DOCS_DEFAULT OFF)
    else()
      message(SEND_ERROR "gzip not found, cannot build docs")
    endif()
  endif()
endif()

set(BUILD_DOCS ${IRODS_BUILD_DOCS_DEFAULT} CACHE BOOL "Build icommand documentation")
if (BUILD_DOCS)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/man")
  add_custom_target(docs ALL)
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
add_compile_options(-nostdinc++ -Wall -Wextra -Werror -Wno-unused-parameter)
link_libraries(c++abi)
include_directories(${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1)

set(
  IRODS_CLIENT_ICOMMANDS_EXECUTABLES
  iadmin
  iapitest
  ibun
  icd
  ichksum
  ichmod
  iclienthints
  icp
  ienv
  ierror
  iexit
  ifsck
  iget
  igroupadmin
  ihelp
  iinit
  ils
  ilsresc
  imcoll
  imeta
  imiscsvrinfo
  imkdir
  imv
  ipasswd
  iphybun
  iphymv
  ips
  iput
  ipwd
  iqdel
  iqmod
  iqstat
  iquest
  iquota
  ireg
  irepl
  irm
  irmdir
  irmtrash
  irods-grid
  irsync
  irule
  iscan
  istream
  isysmeta
  iticket
  itouch
  itrim
  iunreg
  iuserinfo
  izonereport
  )

foreach(EXECUTABLE ${IRODS_CLIENT_ICOMMANDS_EXECUTABLES})
  add_executable(
    ${EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/src/${EXECUTABLE}.cpp
    )
  target_link_libraries(
    ${EXECUTABLE}
    PRIVATE
    irods_client
    irods_plugin_dependencies
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
    )
  target_include_directories(
    ${EXECUTABLE}
    PRIVATE
    ${IRODS_INCLUDE_DIRS}
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
    ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
    ${IRODS_EXTERNALS_FULLPATH_JSON}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    )
  target_compile_definitions(${EXECUTABLE} PRIVATE ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${EXECUTABLE} PRIVATE -Wno-write-strings)
  set_property(TARGET ${EXECUTABLE} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
  install(
    TARGETS
    ${EXECUTABLE}
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

  if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
    if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6")
      continue()
    endif()
  endif()

  if (${EXECUTABLE} STREQUAL "iapitest")
    continue()
  endif()

  if (BUILD_DOCS)
    set(EXECUTABLE_MANPAGE "${CMAKE_BINARY_DIR}/man/${EXECUTABLE}.1")
    add_custom_command(
      OUTPUT ${EXECUTABLE_MANPAGE}
      DEPENDS ${EXECUTABLE}
      COMMENT "generating manpage ${EXECUTABLE}.1"
      COMMAND "${CMAKE_COMMAND}" -E env "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:irods_client>:$ENV{LD_LIBRARY_PATH}" "${HELP2MAN}" -h -h -N -n "an iRODS iCommand" --version-string="iRODS-${IRODS_VERSION}" $<TARGET_FILE:${EXECUTABLE}> -o "${EXECUTABLE_MANPAGE}"
      )
    add_custom_command(
      OUTPUT ${EXECUTABLE_MANPAGE}.gz
      DEPENDS ${EXECUTABLE_MANPAGE}
      COMMENT "gzipping manpage ${EXECUTABLE}.1"
      COMMAND "${GZIP}" -f -k "${EXECUTABLE_MANPAGE}"
      )
    add_custom_target(${EXECUTABLE}-manpage DEPENDS ${EXECUTABLE_MANPAGE}.gz)
    add_dependencies(docs ${EXECUTABLE}-manpage)
    install(
      FILES
      ${EXECUTABLE_MANPAGE}.gz
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      )
  endif()
endforeach()

set(
  IRODS_CLIENT_ICOMMANDS_SCRIPTS
  igetwild
  ilocate
  )
foreach(IRODS_CLIENT_ICOMMANDS_SCRIPT ${IRODS_CLIENT_ICOMMANDS_SCRIPTS})
  set(SCRIPT_SRC_PATH "${CMAKE_SOURCE_DIR}/bin/${IRODS_CLIENT_ICOMMANDS_SCRIPT}")
  install(
    FILES
    ${SCRIPT_SRC_PATH}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )

  if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
    if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6")
      continue()
    endif()
  endif()

  if (BUILD_DOCS)
    set(SCRIPT_MANPAGE "${CMAKE_BINARY_DIR}/man/${IRODS_CLIENT_ICOMMANDS_SCRIPT}.1")
    add_custom_command(
      OUTPUT ${SCRIPT_MANPAGE}
      DEPENDS ${SCRIPT_SRC_PATH}
      COMMENT "generating manpage ${IRODS_CLIENT_ICOMMANDS_SCRIPT}.1"
      COMMAND "${CMAKE_COMMAND}" -E env "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:irods_client>:$ENV{LD_LIBRARY_PATH}" "${HELP2MAN}" -h -h -N -n "an iRODS iCommand" --version-string="iRODS-${IRODS_VERSION}" "${SCRIPT_SRC_PATH}" -o "${SCRIPT_MANPAGE}"
      )
    add_custom_command(
      OUTPUT ${SCRIPT_MANPAGE}.gz
      DEPENDS ${SCRIPT_MANPAGE}
      COMMENT "gzipping manpage ${IRODS_CLIENT_ICOMMANDS_SCRIPT}.1"
      COMMAND "${GZIP}" -f -k "${SCRIPT_MANPAGE}"
      )
    add_custom_target(${IRODS_CLIENT_ICOMMANDS_SCRIPT}-manpage DEPENDS ${SCRIPT_MANPAGE}.gz)
    add_dependencies(docs ${IRODS_CLIENT_ICOMMANDS_SCRIPT}-manpage)
    install(
      FILES
      ${SCRIPT_MANPAGE}.gz
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      )
  endif()
endforeach()

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/test
  DESTINATION ${IRODS_HOME_DIRECTORY}/clients/icommands
  )


if (NOT CPACK_DEBIAN_PACKAGE_VERSION)
  set(CPACK_DEBIAN_PACKAGE_VERSION ${IRODS_CPACK_DEBIAN_PACKAGE_VERSION})
endif()

set(CPACK_PACKAGE_FILE_NAME "irods-icommands${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
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

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_VENDOR "iRODS Consortium")
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)

set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

set(CPACK_DEBIAN_PACKAGE_NAME "irods-icommands")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime (= ${CPACK_DEBIAN_PACKAGE_VERSION}), libc6, libssl1.0.0")
set(CPACK_DEBIAN_PACKAGE_REPLACES "irods-icat, irods-resource")

set(CPACK_RPM_PACKAGE_NAME "irods-icommands")
set(CPACK_RPM_PACKAGE_OBSOLETES "irods-icat, irods-resource")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos linux")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}, openssl")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}, libopenssl1_0_0")
endif()

if (NOT CPACK_GENERATOR)
  set(CPACK_GENERATOR ${IRODS_CPACK_GENERATOR} CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
  message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
endif()

include(CPack)
