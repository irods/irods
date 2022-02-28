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

# We don't do multiarch triples on debian, nor do we use the lib64 dir on centos and
# opensuse (though we probably should). We can revisit this later; for now let's just
# set our own default for CMAKE_INSTALL_LIBDIR
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Object code libraries (lib)")
endif()

include(GNUInstallDirs)

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
