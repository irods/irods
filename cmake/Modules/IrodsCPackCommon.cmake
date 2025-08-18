include_guard(GLOBAL)

include(IrodsCPackPlatform)

if (NOT DEFINED IRODS_PACKAGE_REVISION)
  set(IRODS_PACKAGE_REVISION "0")
endif()

set(IRODS_CPACK_DEFAULT_DESCRIPTION_SUMMARY "The Integrated Rule-Oriented Data System")
set(IRODS_CPACK_DEFAULT_DESCRIPTION "The Integrated Rule-Oriented Data System (iRODS) is open source data management software used by research, commercial, and governmental organizations worldwide.

iRODS is released as a production-level distribution aimed at deployment in mission critical environments. It virtualizes data storage resources, so users can take control of their data, regardless of where and on what device the data is stored.

The development infrastructure supports exhaustive testing on supported platforms; plugin support for microservices, storage resources, authentication mechanisms, network protocols, rule engines, new API endpoints, and databases; and extensive documentation, training, and support services.")

set(CPACK_PACKAGE_CONTACT "iRODS Consortium <packages@irods.org>")
set(CPACK_PACKAGE_VENDOR "iRODS Consortium <packages@irods.org>")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://irods.org")
if (NOT DEFINED CPACK_PACKAGE_DESCRIPTION_SUMMARY)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${IRODS_CPACK_DEFAULT_DESCRIPTION_SUMMARY}")
endif()
if (NOT DEFINED CPACK_PACKAGE_DESCRIPTION_FILE AND NOT DEFINED CPACK_PACKAGE_DESCRIPTION)
  set(CPACK_PACKAGE_DESCRIPTION "${IRODS_CPACK_DEFAULT_DESCRIPTION}")
endif()
if (DEFINED CPACK_PACKAGE_DESCRIPTION AND NOT DEFINED CPACK_PACKAGE_DESCRIPTION_FILE)
  set(_irods_package_desc_file "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CPack_package_description.txt")
  file(WRITE "${_irods_package_desc_file}" "${IRODS_CPACK_DEFAULT_DESCRIPTION}")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${_irods_package_desc_file}")
endif()

set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")

# By default, sister projects should stamp their package revisions with the version of iRODS being built against
if (NOT DEFINED IRODS_STAMP_PACKAGE_REVISION)
  string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
  if (PROJECT_NAME_UPPER STREQUAL "IRODS")
    set(IRODS_STAMP_PACKAGE_REVISION OFF)
  else()
    set(IRODS_STAMP_PACKAGE_REVISION ON)
  endif()
endif()

set(CPACK_DEBIAN_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
if (IRODS_STAMP_PACKAGE_REVISION)
  set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}+${IRODS_VERSION}")
endif()
if (IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
  set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}~${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}")
endif()

set(CPACK_RPM_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR AND NOT IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "none")
  set(CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.el${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
endif()
if (IRODS_STAMP_PACKAGE_REVISION)
  set(CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}+${IRODS_VERSION}")
endif()

if (NOT DEFINED CPACK_DEBIAN_COMPRESSION_TYPE)
  set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
endif()

if (NOT DEFINED CPACK_RPM_COMPRESSION_TYPE)
  set(CPACK_RPM_COMPRESSION_TYPE "gzip")
endif()
if (NOT DEFINED CPACK_RPM_PACKAGE_SUMMARY)
  set(CPACK_RPM_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
endif()

set(CPACK_RPM_SPEC_MORE_DEFINE "%global __python %{__python3}")

# EL10 doesn't like our irods-externals RUNPATHs
if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR GREATER_EQUAL 10)
  set(CPACK_RPM_SPEC_MORE_DEFINE "${CPACK_RPM_SPEC_MORE_DEFINE}
%global __brp_check_rpaths %{nil}")
endif()
