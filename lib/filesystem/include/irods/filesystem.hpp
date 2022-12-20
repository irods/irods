#undef IRODS_FILESYSTEM_HPP_INCLUDE_HEADER

#if defined(IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API)
#  if !defined(IRODS_FILESYSTEM_HPP_FOR_SERVER)
#    define IRODS_FILESYSTEM_HPP_FOR_SERVER
#    define IRODS_FILESYSTEM_HPP_INCLUDE_HEADER
#  endif
#elif !defined(IRODS_FILESYSTEM_HPP_FOR_CLIENT)
#  define IRODS_FILESYSTEM_HPP_FOR_CLIENT
#  define IRODS_FILESYSTEM_HPP_INCLUDE_HEADER
#endif

#ifdef IRODS_FILESYSTEM_HPP_INCLUDE_HEADER

#include "irods/filesystem/filesystem.hpp"
#include "irods/filesystem/filesystem_error.hpp"
#include "irods/filesystem/path.hpp"
#include "irods/filesystem/collection_iterator.hpp"
#include "irods/filesystem/recursive_collection_iterator.hpp"

#endif // IRODS_FILESYSTEM_HPP_INCLUDE_HEADER
