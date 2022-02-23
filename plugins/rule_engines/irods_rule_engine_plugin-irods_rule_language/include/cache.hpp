#ifndef IRODS_NREP_CACHE_HPP
#define IRODS_NREP_CACHE_HPP

#include "rules.hpp"
#include "index.hpp"
#include "configuration.hpp"
#include "irods/region.h"
#include "irods/checksum.h"

#include "cache.proto.hpp"
#include "proto.hpp"
#include "restruct.templates.hpp"
#include "end.instance.hpp"

Cache *copyCache( unsigned char **buf, size_t size, Cache *c );

Cache *restoreCache( const char* );

void applyDiff( unsigned char *pointers, long pointersSize, long diff, long pointerDiff );

void applyDiffToPointers( unsigned char *pointers, long pointersSize, long pointerDiff );

int updateCache( const char*, size_t size, Cache *cache );

#endif // IRODS_NREP_CACHE_HPP
