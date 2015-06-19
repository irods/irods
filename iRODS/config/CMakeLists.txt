set (RELEASE false CACHE BOOL "Indicates if we are building an iRODS release.")

set (LARGE_FILES true CACHE BOOL "Indicates if we are using 64-bit file addressing")
if (LARGE_FILES)
    add_definitions("-D_LARGEFILE_SOURCE")
    add_definitions("-D_FILE_OFFSET_BITS=64")
    add_definitions("-D_LARGE_FILES")
    add_definitions("-D_LARGEFILE64_SOURCE")
endif

set (IRODS_CATALOG true CACHE BOOL "Indicates if we are building an iRODS Catalog (iCAT) server. The alternative is a resource server.")
if (IRODS_CATALOG)
    add_definitions("-DRODS_CAT")
endif

set (OPERATING_SYSTEM "linux" CACHE STRING)
add_definitions("-D${OPERATING_SYSTEM}_platform")

set (IRODS_LIBRARY_NAME "RodsAPIs")
set (IRODS_LIBRARY "lib${IRODS_LIBRARY_NAME}.a")

