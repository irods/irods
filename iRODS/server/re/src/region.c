/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "region.h"
#include <string.h>
#ifdef REGION_MALLOC

Region *make_region(size_t is, jmp_buf *label) {
	Region *r = (Region *)malloc(sizeof(Region));
	if(r == NULL)
		return NULL;
	struct region_node *node = (struct region_node *)malloc(sizeof(struct region_node));
	node->next = NULL;
	node->ptr = NULL;
	node->size = 0;
	r->head = r->tail = node;
	return r;
}
unsigned char *region_alloc_nodesc(Region *r, size_t s, size_t *alloc_size) {
	*alloc_size =
                s>DEFAULT_BLOCK_SIZE?
                    s:
                    roundToAlignment(s);
	unsigned char *pointer = (unsigned char *)malloc(*alloc_size);
	struct region_node *node = (struct region_node *)malloc(sizeof(struct region_node));
	node->next = NULL;
	node->ptr = pointer;
	node->size = s;
	r->tail->next = node;
	r->tail = node;
	return pointer;

}
void *region_alloc(Region *r, size_t size) {
	size_t allocSize;
    unsigned char *mem = region_alloc_nodesc(r, size + CACHE_SIZE(RegionDesc, 1), &allocSize);
    ((RegionDesc *)mem)->region = r;
    ((RegionDesc *)mem)->size = allocSize;
    ((RegionDesc *)mem)->del = 0;
    return mem + CACHE_SIZE(RegionDesc, 1);
}
void region_free(Region *r) {
	while(r->head!=NULL) {
		struct region_node *node = r->head;
		r->head = node->next;
		memset(node->ptr, 0, node->size);
		if(node->ptr!=NULL) free(node->ptr);
		free(node);
	}
	free(r);
}
size_t region_size(Region *r) {
	size_t s = 0;
	struct region_node *node = r->head;
	while(node!=NULL) {
		s += node->size;
		node = node->next;
	}
	return s;
}
#else
/* utility function */
struct region_node *make_region_node(size_t is) {
	struct region_node *node = (struct region_node *)malloc(sizeof(Region));
	if(node == NULL) {
		return NULL;
	}

	node->block = (unsigned char *)malloc(is);
        memset(node->block, 0, is);
	if(node->block == NULL) {
		free(node);
		return NULL;
	}

	node->size = is;
	node->used = 0;
	node->next = NULL;

	return node;
}
Region *make_region(size_t is, jmp_buf *label) {
	Region *r = (Region *)malloc(sizeof(Region));
	if(r == NULL)
		return NULL;

	if(is == 0)
		is = DEFAULT_BLOCK_SIZE;
	struct region_node *node = make_region_node(is);
	if(node == NULL) {
		free(r);
		return NULL;
	}

	r->head = r->active = node;
        r->label = label;
        r->error.code = 0; /* set no error */
	return r;
}
unsigned char *region_alloc_nodesc(Region *r, size_t s, size_t *alloc_size) {
	if(s > r->active->size - r->active->used) {
            int blocksize;
            if(s > DEFAULT_BLOCK_SIZE) {
                blocksize=s;

            } else {
                blocksize= DEFAULT_BLOCK_SIZE;
            }
            struct region_node *next = make_region_node(blocksize);
            if(next == NULL) {
                if(r->label == NULL) { /* no error handler */
                    return NULL;
                } else { /* with error handler */
                    longjmp(*(r->label), -1);
                }
            }
            r->active->next = next;
            r->active = next;

	}

	*alloc_size =
                s>DEFAULT_BLOCK_SIZE?
                    s:
                    roundToAlignment(s);
	unsigned char *pointer = r->active->block + r->active->used;
	r->active->used+=*alloc_size;
	return pointer;

}
void *region_alloc(Region *r, size_t size) {
	size_t allocSize;
    unsigned char *mem = region_alloc_nodesc(r, size + CACHE_SIZE(RegionDesc, 1), &allocSize);
    ((RegionDesc *)mem)->region = r;
    ((RegionDesc *)mem)->size = allocSize;
    ((RegionDesc *)mem)->del = 0;
    return mem + CACHE_SIZE(RegionDesc, 1);
}
void region_free(Region *r) {
	while(r->head!=NULL) {
		struct region_node *node = r->head;
		r->head = node->next;
		/* memset(node->block, 0, node->size); */
		free(node->block);
		free(node);
	}
	free(r->label);
	free(r);
}
size_t region_size(Region *r) {
	size_t s = 0;
	struct region_node *node = r->head;
	while(node!=NULL) {
		s += node->used;
		node = node->next;
	}
	return s;
}
#endif

/* tests */
void assert(int res) {
	if(!res) {
		printf("error");
	}
}
void test() {
	Region *r = make_region(0, NULL);
	assert(region_alloc(r, 16)!=NULL);
	assert(region_alloc(r, 20)!=NULL);
	assert(region_alloc(r,1000)!=NULL);
	assert(region_alloc(r,10000)!=NULL);
	assert(region_alloc(r,16)!=NULL);
	assert(region_alloc(r,10)!=NULL);
	assert(region_alloc(r,1024-16-10)!=NULL);
	region_free(r);
}
