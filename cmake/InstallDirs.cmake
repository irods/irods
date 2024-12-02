# We used to override CMAKE_INSTALL_LIBDIR to always default to "lib", but starting with 5.0, we now
# use the distribution-defaults for library installation directories. Since CMAKE_INSTALL_LIBDIR is
# a cache variable, let's try and detect if an old default is in the cache.
function(warn_for_old_libdir_cache)
  if(CMAKE_INSTALL_LIBDIR STREQUAL "lib"
     AND EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt"
     AND NOT IRODS_INSTALL_LIBDIR_GOOD_DEFAULT)
    load_cache("${CMAKE_BINARY_DIR}" READ_WITH_PREFIX loaded_ CMAKE_INSTALL_LIBDIR)
    if(loaded_CMAKE_INSTALL_LIBDIR STREQUAL "lib")
      message(WARNING "The default value of CMAKE_INSTALL_LIBDIR may have changed since the CMake Cache was generated. It is recommended to use a clean build directory.")
      return()
    endif()
  endif()
  set(IRODS_INSTALL_LIBDIR_GOOD_DEFAULT TRUE CACHE INTERNAL "Do not warn if cached CMAKE_INSTALL_LIBDIR value is \"lib\"")
endfunction()

if(CMAKE_HOST_UNIX)
  # Ideally, we would default to /usr, but cmake doesn't follow its own rules,
  # so we default to / instead.
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/")
  elseif(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "Install path prefix, prepended onto install directories.")
  endif()

  # Because we set CMAKE_INSTALL_PREFIX to "/", we have to set 
  # CPACK_PACKAGING_INSTALL_PREFIX as well or things get weird.
  if(NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/" CACHE PATH "Install path prefix for packaging, prepended onto install directories.")
  endif()
endif()

warn_for_old_libdir_cache()

include(GNUInstallDirs)

# Despite what the CMake documentation says, find_package has a hard time finding our
# package configuration if it's in lib64.
if(CMAKE_INSTALL_LIBDIR MATCHES "^lib[^/]+.*")
  set(IRODS_INSTALL_CMAKEDIR_default "lib/cmake")
elseif(CMAKE_INSTALL_LIBDIR MATCHES "^usr/lib[^/]+.*")
  set(IRODS_INSTALL_CMAKEDIR_default "usr/lib/cmake")
else()
  set(IRODS_INSTALL_CMAKEDIR_default "${CMAKE_INSTALL_LIBDIR}/cmake")
endif()
set(IRODS_INSTALL_CMAKEDIR "${IRODS_INSTALL_CMAKEDIR_default}" CACHE PATH "CMake package configuration directory (lib/cmake)")

if(NOT DEFINED IRODS_INCLUDE_DIRS)
	set(IRODS_INCLUDE_DIRS "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

if(NOT DEFINED IRODS_HOME_DIRECTORY)
  set(IRODS_HOME_DIRECTORY "${CMAKE_INSTALL_LOCALSTATEDIR}/lib/irods" CACHE PATH "iRODS home directory")
endif()

if(NOT DEFINED IRODS_PLUGINS_DIRECTORY)
  set(IRODS_PLUGINS_DIRECTORY "${CMAKE_INSTALL_LIBDIR}/irods/plugins" CACHE PATH "iRODS plugins directory")
endif()

if (NOT DEFINED IRODS_DOC_DIR)
  set(IRODS_DOC_DIR "${CMAKE_INSTALL_DOCDIR}" CACHE PATH "iRODS documentation directory")
endif()
