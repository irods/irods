/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef CACHE_H
#define CACHE_H
#include "rules.h"
#include "index.h"
#include "configuration.h"
#include "region.h"

#include "cache.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

Cache *copyCache(unsigned char **buf, size_t size, Cache *c);
Cache *restoreCache(unsigned char *buf);
void applyDiff(unsigned char *pointers, long pointersSize, long diff, long pointerDiff);
void applyDiffToPointers(unsigned char *pointers, long pointersSize, long pointerDiff);
int updateCache(unsigned char *shared, size_t size, Cache *cache, int processType);
#endif
