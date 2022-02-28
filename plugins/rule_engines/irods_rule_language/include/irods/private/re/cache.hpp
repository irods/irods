#ifndef IRODS_NREP_CACHE_HPP
#define IRODS_NREP_CACHE_HPP

#include "irods/private/re/rules.hpp"
#include "irods/private/re/index.hpp"
#include "irods/private/re/configuration.hpp"
#include "irods/region.h"
#include "irods/checksum.h"

#include "irods/private/re/cache.proto.hpp"
#include "irods/private/re/proto.hpp"
#include "irods/private/re/restruct.templates.hpp"
#include "irods/private/re/end.instance.hpp"

Cache *copyCache( unsigned char **buf, size_t size, Cache *c );

Cache *restoreCache( const char* );

void applyDiff( unsigned char *pointers, long pointersSize, long diff, long pointerDiff );

void applyDiffToPointers( unsigned char *pointers, long pointersSize, long pointerDiff );

int updateCache( const char*, size_t size, Cache *cache );

#endif // IRODS_NREP_CACHE_HPP
