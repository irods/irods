set(ICOMMANDS_INSTALL_DIRECTORY "${IRODS_CLIENTS_DIRECTORY}/icommands" CACHE PATH "Path to install icommands executables")
set(ICOMMANDS_INSTALL_SYMLINKS TRUE CACHE BOOL "Choose whether to install symlinks to icommands in CMAKE_INSTALL_BINDIR.")

add_custom_target(icommands)

if (DEFINED BUILD_DOCS AND NOT BUILD_DOCS)
  set(IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT OFF)
else()
  set(IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT ON)
endif()

# Find help2man
if (NOT DEFINED IRODS_BUILD_ICOMMANDS_DOCS OR IRODS_BUILD_ICOMMANDS_DOCS)
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
        if (NOT DEFINED IRODS_BUILD_ICOMMANDS_DOCS)
          set(IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT OFF)
        else()
          message(SEND_ERROR "help2man version 1.37 or later required, cannot build docs")
        endif()
      endif()
    endif()
  else()
    if (NOT DEFINED IRODS_BUILD_ICOMMANDS_DOCS)
      set(IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT OFF)
    else()
      message(SEND_ERROR "help2man not found, cannot build docs")
    endif()
  endif()
endif()

# Find gzip
if (NOT DEFINED IRODS_BUILD_ICOMMANDS_DOCS OR IRODS_BUILD_ICOMMANDS_DOCS)
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
    if (NOT DEFINED IRODS_BUILD_ICOMMANDS_DOCS)
      set(IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT OFF)
    else()
      message(SEND_ERROR "gzip not found, cannot build docs")
    endif()
  endif()
endif()

set(IRODS_BUILD_ICOMMANDS_DOCS "${IRODS_BUILD_ICOMMANDS_DOCS_DEFAULT}" CACHE BOOL "Build icommands documentation")
if (IRODS_BUILD_ICOMMANDS_DOCS)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/man")
  add_custom_target(icommands-docs ALL)
  add_dependencies(icommands icommands-docs)
endif()

add_dependencies(all-clients icommands)

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
  iphymv
  ips
  iput
  ipwd
  iqdel
  iqmod
  iqstat
  iquery
  iquest
  iquota
  ireg
  irepl
  irm
  irmdir
  irmtrash
  irsync
  irule
  iscan
  istream
  isysmeta
  iticket
  itouch
  itree
  itrim
  iunreg
  iuserinfo
  izonereport
  )

foreach(EXECUTABLE IN LISTS IRODS_CLIENT_ICOMMANDS_EXECUTABLES)
  add_executable(
    ${EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${EXECUTABLE}.cpp"
    )
  target_link_libraries(
    ${EXECUTABLE}
    PRIVATE
    irods_client
    irods_plugin_dependencies
    irods_common
    nlohmann_json::nlohmann_json
    fmt::fmt
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
    ${CMAKE_DL_LIBS}
    )
  target_include_directories(
    ${EXECUTABLE}
    PRIVATE
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
    )
  target_compile_definitions(
    ${EXECUTABLE}
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    BOOST_SYSTEM_NO_DEPRECATED
    )
  if (CMAKE_VERSION VERSION_LESS "3.13.0")
    set_property(TARGET ${EXECUTABLE} APPEND PROPERTY LINK_FLAGS "-Wl,-z,origin")
  else()
    target_link_options(${EXECUTABLE} PRIVATE "LINKER:-z,origin")
  endif()
  add_dependencies(icommands ${EXECUTABLE})
  install(
    TARGETS
    ${EXECUTABLE}
    RUNTIME
    DESTINATION "${ICOMMANDS_INSTALL_DIRECTORY}"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}
    )

  if (${EXECUTABLE} STREQUAL "iapitest")
    continue()
  endif()

  if (IRODS_BUILD_ICOMMANDS_DOCS)
    set(EXECUTABLE_MANPAGE "${CMAKE_CURRENT_BINARY_DIR}/man/${EXECUTABLE}.1")
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
      COMMAND "${GZIP}" -f < "${EXECUTABLE_MANPAGE}" > "${EXECUTABLE_MANPAGE}.gz"
      )
    add_custom_target(${EXECUTABLE}-manpage DEPENDS ${EXECUTABLE_MANPAGE}.gz)
    add_dependencies(icommands-docs ${EXECUTABLE}-manpage)
    install(
      FILES
      ${EXECUTABLE_MANPAGE}.gz
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      COMPONENT ${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}
      )
  endif()
endforeach()

if (ICOMMANDS_INSTALL_SYMLINKS)
  # Normalize both
  cmake_path(
    NORMAL_PATH ICOMMANDS_INSTALL_DIRECTORY
    OUTPUT_VARIABLE ICOMMANDS_INSTALL_DIRECTORY_NORM
  )
  cmake_path(
    NORMAL_PATH CMAKE_INSTALL_BINDIR
    OUTPUT_VARIABLE CMAKE_INSTALL_BINDIR_NORM
  )

  # Remove trailing slashes
  string(REGEX MATCH "^(.+)/+$" _regex_out "${ICOMMANDS_INSTALL_DIRECTORY_NORM}")
  if (NOT ("${CMAKE_MATCH_1}" STREQUAL ""))
    set(ICOMMANDS_INSTALL_DIRECTORY_NORM "${CMAKE_MATCH_1}")
  endif()
  string(REGEX MATCH "^(.+)/+$" _regex_out "${CMAKE_INSTALL_BINDIR_NORM}")
  if (NOT ("${CMAKE_MATCH_1}" STREQUAL ""))
    set(CMAKE_INSTALL_BINDIR_NORM "${CMAKE_MATCH_1}")
  endif()

  if (NOT (ICOMMANDS_INSTALL_DIRECTORY_NORM STREQUAL CMAKE_INSTALL_BINDIR_NORM))
    cmake_path(IS_ABSOLUTE ICOMMANDS_INSTALL_DIRECTORY_NORM ICOMMANDS_INSTALL_DIRECTORY_IS_ABSOLUTE)
    cmake_path(IS_ABSOLUTE CMAKE_INSTALL_BINDIR_NORM CMAKE_INSTALL_BINDIR_IS_ABSOLUTE)

    # Make relative ICOMMANDS_INSTALL_DIRECTORY
    if (ICOMMANDS_INSTALL_DIRECTORY_IS_ABSOLUTE AND NOT CMAKE_INSTALL_BINDIR_IS_ABSOLUTE)
      set(ICOMMANDS_LINK_PATH "${ICOMMANDS_INSTALL_DIRECTORY_NORM}")
    else()
      cmake_path(
        RELATIVE_PATH ICOMMANDS_INSTALL_DIRECTORY_NORM
        BASE_DIRECTORY "${CMAKE_INSTALL_BINDIR_NORM}"
        OUTPUT_VARIABLE ICOMMANDS_LINK_PATH
      )
    endif()

    if (NOT ("${ICOMMANDS_LINK_PATH}" STREQUAL ""))
      # add trailing slash
      if (NOT (ICOMMANDS_LINK_PATH STREQUAL "/"))
        set(ICOMMANDS_LINK_PATH "${ICOMMANDS_LINK_PATH}/")
      endif()

      add_custom_target(icommands-links ALL)
      add_dependencies(icommands icommands-links)

      set(ICOMMANDS_LINK_STAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/links-staging")
      add_custom_command(
        OUTPUT "${ICOMMANDS_LINK_STAGING_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E "make_directory" "${ICOMMANDS_LINK_STAGING_DIR}"
      )

      foreach(EXECUTABLE IN LISTS IRODS_CLIENT_ICOMMANDS_EXECUTABLES)
        # Despite what CMake's documentation might say, the generator expression TARGET_FILE_NAME
        # cannot be used in the OUTPUT target for add_custom_command.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/23837
        set(EXECUTABLE_LINK_TEMPNAME "${EXECUTABLE}.link")
        set(EXECUTABLE_LINK_NAME "$<TARGET_FILE_NAME:${EXECUTABLE}>")

        set(EXECUTABLE_LINK_STAGING "${ICOMMANDS_LINK_STAGING_DIR}/${EXECUTABLE_LINK_TEMPNAME}")
        set(EXECUTABLE_LINK_DEST_PATH "${ICOMMANDS_LINK_PATH}$<TARGET_FILE_NAME:${EXECUTABLE}>")

        add_custom_command(
          OUTPUT "${EXECUTABLE_LINK_STAGING}"
          DEPENDS "${ICOMMANDS_LINK_STAGING_DIR}"
          COMMENT "making symlink for ${EXECUTABLE}"
          COMMAND "${CMAKE_COMMAND}" -E create_symlink "${EXECUTABLE_LINK_DEST_PATH}" "${EXECUTABLE_LINK_STAGING}"
        )
        add_custom_target("${EXECUTABLE}-link" DEPENDS "${EXECUTABLE_LINK_STAGING}")
        add_dependencies(icommands-links "${EXECUTABLE}-link")
        install(
          PROGRAMS "${EXECUTABLE_LINK_STAGING}"
          DESTINATION "${CMAKE_INSTALL_BINDIR}"
          RENAME "${EXECUTABLE_LINK_NAME}"
          COMPONENT ${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}
        )
      endforeach()
    endif()
  endif()
endif()

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
  DESTINATION ${IRODS_HOME_DIRECTORY}/clients/icommands
  COMPONENT ${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}
  )
