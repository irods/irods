include_guard(GLOBAL)

set(IRODS_CPACK_DEFAULT_DESCRIPTION_SUMMARY "The Integrated Rule-Oriented Data System")
set(IRODS_CPACK_DEFAULT_DESCRIPTION "The Integrated Rule-Oriented Data System (iRODS) is open source data management software used by research, commercial, and governmental organizations worldwide.

iRODS is released as a production-level distribution aimed at deployment in mission critical environments. It virtualizes data storage resources, so users can take control of their data, regardless of where and on what device the data is stored.

The development infrastructure supports exhaustive testing on supported platforms; plugin support for microservices, storage resources, authentication mechanisms, network protocols, rule engines, new API endpoints, and databases; and extensive documentation, training, and support services.")

set(CPACK_PACKAGE_CONTACT "iRODS Consortium <packages@irods.org>")
set(CPACK_PACKAGE_VENDOR "iRODS Consortium <packages@irods.org>")
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
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")

if (NOT DEFINED CPACK_DEBIAN_COMPRESSION_TYPE)
  set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
endif()

if (NOT DEFINED CPACK_RPM_COMPRESSION_TYPE)
  set(CPACK_RPM_COMPRESSION_TYPE "gzip")
endif()
if (NOT DEFINED CPACK_RPM_PACKAGE_SUMMARY)
  set(CPACK_RPM_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
endif()

# TODO: set CMAKE_PROJECT_HOMEPAGE_URL instead
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")

set(CPACK_RPM_SPEC_MORE_DEFINE "%global __python %{__python3}")
