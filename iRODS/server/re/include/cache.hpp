/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef CACHE_HPP
#define CACHE_HPP
#include "rules.hpp"
#include "index.hpp"
#include "configuration.hpp"
#include "region.h"

#include "cache.proto.hpp"
#include "proto.hpp"
#include "restruct.templates.hpp"
#include "end.instance.hpp"

Cache *copyCache( unsigned char **buf, size_t size, Cache *c );
Cache *restoreCache( unsigned char *buf );
void applyDiff( unsigned char *pointers, long pointersSize, long diff, long pointerDiff );
void applyDiffToPointers( unsigned char *pointers, long pointersSize, long pointerDiff );
int updateCache( unsigned char *shared, size_t size, Cache *cache, int processType );
#endif
