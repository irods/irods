include_guard(GLOBAL)

set(CPACK_PACKAGE_CONTACT "iRODS Consortium <packages@irods.org>")
set(CPACK_PACKAGE_VENDOR "iRODS Consortium <packages@irods.org>")

set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")

if (NOT DEFINED CPACK_DEBIAN_COMPRESSION_TYPE)
  set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
endif()

if (NOT DEFINED CPACK_RPM_COMPRESSION_TYPE)
  set(CPACK_RPM_COMPRESSION_TYPE "gzip")
endif()

# TODO: set CMAKE_PROJECT_HOMEPAGE_URL instead
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")

set(CPACK_RPM_SPEC_MORE_DEFINE "%global __python %{__python3}")
